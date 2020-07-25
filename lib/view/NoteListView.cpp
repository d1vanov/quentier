/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include <lib/model/NoteModel.h>
#include <lib/model/notebook/NotebookItem.h>
#include <lib/model/notebook/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include <iterator>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:note", errorDescription);                              \
        Q_EMIT notifyError(errorDescription);                                  \
    }                                                                          \
// REPORT_ERROR

namespace quentier {

NoteListView::NoteListView(QWidget * parent) :
    QListView(parent)
{}

void NoteListView::setNotebookItemView(NotebookItemView * pNotebookItemView)
{
    QNTRACE("view:note", "NoteListView::setNotebookItemView");
    m_pNotebookItemView = pNotebookItemView;
}

void NoteListView::setAutoSelectNoteOnNextAddition()
{
    QNTRACE("view:note", "NoteListView::setAutoSelectNoteOnNextAddition");
    m_shouldSelectFirstNoteOnNextNoteAddition = true;
}

QStringList NoteListView::selectedNotesLocalUids() const
{
    QStringList result;

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return result;
    }

    auto indexes = selectedIndexes();
    result.reserve(indexes.size());

    for(const auto & modelIndex: qAsConst(indexes))
    {
        const auto * pItem = pNoteModel->itemForIndex(modelIndex);
        if (Q_UNLIKELY(!pItem)) {
            QNWARNING("view:note", "Found no note model item for selected "
                << "index");
            continue;
        }

        if (!result.contains(pItem->localUid())) {
            result.push_back(pItem->localUid());
        }
    }

    QNTRACE("view:note", "Local uids of selected notes: "
        << (result.isEmpty()
            ? QStringLiteral("<empty>")
            : result.join(QStringLiteral(", "))));

    return result;
}

QString NoteListView::currentNoteLocalUid() const
{
    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return {};
    }

    auto currentModelIndex = currentIndex();
    if (!currentModelIndex.isValid()) {
        QNDEBUG("view:note", "Note view has no valid current index");
        return {};
    }

    const auto * pItem = pNoteModel->itemForIndex(currentModelIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNWARNING("view:note", "Found no note model item for the current "
            << "index");
        return {};
    }

    return pItem->localUid();
}

const Account & NoteListView::currentAccount() const
{
    return m_currentAccount;
}

void NoteListView::setCurrentAccount(const Account & account)
{
    m_currentAccount = account;
}

void NoteListView::setCurrentNoteByLocalUid(QString noteLocalUid)
{
    QNTRACE("view:note", "NoteListView::setCurrentNoteByLocalUid: "
        << noteLocalUid);

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    auto index = pNoteModel->indexForLocalUid(noteLocalUid);
    setCurrentIndex(index);

    m_lastCurrentNoteLocalUid = noteLocalUid;

    QNDEBUG("view:note", "Updated last current note local uid to "
        << noteLocalUid);
}


void NoteListView::setShowNoteThumbnailsState(
    bool showThumbnailsForAllNotes,
    const QSet<QString> & hideThumbnailsLocalUids)
{
    QNDEBUG("view:note", "NoteListView::setShowNoteThumbnailsState");

    m_showThumbnailsForAllNotes = showThumbnailsForAllNotes;
    m_hideThumbnailsLocalUids = hideThumbnailsLocalUids;
}

void NoteListView::selectNotesByLocalUids(const QStringList & noteLocalUids)
{
    QNTRACE("view:note", "NoteListView::selectNotesByLocalUids: "
        << noteLocalUids.join(QStringLiteral(", ")));

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("view:note", "Can't select notes by local uids: no selection "
            << "model within the view");
        return;
    }

    QItemSelection selection;

    QItemSelectionModel::SelectionFlags selectionFlags(
        QItemSelectionModel::Select);

    for(const auto & noteLocalUid: qAsConst(noteLocalUids))
    {
        auto modelIndex = pNoteModel->indexForLocalUid(noteLocalUid);
        if (Q_UNLIKELY(!modelIndex.isValid())) {
            QNDEBUG("view:note", "The model index for note local uid "
                << noteLocalUid << " is invalid, skipping");
            continue;
        }

        QItemSelection currentSelection;
        currentSelection.select(modelIndex, modelIndex);
        selection.merge(currentSelection, selectionFlags);
    }

    selectionFlags = QItemSelectionModel::SelectionFlags(
        QItemSelectionModel::ClearAndSelect);

    pSelectionModel->select(selection, selectionFlags);
}

