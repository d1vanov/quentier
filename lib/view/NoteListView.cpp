/*
 * Copyright 2017-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteListView.h"

#include "NotebookItemView.h"
#include "Utils.h"

#include <lib/model/note/NoteModel.h>
#include <lib/model/notebook/NotebookItem.h>
#include <lib/model/notebook/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include <algorithm>
#include <iterator>
#include <utility>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view::NoteListView", errorDescription);                     \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

NoteListView::NoteListView(QWidget * parent) : QListView{parent} {}

void NoteListView::setNotebookItemView(NotebookItemView * notebookItemView)
{
    QNTRACE("view::NoteListView", "NoteListView::setNotebookItemView");
    m_notebookItemView = notebookItemView;
}

void NoteListView::setAutoSelectNoteOnNextAddition()
{
    QNTRACE(
        "view::NoteListView", "NoteListView::setAutoSelectNoteOnNextAddition");
    m_shouldSelectFirstNoteOnNextNoteAddition = true;
}

QStringList NoteListView::selectedNotesLocalIds() const
{
    QStringList result;

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return result;
    }

    const auto indexes = selectedIndexes();
    result.reserve(indexes.size());

    for (const auto & modelIndex: std::as_const(indexes)) {
        const auto * item = noteModel->itemForIndex(modelIndex);
        if (Q_UNLIKELY(!item)) {
            QNWARNING(
                "view::NoteListView",
                "Found no note model item for selected index");
            continue;
        }

        if (!result.contains(item->localId())) {
            result.push_back(item->localId());
        }
    }

    QNTRACE(
        "view::NoteListView",
        "Local ids of selected notes: "
            << (result.isEmpty() ? QStringLiteral("<empty>")
                                 : result.join(QStringLiteral(", "))));

    return result;
}

QString NoteListView::currentNoteLocalId() const
{
    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return {};
    }

    const auto currentModelIndex = currentIndex();
    if (!currentModelIndex.isValid()) {
        QNDEBUG("view::NoteListView", "Note view has no valid current index");
        return {};
    }

    const auto * item = noteModel->itemForIndex(currentModelIndex);
    if (Q_UNLIKELY(!item)) {
        QNWARNING(
            "view::NoteListView",
            "Found no note model item for the current index");
        return {};
    }

    return item->localId();
}

const Account & NoteListView::currentAccount() const noexcept
{
    return m_currentAccount;
}

void NoteListView::setCurrentAccount(Account account)
{
    m_currentAccount = std::move(account);
}

void NoteListView::setCurrentNoteByLocalId(QString noteLocalId)
{
    QNTRACE(
        "view::NoteListView",
        "NoteListView::setCurrentNoteByLocalId: " << noteLocalId);

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const auto index = noteModel->indexForLocalId(noteLocalId);
    setCurrentIndex(index);

    m_lastCurrentNoteLocalId = noteLocalId;

    QNDEBUG(
        "view::NoteListView",
        "Updated last current note local id to " << noteLocalId);
}

void NoteListView::setShowNoteThumbnailsState(
    const bool showThumbnailsForAllNotes, QSet<QString> hideThumbnailsLocalIds)
{
    QNDEBUG("view::NoteListView", "NoteListView::setShowNoteThumbnailsState");

    m_showThumbnailsForAllNotes = showThumbnailsForAllNotes;
    m_hideThumbnailsLocalIds = std::move(hideThumbnailsLocalIds);
}

void NoteListView::selectNotesByLocalIds(const QStringList & noteLocalIds)
{
    QNTRACE(
        "view::NoteListView",
        "NoteListView::selectNotesByLocalIds: "
            << noteLocalIds.join(QStringLiteral(", ")));

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        QNDEBUG(
            "view::NoteListView",
            "Can't select notes by local ids: no selection model within the "
            "view");
        return;
    }

    QItemSelection selection;

    QItemSelectionModel::SelectionFlags selectionFlags(
        QItemSelectionModel::Select);

    for (const auto & noteLocalId: std::as_const(noteLocalIds)) {
        const auto modelIndex = noteModel->indexForLocalId(noteLocalId);
        if (Q_UNLIKELY(!modelIndex.isValid())) {
            QNDEBUG(
                "view::NoteListView",
                "The model index for note local id "
                    << noteLocalId << " is invalid, skipping");
            continue;
        }

        QItemSelection currentSelection;
        currentSelection.select(modelIndex, modelIndex);
        selection.merge(currentSelection, selectionFlags);
    }

    selectionFlags = QItemSelectionModel::SelectionFlags(
        QItemSelectionModel::ClearAndSelect);

    selectionModel->select(selection, selectionFlags);
}

void NoteListView::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QListView::dataChanged(topLeft, bottomRight, roles);
}

void NoteListView::rowsAboutToBeRemoved(
    const QModelIndex & parent, const int start, const int end)
{
    QNTRACE(
        "view::NoteListView",
        "NoteListView::rowsAboutToBeRemoved: start = " << start
                                                       << ", end = " << end);

    const QModelIndex current = currentIndex();
    if (current.isValid()) {
        const int row = current.row();
        if ((row >= start) && (row <= end)) {
            // The default implementation moves current to the next remaining
            // item but for this view it makes more sense to just get rid of
            // the item altogether
            setCurrentIndex(QModelIndex{});

            // Also note that the last current note local id is not changed
            // here: while the row with it would be removed shortly, the row
            // removal might correspond to the re-sorting or filtering of note
            // model items so the row corresponding to this note might become
            // available again before some other note item is selected manually.
            // To account for this, leaving the last current note local id
            // untouched for now.
        }
    }

    QListView::rowsAboutToBeRemoved(parent, start, end);
}

void NoteListView::rowsInserted(
    const QModelIndex & parent, const int start, const int end)
{
    QNTRACE(
        "view::NoteListView",
        "NoteListView::rowsInserted: start = " << start << ", end = " << end);

    QListView::rowsInserted(parent, start, end);

    if (Q_UNLIKELY(m_shouldSelectFirstNoteOnNextNoteAddition)) {
        m_shouldSelectFirstNoteOnNextNoteAddition = false;

        // NOTE: for some reason it's not safe to set the current index right
        // here right now: sometimes things crash somewhere in the gory guts
        // of Qt. So scheduling a separate callback as a workaround
        QTimer * timer = new QTimer(this);
        timer->setSingleShot(true);

        QObject::connect(
            timer, &QTimer::timeout, this,
            &NoteListView::onSelectFirstNoteEvent);

        timer->start(0);
        return;
    }

    // Need to check if the row inserting made the last current note local id
    // available
    const auto currentModelIndex = currentIndex();
    if (currentModelIndex.isValid()) {
        QNTRACE(
            "view::NoteListView",
            "Current model index is already valid, no need to restore "
            "anything");
        return;
    }

    if (m_lastCurrentNoteLocalId.isEmpty()) {
        QNTRACE(
            "view::NoteListView",
            "The last current note local id is empty, no current note item to "
            "restore");
        return;
    }

    QNDEBUG(
        "view::NoteListView",
        "Should try to re-select the last current note local id");

    // As above, it is not safe to attempt to change the current model index
    // right inside this callback so scheduling a separate one
    QTimer * timer = new QTimer(this);
    timer->setSingleShot(true);

    QObject::connect(
        timer, &QTimer::timeout, this,
        &NoteListView::onTrySetLastCurrentNoteByLocalIdEvent);

    timer->start(0);
    return;
}

void NoteListView::onCreateNewNoteAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onCreateNewNoteAction");
    Q_EMIT newNoteCreationRequested();
}

void NoteListView::onDeleteNoteAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onDeleteNoteAction");

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    ErrorString error;
    if (!noteModel->deleteNote(noteLocalId, error)) {
        ErrorString errorDescription{QT_TR_NOOP("Can't delete note: ")};
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view::NoteListView", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }
}

void NoteListView::onEditNoteAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onEditNoteAction");

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    Q_EMIT editNoteDialogRequested(noteLocalId);
}

void NoteListView::onMoveToOtherNotebookAction()
{
    QNTRACE("view::NoteListView", "NoteListView::onMoveToOtherNotebookAction");

    const QStringList actionData = actionDataStringList();
    if (actionData.isEmpty() || (actionData.size() != 2)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move note to another notebook: internal "
                       "error, wrong action data"));
        return;
    }

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const QString noteLocalId = actionData[0];
    const QString notebookName = actionData[1];

    ErrorString error;
    if (!noteModel->moveNoteToNotebook(noteLocalId, notebookName, error)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't move note to another notebook: ")};

        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view::NoteListView", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onOpenNoteInSeparateWindowAction()
{
    QNDEBUG(
        "view::NoteListView", "NoteListView::onOpenNoteInSeparateWindowAction");

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    Q_EMIT openNoteInSeparateWindowRequested(noteLocalId);
}

void NoteListView::onUnfavoriteAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onUnfavoriteAction");

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    ErrorString error;
    if (!noteModel->unfavoriteNote(noteLocalId, error)) {
        ErrorString errorDescription{QT_TR_NOOP("Can't unfavorite note: ")};
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view::NoteListView", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onFavoriteAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onFavoriteAction");

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    ErrorString error;
    if (!noteModel->favoriteNote(noteLocalId, error)) {
        ErrorString errorDescription{QT_TR_NOOP("Can't favorite note: ")};
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view::NoteListView", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onShowNoteInfoAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onShowNoteInfoAction");

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }
    Q_EMIT noteInfoDialogRequested(noteLocalId);
}

void NoteListView::onToggleThumbnailPreference()
{
    QNDEBUG("view::NoteListView", "NoteListView::onToggleThumbnailPreference");

    // note here empty string is OK (means all notes)
    const QString noteLocalId = actionDataString();
    Q_EMIT toggleThumbnailsPreference(noteLocalId);
}

void NoteListView::onCopyInAppNoteLinkAction()
{
    QNDEBUG("view::NoteListView", "NoteListView::onCopyInAppNoteLinkAction");

    const auto noteLocalIdAndGuid = actionDataStringList();
    if (Q_UNLIKELY(noteLocalIdAndGuid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't copy in-app note link: internal error, "
                       "no note local id and guid were passed to "
                       "the action handler"));
        return;
    }

    if (Q_UNLIKELY(noteLocalIdAndGuid.size() != 2)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't copy in-app note link: internal error, "
                       "unidentified data received instead of note "
                       "local id and guid"));
        return;
    }

    QNTRACE(
        "view::NoteListView",
        "Requesting copy in app note link for note model "
            << "item: local id = " << noteLocalIdAndGuid.at(0)
            << ", guid = " << noteLocalIdAndGuid.at(1));

    Q_EMIT copyInAppNoteLinkRequested(
        noteLocalIdAndGuid.at(0), noteLocalIdAndGuid.at(1));
}

void NoteListView::onExportSingleNoteToEnexAction()
{
    QNDEBUG(
        "view::NoteListView", "NoteListView::onExportSingleNoteToEnexAction");

    const QString noteLocalId = actionDataString();
    if (noteLocalId.isEmpty()) {
        return;
    }

    QStringList noteLocalIds;
    noteLocalIds << noteLocalId;
    Q_EMIT enexExportRequested(noteLocalIds);
}

void NoteListView::onExportSeveralNotesToEnexAction()
{
    QNDEBUG(
        "view::NoteListView", "NoteListView::onExportSeveralNotesToEnexAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "can't cast the slot invoker to QAction"));
        return;
    }

    const auto noteLocalIds = action->data().toStringList();
    if (Q_UNLIKELY(noteLocalIds.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't export note to ENEX: internal error, the list of "
                       "local ids of notes to be exported is empty"));
        return;
    }

    Q_EMIT enexExportRequested(noteLocalIds);
}

void NoteListView::onSelectFirstNoteEvent()
{
    QNDEBUG("view::NoteListView", "NoteListView::onSelectFirstNoteEvent");

    const auto * model = this->model();
    if (Q_UNLIKELY(!model)) {
        QNERROR(
            "view::NoteListView",
            "No model is set to note list view on select first note event");
        return;
    }

    setCurrentIndex(model->index(
        0, static_cast<int>(NoteModel::Column::Title), QModelIndex{}));
}

void NoteListView::onTrySetLastCurrentNoteByLocalIdEvent()
{
    QNDEBUG(
        "view::NoteListView",
        "NoteListView::onTrySetLastCurrentNoteByLocalIdEvent");

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const auto modelIndex =
        noteModel->indexForLocalId(m_lastCurrentNoteLocalId);
    if (!modelIndex.isValid()) {
        QNTRACE(
            "view::NoteListView",
            "No valid model index within the note model for "
                << "the last current note local id "
                << m_lastCurrentNoteLocalId);
        return;
    }

    setCurrentIndex(modelIndex);
}

void NoteListView::contextMenuEvent(QContextMenuEvent * event)
{
    QNDEBUG("view::NoteListView", "NoteListView::contextMenuEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "view::NoteListView",
            "Detected Qt error: note list view received context menu event "
            "with null pointer to the context menu event");
        return;
    }

    showContextMenuAtPoint(event->pos(), event->globalPos());
}

void NoteListView::showContextMenuAtPoint(
    const QPoint & pos, const QPoint & globalPos)
{
    QNDEBUG("view::NoteListView", "NoteListView::showContextMenuAtPoint");

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: "
                       "can't get the selection model from the view"));
        return;
    }

    auto selectedRowIndexes = selectionModel->selectedIndexes();
    selectedRowIndexes << currentIndex();

    QStringList noteLocalIds;
    noteLocalIds.reserve(selectedRowIndexes.size());

    for (const auto & modelIndex: std::as_const(selectedRowIndexes)) {
        const auto * noteModelItem = noteModel->itemForIndex(modelIndex);
        if (Q_UNLIKELY(!noteModelItem)) {
            QNWARNING(
                "view::NoteListView",
                "Detected selected model index for which no model item was "
                "found");
            continue;
        }

        noteLocalIds << noteModelItem->localId();

        QNTRACE(
            "view::NoteListView",
            "Included note local id " << noteModelItem->localId());
    }

    Q_UNUSED(noteLocalIds.removeDuplicates())

    QNTRACE(
        "view::NoteListView",
        "Selected note local ids: " << noteLocalIds.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalIds.isEmpty())) {
        QNDEBUG(
            "view::NoteListView",
            "Won't show the context menu: no notes are selected");
        return;
    }

    if (noteLocalIds.size() == 1) {
        showSingleNoteContextMenu(pos, globalPos, *noteModel);
    }
    else {
        showMultipleNotesContextMenu(globalPos, noteLocalIds);
    }
}

void NoteListView::showSingleNoteContextMenu(
    const QPoint & pos, const QPoint & globalPos, const NoteModel & noteModel)
{
    QNDEBUG("view::NoteListView", "NoteListView::showSingleNoteContextMenu");

    auto clickedItemIndex = indexAt(pos);
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::NoteListView",
            "Clicked item index is not valid, not doing anything");
        return;
    }

    const auto * item = noteModel.itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: no "
                       "item corresponding to the clicked item's index"));
        return;
    }

    if (!m_notebookItemView) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: no "
                       "notebook item view is set to the note list view "
                       "for notebooks reference"));
        return;
    }

    const auto * notebookModel =
        qobject_cast<const NotebookModel *>(m_notebookItemView->model());

    delete m_noteItemContextMenu;
    m_noteItemContextMenu = new QMenu(this);

    const NotebookItem * notebookItem = nullptr;
    if (notebookModel) {
        const auto notebookIndex =
            notebookModel->indexForLocalId(item->notebookLocalId());

        const auto * notebookModelItem =
            notebookModel->itemForIndex(notebookIndex);

        if (notebookModelItem) {
            notebookItem = notebookModelItem->cast<NotebookItem>();
        }
    }

    const bool canCreateNotes =
        (notebookItem ? notebookItem->canCreateNotes() : false);

    addContextMenuAction(
        tr("Create new note"), *m_noteItemContextMenu, this,
        [this] { onCreateNewNoteAction(); }, QString{},
        canCreateNotes ? ActionState::Enabled : ActionState::Disabled);

    const QString & noteLocalId = item->localId();

    addContextMenuAction(
        tr("Open in separate window"), *m_noteItemContextMenu, this,
        [this] { onOpenNoteInSeparateWindowAction(); }, noteLocalId,
        ActionState::Enabled);

    m_noteItemContextMenu->addSeparator();

    const bool canUpdateNotes =
        (notebookItem ? notebookItem->canUpdateNotes() : false);

    addContextMenuAction(
        tr("Delete"), *m_noteItemContextMenu, this,
        [this] { onDeleteNoteAction(); }, noteLocalId,
        canUpdateNotes ? ActionState::Enabled : ActionState::Disabled);

    addContextMenuAction(
        tr("Edit") + QStringLiteral("..."), *m_noteItemContextMenu, this,
        [this] { onEditNoteAction(); }, noteLocalId,
        canUpdateNotes ? ActionState::Enabled : ActionState::Disabled);

    if (notebookModel && notebookItem &&
        notebookItem->linkedNotebookGuid().isEmpty() && canUpdateNotes)
    {
        QStringList otherNotebookNames = notebookModel->notebookNames(
            NotebookModel::Filters(NotebookModel::Filter::CanCreateNotes));

        const QString & notebookName = notebookItem->name();

        const auto it = std::lower_bound(
            otherNotebookNames.constBegin(), otherNotebookNames.constEnd(),
            notebookName);

        if ((it != otherNotebookNames.constEnd()) && (*it == notebookName)) {
            const int offset = static_cast<int>(
                std::distance(otherNotebookNames.constBegin(), it));

            const auto nit = otherNotebookNames.begin() + offset;
            Q_UNUSED(otherNotebookNames.erase(nit))
        }

        // Need to filter out other notebooks which prohibit the creation of
        // notes in them as moving the note from one notebook to another
        // involves modifying the original notebook's note and the "creation"
        // of a note in another notebook
        for (auto it = otherNotebookNames.begin();
             it != otherNotebookNames.end();)
        {
            auto notebookItemIndex = notebookModel->indexForNotebookName(*it);
            if (Q_UNLIKELY(!notebookItemIndex.isValid())) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const auto * notebookModelItem =
                notebookModel->itemForIndex(notebookItemIndex);

            if (Q_UNLIKELY(!notebookModelItem)) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const auto * otherNotebookItem =
                notebookModelItem->cast<NotebookItem>();

            if (Q_UNLIKELY(!otherNotebookItem)) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            if (!otherNotebookItem->canCreateNotes()) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            ++it;
        }

        if (!otherNotebookNames.isEmpty()) {
            auto * targetNotebooksSubMenu =
                m_noteItemContextMenu->addMenu(tr("Move to notebook"));

            Q_ASSERT(targetNotebooksSubMenu);

            for (const auto & otherNotebookName:
                 std::as_const(otherNotebookNames))
            {
                QStringList dataPair;
                dataPair.reserve(2);
                dataPair << noteLocalId;
                dataPair << otherNotebookName;

                addContextMenuAction(
                    otherNotebookName, *targetNotebooksSubMenu, this,
                    [this] { onMoveToOtherNotebookAction(); }, dataPair,
                    ActionState::Enabled);
            }
        }
    }

    if (item->isFavorited()) {
        addContextMenuAction(
            tr("Unfavorite"), *m_noteItemContextMenu, this,
            [this] { onUnfavoriteAction(); }, noteLocalId,
            ActionState::Enabled);
    }
    else {
        addContextMenuAction(
            tr("Favorite"), *m_noteItemContextMenu, this,
            [this] { onFavoriteAction(); }, noteLocalId, ActionState::Enabled);
    }

    m_noteItemContextMenu->addSeparator();

    addContextMenuAction(
        tr("Export to enex") + QStringLiteral("..."), *m_noteItemContextMenu,
        this, [this] { onExportSingleNoteToEnexAction(); }, noteLocalId,
        ActionState::Enabled);

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_noteItemContextMenu, this,
        [this] { onShowNoteInfoAction(); }, noteLocalId, ActionState::Enabled);

    if (!item->guid().isEmpty()) {
        QStringList localIdAndGuid;
        localIdAndGuid.reserve(2);
        localIdAndGuid << noteLocalId;
        localIdAndGuid << item->guid();

        addContextMenuAction(
            tr("Copy in-app note link"), *m_noteItemContextMenu, this,
            [this] { onCopyInAppNoteLinkAction(); }, localIdAndGuid,
            ActionState::Enabled);
    }

    auto * thumbnailsSubMenu = m_noteItemContextMenu->addMenu(tr("Thumbnails"));

    Q_ASSERT(thumbnailsSubMenu);

    const QString showHideForAllNotes = m_showThumbnailsForAllNotes
        ? tr("Hide for all notes")
        : tr("Show for all notes");

    addContextMenuAction(
        showHideForAllNotes, *thumbnailsSubMenu, this,
        [this] { onToggleThumbnailPreference(); }, QString::fromUtf8(""),
        ActionState::Enabled);

    const auto & thumbnailData = item->thumbnailData();
    const bool hasThumbnail = !thumbnailData.isEmpty();
    const bool canToggleThumbnail = hasThumbnail && m_showThumbnailsForAllNotes;

    if (canToggleThumbnail) {
        const bool isHiddenForCurrentNote =
            m_hideThumbnailsLocalIds.contains(noteLocalId);

        const QString showHideForCurrent = isHiddenForCurrentNote
            ? tr("Show for current note")
            : tr("Hide for current note");

        addContextMenuAction(
            showHideForCurrent, *thumbnailsSubMenu, this,
            [this] { onToggleThumbnailPreference(); }, noteLocalId,
            ActionState::Enabled);
    }

    m_noteItemContextMenu->show();
    m_noteItemContextMenu->exec(globalPos);
}

void NoteListView::showMultipleNotesContextMenu(
    const QPoint & globalPos, const QStringList & noteLocalIds)
{
    QNDEBUG("view::NoteListView", "NoteListView::showMultipleNotesContextMenu");

    delete m_noteItemContextMenu;
    m_noteItemContextMenu = new QMenu(this);

    addContextMenuAction(
        tr("Export to enex") + QStringLiteral("..."), *m_noteItemContextMenu,
        this, [this] { onExportSeveralNotesToEnexAction(); }, noteLocalIds,
        ActionState::Enabled);

    m_noteItemContextMenu->show();
    m_noteItemContextMenu->exec(globalPos);
}

void NoteListView::currentChanged(
    const QModelIndex & current, const QModelIndex & previous)
{
    QNTRACE("view::NoteListView", "NoteListView::currentChanged");
    Q_UNUSED(previous)

    if (!current.isValid()) {
        QNTRACE("view::NoteListView", "Current index is invalid");
        return;
    }

    auto * noteModel = this->noteModel();
    if (Q_UNLIKELY(!noteModel)) {
        return;
    }

    const auto * item = noteModel->itemForIndex(current);
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't retrieve the new current "
                       "note item for note model index"));
        return;
    }

    m_lastCurrentNoteLocalId = item->localId();

    QNTRACE(
        "view::NoteListView",
        "Updated the last current note local id to " << item->localId());

    Q_EMIT currentNoteChanged(item->localId());
}

void NoteListView::mousePressEvent(QMouseEvent * event)
{
    if (event && (event->buttons() & Qt::RightButton) &&
        !(event->buttons() & Qt::LeftButton) &&
        !(event->buttons() & Qt::MiddleButton))
    {
        showContextMenuAtPoint(event->pos(), event->globalPos());
        return;
    }

    QListView::mousePressEvent(event);

    if (m_noteItemContextMenu && !m_noteItemContextMenu->isHidden()) {
        m_noteItemContextMenu->hide();
    }
}

const NotebookItem * NoteListView::currentNotebookItem()
{
    QNDEBUG("view::NoteListView", "NoteListView::currentNotebookItem");

    const auto currentNotebookItemIndex =
        m_notebookItemView->currentlySelectedItemIndex();

    if (Q_UNLIKELY(!currentNotebookItemIndex.isValid())) {
        QNDEBUG(
            "view::NoteListView",
            "No current notebook within the notebook view");
        return nullptr;
    }

    const auto * notebookModel =
        qobject_cast<const NotebookModel *>(m_notebookItemView->model());

    if (!notebookModel) {
        QNDEBUG(
            "view::NoteListView",
            "No notebook model is set to the notebook view");
        return nullptr;
    }

    const auto * notebookModelItem =
        notebookModel->itemForIndex(currentNotebookItemIndex);

    if (Q_UNLIKELY(!notebookModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't find the notebook model item corresponding "
                       "to the current item selected in the notebooks view"));
        return nullptr;
    }

    if (notebookModelItem->type() != INotebookModelItem::Type::Notebook) {
        QNDEBUG(
            "view::NoteListView",
            "Non-notebook item is selected within "
                << "the notebook item view");
        return nullptr;
    }

    const auto * notebookItem = notebookModelItem->cast<NotebookItem>();

    QNTRACE(
        "view::NoteListView",
        "Selected notebook item: "
            << (notebookItem ? notebookItem->toString()
                             : QStringLiteral("<null>")));

    return notebookItem;
}

NoteModel * NoteListView::noteModel() const
{
    auto * model = this->model();
    if (Q_UNLIKELY(!model)) {
        return nullptr;
    }

    auto * noteModel = qobject_cast<NoteModel *>(model);
    if (Q_UNLIKELY(!noteModel)) {
        QNERROR(
            "view::NoteListView",
            "Wrong model connected to the note list view");
        return nullptr;
    }

    return noteModel;
}

QVariant NoteListView::actionData()
{
    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast the slot invoker to QAction"))
        return {};
    }

    return action->data();
}

QString NoteListView::actionDataString()
{
    QVariant value = actionData();
    if (Q_UNLIKELY(!value.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast action string"))
        return {};
    }

    return value.toString();
}

QStringList NoteListView::actionDataStringList()
{
    QVariant value = actionData();
    if (Q_UNLIKELY(!value.isValid())) {
        return {};
    }

    return value.toStringList();
}

} // namespace quentier
