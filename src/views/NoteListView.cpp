/*
 * Copyright 2017 Dmitry Ivanov
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
#include "../models/NoteFilterModel.h"
#include "../models/NoteModel.h"
#include "../models/NotebookModel.h"
#include "../models/NotebookItem.h"
#include <quentier/logging/QuentierLogger.h>
#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <iterator>

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        Q_EMIT notifyError(errorDescription); \
    }

namespace quentier {

NoteListView::NoteListView(QWidget * parent) :
    QListView(parent),
    m_pNoteItemContextMenu(Q_NULLPTR),
    m_pNotebookItemView(Q_NULLPTR),
    m_shouldSelectFirstNoteOnNextNoteAddition(false),
    m_currentAccount(),
    m_lastCurrentNoteLocalUid()
{}

void NoteListView::setNotebookItemView(NotebookItemView * pNotebookItemView)
{
    QNDEBUG(QStringLiteral("NoteListView::setNotebookItemView"));
    m_pNotebookItemView = pNotebookItemView;
}

void NoteListView::setAutoSelectNoteOnNextAddition()
{
    QNDEBUG(QStringLiteral("NoteListView::setAutoSelectNoteOnNextAddition"));
    m_shouldSelectFirstNoteOnNextNoteAddition = true;
}

QStringList NoteListView::selectedNotesLocalUids() const
{
    QStringList result;

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNDEBUG(QStringLiteral("Can't return the list of selected note local uids: "
                               "wrong model connected to the note list view"));
        return result;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Can't return the list of selected note local uids: can't get the source model "
                               "from the note filter model connected to the note list view"));
        return result;
    }

    QModelIndexList indexes = selectedIndexes();
    result.reserve(indexes.size());

    for(auto it = indexes.constBegin(), end = indexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & filterModelIndex = *it;
        QModelIndex sourceModelIndex = pNoteFilterModel->mapToSource(filterModelIndex);
        const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceModelIndex);
        if (Q_UNLIKELY(!pItem)) {
            QNWARNING(QStringLiteral("Found no note model item for selected index mapped from filter model to source model"));
            continue;
        }

        if (!result.contains(pItem->localUid())) {
            result.push_back(pItem->localUid());
        }
    }

    QNTRACE(QStringLiteral("Local uids of selected notes: ")
            << (result.isEmpty() ? QStringLiteral("<empty>") : result.join(QStringLiteral(", "))));
    return result;
}

QString NoteListView::currentNoteLocalUid() const
{
    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNDEBUG(QStringLiteral("Can't return the current note local uid: "
                               "wrong model connected to the note list view"));
        return QString();
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Can't return the current note local uid: can't get the source model "
                               "from the note filter model connected to the note list view"));
        return QString();
    }

    QModelIndex currentFilterModelIndex = currentIndex();
    if (!currentFilterModelIndex.isValid()) {
        QNDEBUG(QStringLiteral("Note view has no valid current index"));
        return QString();
    }

    QModelIndex currentSourceModelIndex = pNoteFilterModel->mapToSource(currentFilterModelIndex);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(currentSourceModelIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNWARNING(QStringLiteral("Found no note model item for the current index mapped from filter model to source model"));
        return QString();
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
    QNDEBUG(QStringLiteral("NoteListView::setCurrentNoteByLocalUid: ") << noteLocalUid);

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNDEBUG(QStringLiteral("Can't react on the change of current note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Can't react on the change: can't get the source model from the note filter model "
                               "connected to the note list view"));
        return;
    }

    QModelIndex index = pNoteModel->indexForLocalUid(noteLocalUid);
    if (index.isValid()) {
        index = pNoteFilterModel->mapFromSource(index);
    }

    setCurrentIndex(index);

    m_lastCurrentNoteLocalUid = noteLocalUid;
    QNDEBUG(QStringLiteral("Updated last current note local uid to ") << noteLocalUid);
}


void NoteListView::setShowNoteThumbnailsState(bool showThumbnailsForAllNotes, const QSet<QString> & hideThumbnailsLocalUids)
{
    QNDEBUG(QStringLiteral("NoteListView::setShowNoteThumbnailsState: showThumbnailsForAllNotes=")
                << showThumbnailsForAllNotes << QStringLiteral(", ")
                << hideThumbnailsLocalUids.values().join(QStringLiteral(", ")));

    m_showThumbnailsForAllNotes = showThumbnailsForAllNotes;
    m_hideThumbnailsLocalUids = hideThumbnailsLocalUids;
}

void NoteListView::selectNotesByLocalUids(const QStringList & noteLocalUids)
{
    QNDEBUG(QStringLiteral("NoteListView::selectNotesByLocalUids: ") << noteLocalUids.join(QStringLiteral(", ")));

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNDEBUG(QStringLiteral("Can't select notes by local uids: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Can't select notes by local uids: can't get the source model from the note filter model "
                               "connected to the note list view"));
        return;
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG(QStringLiteral("Can't select notes by local uids: no selection model within the view"));
        return;
    }

    QItemSelection selection;
    QItemSelectionModel::SelectionFlags selectionFlags(QItemSelectionModel::Select);

    for(auto it = noteLocalUids.constBegin(), end = noteLocalUids.constEnd(); it != end; ++it)
    {
        QModelIndex sourceModelIndex = pNoteModel->indexForLocalUid(*it);
        if (Q_UNLIKELY(!sourceModelIndex.isValid())) {
            QNDEBUG(QStringLiteral("The source model index for note local uid ") << *it
                    << QStringLiteral(" is invalid, skipping"));
            continue;
        }

        QModelIndex filterModelIndex = pNoteFilterModel->mapFromSource(sourceModelIndex);
        if (!filterModelIndex.isValid()) {
            QNDEBUG(QStringLiteral("It looks like note with local uid ") << *it
                    << QStringLiteral(" is filtered out by the note filter model because the index in the note filter model is invalid"));
            continue;
        }

        QItemSelection currentSelection;
        currentSelection.select(filterModelIndex, filterModelIndex);
        selection.merge(currentSelection, selectionFlags);
    }

    selectionFlags = QItemSelectionModel::SelectionFlags(QItemSelectionModel::ClearAndSelect);
    pSelectionModel->select(selection, selectionFlags);
}

void NoteListView::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               )
#else
                               , const QVector<int> & roles)
#endif
{
    QListView::dataChanged(topLeft, bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                               );
#else
                               , roles);
#endif
}

void NoteListView::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("NoteListView::rowsAboutToBeRemoved: start = ") << start << QStringLiteral(", end = ") << end);

    QModelIndex current = currentIndex();
    if (current.isValid())
    {
        int row = current.row();
        if ((row >= start) && (row <= end))
        {
            // The default implementation moves current to the next remaining item
            // but for this view it makes more sense to just get rid of the item altogether
            setCurrentIndex(QModelIndex());

            // Also note that the last current note local uid is not changed here: while the row with it
            // would be removed shortly, the row removal might correspond to the re-sorting or filtering of note
            // model items so the row corresponding to this note might become available again before some other note
            // item is selected manually. To account for this, leaving the last current note local uid untouched for now.
        }
    }

    QListView::rowsAboutToBeRemoved(parent, start, end);
}

void NoteListView::rowsInserted(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("NoteListView::rowsInserted: start = ") << start << QStringLiteral(", end = ") << end);

    QListView::rowsInserted(parent, start, end);

    if (Q_UNLIKELY(m_shouldSelectFirstNoteOnNextNoteAddition))
    {
        m_shouldSelectFirstNoteOnNextNoteAddition = false;

        // NOTE: for some reason it's not safe to set the current index right here right now:
        // sometimes things crash somewhere in the gory guts of Qt. So scheduling a separate callback
        // as a workaround
        QTimer * pTimer = new QTimer(this);
        pTimer->setSingleShot(true);
        QObject::connect(pTimer, QNSIGNAL(QTimer,timeout),
                         this, QNSLOT(NoteListView,onSelectFirstNoteEvent));
        pTimer->start(0);
        return;
    }

    // Need to check if the row inserting made the last current note local uid available
    QModelIndex currentModelIndex = currentIndex();
    if (currentModelIndex.isValid()) {
        QNTRACE(QStringLiteral("Current model index is already valid, no need to restore anything"));
        return;
    }

    if (m_lastCurrentNoteLocalUid.isEmpty()) {
        QNTRACE(QStringLiteral("The last current note local uid is empty, no current note item to restore"));
        return;
    }

    QNDEBUG(QStringLiteral("Should try to re-select the last current note local uid"));

    // As above, it is not safe to attempt to change the current model index right inside this callback
    // so scheduling a separate one
    QTimer * pTimer = new QTimer(this);
    pTimer->setSingleShot(true);
    QObject::connect(pTimer, QNSIGNAL(QTimer,timeout),
                     this, QNSLOT(NoteListView,onTrySetLastCurrentNoteByLocalUidEvent));
    pTimer->start(0);
    return;
}

void NoteListView::onCreateNewNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onCreateNewNoteAction"));

    Q_EMIT newNoteCreationRequested();
}

void NoteListView::onDeleteNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onDeleteNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't delete note, "
                                "can't cast the slot invoker to QAction"))
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't delete note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't delete note: can't get the source model from the note filter model connected to the note list view"));
        return;
    }

    bool res = pNoteModel->deleteNote(pAction->data().toString());
    if (!res) {
        REPORT_ERROR(QT_TR_NOOP("Can't delete note: can't find the item to be deleted within the model"));
        return;
    }
}

void NoteListView::onEditNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onEditNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't edit note: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't edit note: internal error, the local uid of the note to be edited is empty"));
        return;
    }

    Q_EMIT editNoteDialogRequested(noteLocalUid);
}

void NoteListView::onMoveToOtherNotebookAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onMoveToOtherNotebookAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move note to another notebook: internal error, "
                                "can't cast the slot invoker to QAction"));
        return;
    }

    QStringList actionData = pAction->data().toStringList();
    if (actionData.isEmpty() || (actionData.size() != 2)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move note to another notebook: internal error, "
                                "wrong action data"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move note to another notebook: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move note to another notebook: can't get the source model "
                                "from the note filter model connected to the note list view"));
        return;
    }

    QString noteLocalUid = actionData[0];
    QString notebookName = actionData[1];

    pNoteModel->moveNoteToNotebook(noteLocalUid, notebookName);
}

void NoteListView::onOpenNoteInSeparateWindowAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onOpenNoteInSeparateWindowAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't open note in a separate window: internal error, "
                                "can't cast the slot invoker to QAction"));
        return;
    }

    Q_EMIT openNoteInSeparateWindowRequested(pAction->data().toString());
}

void NoteListView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onUnfavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't unfavorite note: internal error, "
                                "can't cast the slot invoker to QAction"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't unfavorite note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't unfavorite note: can't get the source model from the note filter model "
                                "connected to the note list view"));
        return;
    }

    pNoteModel->unfavoriteNote(pAction->data().toString());
}

void NoteListView::onFavoriteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onFavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't favorite note: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't favorite note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't favorite note: can't get the source model from the note filter model "
                                "connected to the note list view"));
        return;
    }

    pNoteModel->favoriteNote(pAction->data().toString());
}

void NoteListView::onShowNoteInfoAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onShowNoteInfoAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show note info: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't show note info: internal error, the local uid of the note to be edited is empty"));
        return;
    }

    Q_EMIT noteInfoDialogRequested(noteLocalUid);
}


void NoteListView::onToggleThumbnailPreference() {
    QNDEBUG(QStringLiteral("NoteListView::onToggleThumbnailPreference noteLocalUid="));
    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't get data"));
        return;
    }
    QString noteLocalUid = pAction->data().toString();
    Q_EMIT toggleThumbnailsPreference(noteLocalUid);
}

void NoteListView::onCopyInAppNoteLinkAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onCopyInAppNoteLinkAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't copy in-app note link: internal error, "
                                "can't cast the slot invoker to QAction"));
        return;
    }

    QStringList noteLocalUidAndGuid = pAction->data().toStringList();
    if (Q_UNLIKELY(noteLocalUidAndGuid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't copy in-app note link: internal error, "
                                "no note local uid and guid were passed to the action handler"));
        return;
    }

    if (Q_UNLIKELY(noteLocalUidAndGuid.size() != 2)) {
        REPORT_ERROR(QT_TR_NOOP("Can't copy in-app note link: internal error, "
                                "unidentified data received instead of note local uid and guid"));
        return;
    }

    QNTRACE(QStringLiteral("Requesting copy in app note link for note model item: local uid = ")
            << noteLocalUidAndGuid.at(0) << QStringLiteral(", guid = ") << noteLocalUidAndGuid.at(1));
    Q_EMIT copyInAppNoteLinkRequested(noteLocalUidAndGuid.at(0), noteLocalUidAndGuid.at(1));
}

void NoteListView::onExportSingleNoteToEnexAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onExportSingleNoteToEnexAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't export note to ENEX: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't export note to ENEX: internal error, "
                                "the local uid of the note to be exported is empty"));
        return;
    }

    QStringList noteLocalUids;
    noteLocalUids << noteLocalUid;
    Q_EMIT enexExportRequested(noteLocalUids);
}

void NoteListView::onExportSeveralNotesToEnexAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onExportSeveralNotesToEnexAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TR_NOOP("Can't export notes to ENEX: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    QStringList noteLocalUids = pAction->data().toStringList();
    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        REPORT_ERROR(QT_TR_NOOP("Can't export note to ENEX: internal error, "
                                "the list of local uids of notes to be exported is empty"));
        return;
    }

    Q_EMIT enexExportRequested(noteLocalUids);
}

void NoteListView::onSelectFirstNoteEvent()
{
    QNDEBUG(QStringLiteral("NoteListView::onSelectFirstNoteEvent"));

    const QAbstractItemModel * pModel = model();
    if (Q_UNLIKELY(!pModel)) {
        QNDEBUG(QStringLiteral("No model"));
        return;
    }

    setCurrentIndex(pModel->index(0, NoteModel::Columns::Title, QModelIndex()));
}

void NoteListView::onTrySetLastCurrentNoteByLocalUidEvent()
{
    QNDEBUG(QStringLiteral("NoteListView::onTrySetLastCurrentNoteByLocalUidEvent"));

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNDEBUG(QStringLiteral("Can't restore the last current note local uid: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Can't restore the last current note local uid: can't get the source model from the note "
                               "filter model connected to the note list view"));
        return;
    }

    QModelIndex sourceModelIndex = pNoteModel->indexForLocalUid(m_lastCurrentNoteLocalUid);
    if (!sourceModelIndex.isValid()) {
        QNTRACE(QStringLiteral("No valid model index within the note model for the last current note local uid ")
                << m_lastCurrentNoteLocalUid);
        return;
    }

    QModelIndex filterModelIndex = pNoteFilterModel->mapFromSource(sourceModelIndex);
    if (!filterModelIndex.isValid()) {
        QNTRACE(QStringLiteral("It appears the last current note model item is filtered out, at least the model index "
                               "within the note filter model is invalid"));
        return;
    }

    setCurrentIndex(filterModelIndex);
}

void NoteListView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NoteListView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: note list view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    showContextMenuAtPoint(pEvent->pos(), pEvent->globalPos());
}

void NoteListView::showContextMenuAtPoint(const QPoint & pos, const QPoint & globalPos)
{
    QNDEBUG(QStringLiteral("NoteListView::showContextMenuAtPoint"));

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the note item's context menu: wrong model connected to the note list view"));
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the note item's context menu: can't get the source model from the note filter model "
                                "connected to the note list view"));
        return;
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the note item's context menu: can't get the selection model from the view"));
        return;
    }

    QModelIndexList selectedRowIndexes = pSelectionModel->selectedIndexes();
    selectedRowIndexes << currentIndex();

    QStringList noteLocalUids;
    noteLocalUids.reserve(selectedRowIndexes.size());

    for(auto it = selectedRowIndexes.constBegin(), end = selectedRowIndexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & proxyModelIndex = *it;
        QNTRACE(QStringLiteral("Inspecting proxy model index at row ") << proxyModelIndex.row());

        QModelIndex modelIndex = pNoteFilterModel->mapToSource(proxyModelIndex);
        const NoteModelItem * pNoteModelItem = pNoteModel->itemForIndex(modelIndex);
        if (Q_UNLIKELY(!pNoteModelItem)) {
            QNWARNING(QStringLiteral("Detected selected model index for which no model item was found"));
            continue;
        }

        noteLocalUids << pNoteModelItem->localUid();
        QNTRACE(QStringLiteral("Included note local uid ") << pNoteModelItem->localUid());
    }

    Q_UNUSED(noteLocalUids.removeDuplicates())
    QNTRACE(QStringLiteral("Selected note local uids: ") << noteLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(noteLocalUids.isEmpty())) {
        QNDEBUG(QStringLiteral("Won't show the context menu: no notes are selected"));
        return;
    }

    if (noteLocalUids.size() == 1) {
        showSingleNoteContextMenu(pos, globalPos, *pNoteFilterModel, *pNoteModel);
    }
    else {
        showMultipleNotesContextMenu(globalPos, noteLocalUids);
    }
}

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteListView,slot)); \
        menu->addAction(pAction); \
    }

void NoteListView::showSingleNoteContextMenu(const QPoint & pos, const QPoint & globalPos,
                                             const NoteFilterModel & noteFilterModel, const NoteModel & noteModel)
{
    QNDEBUG(QStringLiteral("NoteListView::showSingleNoteContextMenu"));

    QModelIndex clickedFilterModelItemIndex = indexAt(pos);
    if (Q_UNLIKELY(!clickedFilterModelItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = noteFilterModel.mapToSource(clickedFilterModelItemIndex);
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: note item index mapped from the proxy index of note filter model is invalid"));
        return;
    }

    const NoteModelItem * pItem = noteModel.itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the note item's context menu: no item corresponding to the clicked item's index"));
        return;
    }

    if (!m_pNotebookItemView) {
        REPORT_ERROR(QT_TR_NOOP("Can't show the note item's context menu: no notebook item view is set to the note list view "
                                "for notebooks reference"));
        return;
    }

    const NotebookModel * pNotebookModel = qobject_cast<const NotebookModel*>(m_pNotebookItemView->model());

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);



    const NotebookItem * pNotebookItem = Q_NULLPTR;
    if (pNotebookModel)
    {
        QModelIndex notebookIndex = pNotebookModel->indexForLocalUid(pItem->notebookLocalUid());
        const NotebookModelItem * pNotebookModelItem = pNotebookModel->itemForIndex(notebookIndex);
        if (pNotebookModelItem && (pNotebookModelItem->type() == NotebookModelItem::Type::Notebook)) {
            pNotebookItem = pNotebookModelItem->notebookItem();
        }
    }

    bool canCreateNotes = (pNotebookItem
                           ? pNotebookItem->canCreateNotes()
                           : false);

    ADD_CONTEXT_MENU_ACTION(tr("Create new note"), m_pNoteItemContextMenu,
                            onCreateNewNoteAction, QString(), canCreateNotes);

    const QString & noteLocalUid = pItem->localUid();
    ADD_CONTEXT_MENU_ACTION(tr("Open in separate window"), m_pNoteItemContextMenu,
                            onOpenNoteInSeparateWindowAction, noteLocalUid, true);

    m_pNoteItemContextMenu->addSeparator();

    bool canUpdateNotes = (pNotebookItem
                           ? pNotebookItem->canUpdateNotes()
                           : false);

    ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pNoteItemContextMenu,
                            onDeleteNoteAction, noteLocalUid,
                            canUpdateNotes);

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pNoteItemContextMenu, onEditNoteAction,
                            noteLocalUid, canUpdateNotes);

    if (pNotebookModel && pNotebookItem && pNotebookItem->linkedNotebookGuid().isEmpty() && canUpdateNotes)
    {
        QStringList otherNotebookNames = pNotebookModel->notebookNames(NotebookModel::NotebookFilters(NotebookModel::NotebookFilter::CanCreateNotes));
        const QString & notebookName = pNotebookItem->name();
        auto it = std::lower_bound(otherNotebookNames.constBegin(), otherNotebookNames.constEnd(), notebookName);
        if ((it != otherNotebookNames.constEnd()) && (*it == notebookName)) {
            int offset = static_cast<int>(std::distance(otherNotebookNames.constBegin(), it));
            QStringList::iterator nit = otherNotebookNames.begin() + offset;
            Q_UNUSED(otherNotebookNames.erase(nit))
        }

        // Need to filter out other notebooks which prohibit the creation of notes in them
        // as moving the note from one notebook to another involves modifying the original notebook's note
        // and the "creation" of a note in another notebook
        for(auto it = otherNotebookNames.begin(); it != otherNotebookNames.end(); )
        {
            QModelIndex notebookItemIndex = pNotebookModel->indexForNotebookName(*it);
            if (Q_UNLIKELY(!notebookItemIndex.isValid())) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const NotebookModelItem * pNotebookModelItem = pNotebookModel->itemForIndex(notebookItemIndex);
            if (Q_UNLIKELY(!pNotebookModelItem)) {
                it = otherNotebookNames.erase(it);
                continue;
            }

            const NotebookItem * pOtherNotebookItem = pNotebookModelItem->notebookItem();
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
            QMenu * pTargetNotebooksSubMenu = m_pNoteItemContextMenu->addMenu(tr("Move to notebook"));
            for(auto it = otherNotebookNames.constBegin(), end = otherNotebookNames.constEnd(); it != end; ++it)
            {
                QStringList dataPair;
                dataPair.reserve(2);
                dataPair << noteLocalUid;
                dataPair << *it;
                ADD_CONTEXT_MENU_ACTION(*it, pTargetNotebooksSubMenu, onMoveToOtherNotebookAction,
                                        dataPair, true);
            }
        }
    }

    if (pItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pNoteItemContextMenu,
                                onUnfavoriteAction, noteLocalUid, true);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pNoteItemContextMenu,
                                onFavoriteAction, noteLocalUid, true);
    }

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Export to enex") + QStringLiteral("..."), m_pNoteItemContextMenu,
                            onExportSingleNoteToEnexAction, noteLocalUid, true);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pNoteItemContextMenu,
                            onShowNoteInfoAction, noteLocalUid, true);

    if (!pItem->guid().isEmpty()) {
        QStringList localUidAndGuid;
        localUidAndGuid.reserve(2);
        localUidAndGuid << noteLocalUid;
        localUidAndGuid << pItem->guid();
        ADD_CONTEXT_MENU_ACTION(tr("Copy in-app note link"), m_pNoteItemContextMenu,
                                onCopyInAppNoteLinkAction, localUidAndGuid, true);
    }

    QMenu * pThumbnailsSubMenu = m_pNoteItemContextMenu->addMenu(tr("Thumbnails"));
    QString showHideForAllNotes = m_showThumbnailsForAllNotes ? tr("Hide for all notes") : tr("Show for all notes");
    ADD_CONTEXT_MENU_ACTION(showHideForAllNotes, pThumbnailsSubMenu, onToggleThumbnailPreference,
                            QVariant::fromValue(QStringLiteral("")), true);

    const QByteArray & thumbnailData = pItem->thumbnailData();
    bool hasThumbnail = !thumbnailData.isEmpty();
    bool canToggleThumbnail = hasThumbnail && m_showThumbnailsForAllNotes;
    if (canToggleThumbnail) {
        bool isHiddenForCurrentNote = m_hideThumbnailsLocalUids.contains(noteLocalUid);
        QString showHideForCurrent = isHiddenForCurrentNote ? tr("Show for current note") : tr("Hide for current note");
        ADD_CONTEXT_MENU_ACTION( showHideForCurrent, pThumbnailsSubMenu, onToggleThumbnailPreference, noteLocalUid, true);
    }

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(globalPos);
}

void NoteListView::showMultipleNotesContextMenu(const QPoint & globalPos, const QStringList & noteLocalUids)
{
    QNDEBUG(QStringLiteral("NoteListView::showMultipleNotesContextMenu"));

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);

    ADD_CONTEXT_MENU_ACTION(tr("Export to enex") + QStringLiteral("..."), m_pNoteItemContextMenu,
                            onExportSeveralNotesToEnexAction, noteLocalUids, true);

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(globalPos);
}

void NoteListView::currentChanged(const QModelIndex & current,
                                  const QModelIndex & previous)
{
    QNTRACE(QStringLiteral("NoteListView::currentChanged"));
    Q_UNUSED(previous)

    if (!current.isValid()) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(current.model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: wrong model connected to the note list view"));
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't get the source model from the note filter model "
                                "connected to the note list view"));
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(current);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't retrieve the new current note item for note model index"));
        return;
    }

    m_lastCurrentNoteLocalUid = pItem->localUid();
    QNTRACE(QStringLiteral("Updated the last current note local uid to ") << pItem->localUid());

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
    QNDEBUG(QStringLiteral("NoteListView::currentNotebookItem"));

    QModelIndex currentNotebookItemIndex = m_pNotebookItemView->currentlySelectedItemIndex();
    if (Q_UNLIKELY(!currentNotebookItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("No current notebook within the notebook view"));
        return Q_NULLPTR;
    }

    const NotebookModel * pNotebookModel = qobject_cast<const NotebookModel*>(m_pNotebookItemView->model());
    if (!pNotebookModel) {
        QNDEBUG(QStringLiteral("No notebook model is set to the notebook view"));
        return Q_NULLPTR;
    }

    const NotebookModelItem * pNotebookModelItem = pNotebookModel->itemForIndex(currentNotebookItemIndex);
    if (Q_UNLIKELY(!pNotebookModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't find the notebook model item corresponding "
                                "to the current item selected in the notebooks view"));
        return Q_NULLPTR;
    }

    if (pNotebookModelItem->type() != NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Non-notebook item is selected within the notebook item view"));
        return Q_NULLPTR;
    }

    const NotebookItem * pNotebookItem = pNotebookModelItem->notebookItem();
    QNTRACE(QStringLiteral("Selected notebook item: ")
            << (pNotebookItem ? pNotebookItem->toString() : QStringLiteral("<null>")));

    return pNotebookItem;
}

} // namespace quentier