void NoteListView::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QListView::dataChanged(topLeft, bottomRight, roles);
}

void NoteListView::rowsAboutToBeRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNTRACE("view:note", "NoteListView::rowsAboutToBeRemoved: start = "
        << start << ", end = " << end);

    QModelIndex current = currentIndex();
    if (current.isValid())
    {
        int row = current.row();
        if ((row >= start) && (row <= end))
        {
            // The default implementation moves current to the next remaining
            // item but for this view it makes more sense to just get rid of
            // the item altogether
            setCurrentIndex(QModelIndex());

            // Also note that the last current note local uid is not changed
            // here: while the row with it would be removed shortly, the row
            // removal might correspond to the re-sorting or filtering of note
            // model items so the row corresponding to this note might become
            // available again before some other note item is selected manually.
            // To account for this, leaving the last current note local uid
            // untouched for now.
        }
    }

    QListView::rowsAboutToBeRemoved(parent, start, end);
}

void NoteListView::rowsInserted(const QModelIndex & parent, int start, int end)
{
    QNTRACE("view:note", "NoteListView::rowsInserted: start = " << start
        << ", end = " << end);

    QListView::rowsInserted(parent, start, end);

    if (Q_UNLIKELY(m_shouldSelectFirstNoteOnNextNoteAddition))
    {
        m_shouldSelectFirstNoteOnNextNoteAddition = false;

        // NOTE: for some reason it's not safe to set the current index right
        // here right now: sometimes things crash somewhere in the gory guts
        // of Qt. So scheduling a separate callback as a workaround
        QTimer * pTimer = new QTimer(this);
        pTimer->setSingleShot(true);

        QObject::connect(
            pTimer,
            &QTimer::timeout,
            this,
            &NoteListView::onSelectFirstNoteEvent);

        pTimer->start(0);
        return;
    }

    // Need to check if the row inserting made the last current note local uid
    // available
    auto currentModelIndex = currentIndex();
    if (currentModelIndex.isValid()) {
        QNTRACE("view:note", "Current model index is already valid, no need "
            << "to restore anything");
        return;
    }

    if (m_lastCurrentNoteLocalUid.isEmpty()) {
        QNTRACE("view:note", "The last current note local uid is empty, "
            << "no current note item to restore");
        return;
    }

    QNDEBUG("view:note", "Should try to re-select the last current note local "
        << "uid");

    // As above, it is not safe to attempt to change the current model index
    // right inside this callback so scheduling a separate one
    QTimer * pTimer = new QTimer(this);
    pTimer->setSingleShot(true);

    QObject::connect(
        pTimer,
        &QTimer::timeout,
        this,
        &NoteListView::onTrySetLastCurrentNoteByLocalUidEvent);

    pTimer->start(0);
    return;
}

void NoteListView::onCreateNewNoteAction()
{
    QNDEBUG("view:note", "NoteListView::onCreateNewNoteAction");

    Q_EMIT newNoteCreationRequested();
}

void NoteListView::onDeleteNoteAction()
{
    QNDEBUG("view:note", "NoteListView::onDeleteNoteAction");

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    ErrorString error;
    bool res = pNoteModel->deleteNote(noteLocalUid, error);
    if (!res) {
        ErrorString errorDescription(QT_TR_NOOP("Can't delete note: "));
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view:note", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }
}

void NoteListView::onEditNoteAction()
{
    QNDEBUG("view:note", "NoteListView::onEditNoteAction");

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    Q_EMIT editNoteDialogRequested(noteLocalUid);
}

void NoteListView::onMoveToOtherNotebookAction()
{
    QNTRACE("view:note", "NoteListView::onMoveToOtherNotebookAction");

    QStringList actionData = actionDataStringList();
    if (actionData.isEmpty() || (actionData.size() != 2)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move note to another notebook: internal "
                       "error, wrong action data"));
        return;
    }

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    QString noteLocalUid = actionData[0];
    QString notebookName = actionData[1];

    ErrorString error;
    bool res = pNoteModel->moveNoteToNotebook(
        noteLocalUid,
        notebookName,
        error);

    if (!res)
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't note to another notebookL "));

        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view:note", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onOpenNoteInSeparateWindowAction()
{
    QNDEBUG("view:note", "NoteListView::onOpenNoteInSeparateWindowAction");

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    Q_EMIT openNoteInSeparateWindowRequested(noteLocalUid);
}

void NoteListView::onUnfavoriteAction()
{
    QNDEBUG("view:note", "NoteListView::onUnfavoriteAction");

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    ErrorString error;
    if (!pNoteModel->unfavoriteNote(noteLocalUid, error)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't unfavorite note: "));
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view:note", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onFavoriteAction()
{
    QNDEBUG("view:note", "NoteListView::onFavoriteAction");

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    ErrorString error;
    if (!pNoteModel->favoriteNote(noteLocalUid, error)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't favorite note: "));
        errorDescription.appendBase(error.base());
        errorDescription.appendBase(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING("view:note", errorDescription);
        Q_EMIT notifyError(errorDescription);
    }
}

void NoteListView::onShowNoteInfoAction()
{
    QNDEBUG("view:note", "NoteListView::onShowNoteInfoAction");

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }
    Q_EMIT noteInfoDialogRequested(noteLocalUid);
}

void NoteListView::onToggleThumbnailPreference()
{
    QNDEBUG("view:note", "NoteListView::onToggleThumbnailPreference");

    // note here empty string is OK (means all notes)
    const QString noteLocalUid = actionDataString();
    Q_EMIT toggleThumbnailsPreference(noteLocalUid);
}

void NoteListView::onCopyInAppNoteLinkAction()
{
    QNDEBUG("view:note", "NoteListView::onCopyInAppNoteLinkAction");

    auto noteLocalUidAndGuid = actionDataStringList();
    if (Q_UNLIKELY(noteLocalUidAndGuid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't copy in-app note link: internal error, "
                       "no note local uid and guid were passed to "
                       "the action handler"));
        return;
    }

    if (Q_UNLIKELY(noteLocalUidAndGuid.size() != 2)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't copy in-app note link: internal error, "
                       "unidentified data received instead of note "
                       "local uid and guid"));
        return;
    }

    QNTRACE("view:note", "Requesting copy in app note link for note model "
        << "item: local uid = " << noteLocalUidAndGuid.at(0)
        << ", guid = " << noteLocalUidAndGuid.at(1));

    Q_EMIT copyInAppNoteLinkRequested(
        noteLocalUidAndGuid.at(0),
        noteLocalUidAndGuid.at(1));
}

void NoteListView::onExportSingleNoteToEnexAction()
{
    QNDEBUG("view:note", "NoteListView::onExportSingleNoteToEnexAction");

    const QString noteLocalUid = actionDataString();
    if (noteLocalUid.isEmpty()) {
        return;
    }

    QStringList noteLocalUids;
    noteLocalUids << noteLocalUid;
    Q_EMIT enexExportRequested(noteLocalUids);
}

void NoteListView::onExportSeveralNotesToEnexAction()
{
    QNDEBUG("view:note", "NoteListView::onExportSeveralNotesToEnexAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't export notes to ENEX: internal error, "
                       "can't cast the slot invoker to QAction"));
        return;
    }

    auto noteLocalUids = pAction->data().toStringList();
    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't export note to ENEX: internal error, the list of "
                       "local uids of notes to be exported is empty"));
        return;
    }

    Q_EMIT enexExportRequested(noteLocalUids);
}

void NoteListView::onSelectFirstNoteEvent()
{
    QNDEBUG("view:note", "NoteListView::onSelectFirstNoteEvent");

    const auto * pModel = model();
    if (Q_UNLIKELY(!pModel)) {
        QNERROR("view:note", "No model is set to note list view on select "
            << "first note event");
        return;
    }

    setCurrentIndex(pModel->index(0, NoteModel::Columns::Title, QModelIndex()));
}

void NoteListView::onTrySetLastCurrentNoteByLocalUidEvent()
{
    QNDEBUG(
        "view:note",
        "NoteListView::onTrySetLastCurrentNoteByLocalUidEvent");

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    auto modelIndex = pNoteModel->indexForLocalUid(m_lastCurrentNoteLocalUid);
    if (!modelIndex.isValid()) {
        QNTRACE("view:note", "No valid model index within the note model for "
                << "the last current note local uid "
                << m_lastCurrentNoteLocalUid);
        return;
    }

    setCurrentIndex(modelIndex);
}

void NoteListView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:note", "NoteListView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("view:note", "Detected Qt error: note list view received "
            << "context menu event with null pointer to the context menu "
            << "event");
        return;
    }

    showContextMenuAtPoint(pEvent->pos(), pEvent->globalPos());
}

void NoteListView::showContextMenuAtPoint(
    const QPoint & pos, const QPoint & globalPos)
{
    QNDEBUG("view:note", "NoteListView::showContextMenuAtPoint");

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: "
                       "can't get the selection model from the view"));
        return;
    }

    auto selectedRowIndexes = pSelectionModel->selectedIndexes();
    selectedRowIndexes << currentIndex();

    QStringList noteLocalUids;
    noteLocalUids.reserve(selectedRowIndexes.size());

    for(const auto & modelIndex: qAsConst(selectedRowIndexes))
    {
        const auto * pNoteModelItem = pNoteModel->itemForIndex(modelIndex);
        if (Q_UNLIKELY(!pNoteModelItem)) {
            QNWARNING("view:note", "Detected selected model index for which "
                << "no model item was found");
            continue;
        }

        noteLocalUids << pNoteModelItem->localUid();

        QNTRACE("view:note", "Included note local uid "
            << pNoteModelItem->localUid());
    }

    Q_UNUSED(noteLocalUids.removeDuplicates())

    QNTRACE("view:note", "Selected note local uids: "
        << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        QNDEBUG("view:note", "Won't show the context menu: no notes are "
            << "selected");
        return;
    }

    if (noteLocalUids.size() == 1) {
        showSingleNoteContextMenu(pos, globalPos, *pNoteModel);
    }
    else {
        showMultipleNotesContextMenu(globalPos, noteLocalUids);
    }
}

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction,                                                           \
            &QAction::triggered,                                               \
            this,                                                              \
            &NoteListView::slot);                                              \
        menu->addAction(pAction);                                              \
    }                                                                          \
// ADD_CONTEXT_MENU_ACTION

void NoteListView::showSingleNoteContextMenu(
    const QPoint & pos, const QPoint & globalPos, const NoteModel & noteModel)
{
    QNDEBUG("view:note", "NoteListView::showSingleNoteContextMenu");

    auto clickedItemIndex = indexAt(pos);
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG("view:note", "Clicked item index is not valid, not doing "
            << "anything");
        return;
    }

    const auto * pItem = noteModel.itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: no "
                       "item corresponding to the clicked item's index"));
        return;
    }

    if (!m_pNotebookItemView) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the note item's context menu: no "
                       "notebook item view is set to the note list view "
                       "for notebooks reference"));
        return;
    }

    const auto * pNotebookModel = qobject_cast<const NotebookModel*>(
        m_pNotebookItemView->model());

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);

    const NotebookItem * pNotebookItem = nullptr;
    if (pNotebookModel)
    {
        auto notebookIndex = pNotebookModel->indexForLocalUid(
            pItem->notebookLocalUid());

        const auto * pNotebookModelItem = pNotebookModel->itemForIndex(
            notebookIndex);

        if (pNotebookModelItem) {
            pNotebookItem = pNotebookModelItem->cast<NotebookItem>();
        }
    }

    bool canCreateNotes =
        (pNotebookItem ? pNotebookItem->canCreateNotes() : false);

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new note"),
        m_pNoteItemContextMenu,
        onCreateNewNoteAction,
        QString(),
        canCreateNotes);

    const QString & noteLocalUid = pItem->localUid();

    ADD_CONTEXT_MENU_ACTION(
        tr("Open in separate window"),
        m_pNoteItemContextMenu,
        onOpenNoteInSeparateWindowAction,
        noteLocalUid,
        true);

    m_pNoteItemContextMenu->addSeparator();

    bool canUpdateNotes =
        (pNotebookItem ? pNotebookItem->canUpdateNotes() : false);

    ADD_CONTEXT_MENU_ACTION(
        tr("Delete"),
        m_pNoteItemContextMenu,
        onDeleteNoteAction,
        noteLocalUid,
        canUpdateNotes);

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."),
        m_pNoteItemContextMenu,
        onEditNoteAction,
        noteLocalUid,
        canUpdateNotes);

    if (pNotebookModel && pNotebookItem &&
        pNotebookItem->linkedNotebookGuid().isEmpty() && canUpdateNotes)
    {
        QStringList otherNotebookNames = pNotebookModel->notebookNames(
            NotebookModel::Filters(NotebookModel::Filter::CanCreateNotes));

        const QString & notebookName = pNotebookItem->name();

        auto it = std::lower_bound(
            otherNotebookNames.constBegin(),
            otherNotebookNames.constEnd(),
            notebookName);

        if ((it != otherNotebookNames.constEnd()) && (*it == notebookName))
        {
            int offset = static_cast<int>(
                std::distance(otherNotebookNames.constBegin(), it));

            auto nit = otherNotebookNames.begin() + offset;
            Q_UNUSED(otherNotebookNames.erase(nit))
        }

        // Need to filter out other notebooks which prohibit the creation of
        // notes in them as moving the note from one notebook to another
        // involves modifying the original notebook's note and the "creation"
        // of a note in another notebook
        for(auto it = otherNotebookNames.begin();
            it != otherNotebookNames.end(); )
        {
            auto notebookItemIndex = pNotebookModel->indexForNotebookName(*it);
            if (Q_UNLIKELY(!notebookItemIndex.isValid())) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const auto * pNotebookModelItem = pNotebookModel->itemForIndex(
                notebookItemIndex);

            if (Q_UNLIKELY(!pNotebookModelItem)) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const auto * pOtherNotebookItem =
                pNotebookModelItem->cast<NotebookItem>();

            if (Q_UNLIKELY(!pOtherNotebookItem)) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            if (!pOtherNotebookItem->canCreateNotes()) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            ++it;
        }

        if (!otherNotebookNames.isEmpty())
        {
            auto * pTargetNotebooksSubMenu = m_pNoteItemContextMenu->addMenu(
                tr("Move to notebook"));

            for(const auto & otherNotebookName: qAsConst(otherNotebookNames))
            {
                QStringList dataPair;
                dataPair.reserve(2);
                dataPair << noteLocalUid;
                dataPair << otherNotebookName;

                ADD_CONTEXT_MENU_ACTION(
                    otherNotebookName,
                    pTargetNotebooksSubMenu,
                    onMoveToOtherNotebookAction,
                    dataPair,
                    true);
            }
        }
    }

    if (pItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"),
            m_pNoteItemContextMenu,
            onUnfavoriteAction,
            noteLocalUid,
            true);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"),
            m_pNoteItemContextMenu,
            onFavoriteAction,
            noteLocalUid,
            true);
    }

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Export to enex") + QStringLiteral("..."),
        m_pNoteItemContextMenu,
        onExportSingleNoteToEnexAction,
        noteLocalUid,
        true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pNoteItemContextMenu,
        onShowNoteInfoAction,
        noteLocalUid,
        true);

    if (!pItem->guid().isEmpty())
    {
        QStringList localUidAndGuid;
        localUidAndGuid.reserve(2);
        localUidAndGuid << noteLocalUid;
        localUidAndGuid << pItem->guid();

        ADD_CONTEXT_MENU_ACTION(
            tr("Copy in-app note link"),
            m_pNoteItemContextMenu,
            onCopyInAppNoteLinkAction,
            localUidAndGuid,
            true);
    }

    auto * pThumbnailsSubMenu = m_pNoteItemContextMenu->addMenu(
        tr("Thumbnails"));

    QString showHideForAllNotes =
        m_showThumbnailsForAllNotes
        ? tr("Hide for all notes")
        : tr("Show for all notes");

    ADD_CONTEXT_MENU_ACTION(
        showHideForAllNotes,
        pThumbnailsSubMenu,
        onToggleThumbnailPreference,
        QVariant::fromValue(QString::fromUtf8("")),
        true);

    const auto & thumbnailData = pItem->thumbnailData();
    bool hasThumbnail = !thumbnailData.isEmpty();
    bool canToggleThumbnail = hasThumbnail && m_showThumbnailsForAllNotes;

    if (canToggleThumbnail)
    {
        bool isHiddenForCurrentNote = m_hideThumbnailsLocalUids.contains(
            noteLocalUid);

        QString showHideForCurrent =
            isHiddenForCurrentNote
            ? tr("Show for current note")
            : tr("Hide for current note");

        ADD_CONTEXT_MENU_ACTION(
            showHideForCurrent,
            pThumbnailsSubMenu,
            onToggleThumbnailPreference,
            noteLocalUid,
            true);
    }

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(globalPos);
}

void NoteListView::showMultipleNotesContextMenu(
    const QPoint & globalPos, const QStringList & noteLocalUids)
{
    QNDEBUG("view:note", "NoteListView::showMultipleNotesContextMenu");

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);

    ADD_CONTEXT_MENU_ACTION(
        tr("Export to enex") + QStringLiteral("..."),
        m_pNoteItemContextMenu,
        onExportSeveralNotesToEnexAction,
        noteLocalUids,
        true);

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(globalPos);
}

void NoteListView::currentChanged(
    const QModelIndex & current, const QModelIndex & previous)
{
    QNTRACE("view:note", "NoteListView::currentChanged");
    Q_UNUSED(previous)

    if (!current.isValid()) {
        QNTRACE("view:note", "Current index is invalid");
        return;
    }

    auto * pNoteModel = noteModel();
    if (Q_UNLIKELY(!pNoteModel)) {
        return;
    }

    const auto * pItem = pNoteModel->itemForIndex(current);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't retrieve the new current "
                       "note item for note model index"));
        return;
    }

    m_lastCurrentNoteLocalUid = pItem->localUid();

    QNTRACE("view:note", "Updated the last current note local uid to "
        << pItem->localUid());

    Q_EMIT currentNoteChanged(pItem->localUid());
}

void NoteListView::mousePressEvent(QMouseEvent * pEvent)
{
    if (pEvent &&
        (pEvent->buttons() & Qt::RightButton) &&
        !(pEvent->buttons() & Qt::LeftButton) &&
        !(pEvent->buttons() & Qt::MidButton) )
    {
        showContextMenuAtPoint(pEvent->pos(), pEvent->globalPos());
        return;
    }

    QListView::mousePressEvent(pEvent);

    if (m_pNoteItemContextMenu && !m_pNoteItemContextMenu->isHidden()) {
        m_pNoteItemContextMenu->hide();
    }
}

const NotebookItem * NoteListView::currentNotebookItem()
{
    QNDEBUG("view:note", "NoteListView::currentNotebookItem");

    auto currentNotebookItemIndex =
        m_pNotebookItemView->currentlySelectedItemIndex();

    if (Q_UNLIKELY(!currentNotebookItemIndex.isValid())) {
        QNDEBUG("view:note", "No current notebook within the notebook view");
        return nullptr;
    }

    const auto * pNotebookModel = qobject_cast<const NotebookModel*>(
        m_pNotebookItemView->model());

    if (!pNotebookModel) {
        QNDEBUG("view:note", "No notebook model is set to the notebook view");
        return nullptr;
    }

    const auto * pNotebookModelItem = pNotebookModel->itemForIndex(
        currentNotebookItemIndex);

    if (Q_UNLIKELY(!pNotebookModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't find the notebook model item corresponding "
                       "to the current item selected in the notebooks view"));
        return nullptr;
    }

    if (pNotebookModelItem->type() != INotebookModelItem::Type::Notebook) {
        QNDEBUG("view:note", "Non-notebook item is selected within "
            << "the notebook item view");
        return nullptr;
    }

    const auto * pNotebookItem = pNotebookModelItem->cast<NotebookItem>();

    QNTRACE("view:note", "Selected notebook item: "
        << (pNotebookItem
            ? pNotebookItem->toString()
            : QStringLiteral("<null>")));

    return pNotebookItem;
}

NoteModel * NoteListView::noteModel() const
{
    auto * pModel = model();
    if (Q_UNLIKELY(!pModel)) {
        return nullptr;
    }

    auto * pNoteModel = qobject_cast<NoteModel *>(pModel);
    if (Q_UNLIKELY(!pNoteModel)) {
        QNERROR("view:note", "Wrong model connected to the note list view");
        return nullptr;
    }

    return pNoteModel;
}

QVariant NoteListView::actionData()
{
    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't cast the slot invoker to QAction"))
        return {};
    }

    return pAction->data();
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
