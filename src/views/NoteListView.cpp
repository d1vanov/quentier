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
#include <QMenu>
#include <iterator>

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

NoteListView::NoteListView(QWidget * parent) :
    QListView(parent),
    m_pNoteItemContextMenu(Q_NULLPTR),
    m_pNotebookItemView(Q_NULLPTR)
{}

void NoteListView::setNotebookItemView(NotebookItemView * pNotebookItemView)
{
    QNDEBUG(QStringLiteral("NoteListView::setNotebookItemView"));

    m_pNotebookItemView = pNotebookItemView;
}

void NoteListView::onCurrentNoteChanged(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("NoteListView::onCurrentNoteChanged: ") << noteLocalUid);

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
        if ((row >= start) && (row <= end)) {
            // The default implementation moves current to the next remaining item
            // but for this view it makes more sense to just get rid of the item altogether
            setCurrentIndex(QModelIndex());
        }
    }

    QListView::rowsAboutToBeRemoved(parent, start, end);
}

void NoteListView::onCreateNewNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onCreateNewNoteAction"));

    emit newNoteCreationRequested();
}

void NoteListView::onDeleteNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onDeleteNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't delete note, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't delete note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't delete note: can't get the source model from the note filter model connected to the note list view"));
        return;
    }

    bool res = pNoteModel->deleteNote(pAction->data().toString());
    if (!res) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't delete note: can't find the item to be deleted within the model"));
        return;
    }
}

void NoteListView::onEditNoteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onEditNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't edit note: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't edit note: internal error, the local uid "
                                       "of the note to be edited is empty"));
        return;
    }

    emit editNoteDialogRequested(noteLocalUid);
}

void NoteListView::onMoveToOtherNotebookAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onMoveToOtherNotebookAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't move note to another notebook: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    QStringList actionData = pAction->data().toStringList();
    if (actionData.isEmpty() || (actionData.size() != 2)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't move note to another notebook: internal error, "
                                       "wrong action data"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't move note to another notebook: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't move note to another notebook: can't get the source model from the note filter model "
                                       "connected to the note list view"));
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
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't open note in a separate window: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    emit openNoteInSeparateWindowRequested(pAction->data().toString());
}

void NoteListView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("NoteListView::onUnfavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't unfavorite note: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't unfavorite note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't unfavorite note: can't get the source model from the note filter model "
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
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't favorite note: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    NoteFilterModel * pNoteFilterModel = qobject_cast<NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't favorite note: wrong model connected to the note list view"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't favorite note: can't get the source model from the note filter model "
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
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show note info: internal error, "
                                       "can't cast the slot invoker to QAction"));
        return;
    }

    QString noteLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show note info: internal error, the local uid "
                                       "of the note to be edited is empty"));
        return;
    }

    emit noteInfoDialogRequested(noteLocalUid);
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

    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the note item's context menu: wrong model connected to the note list view"));
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the note item's context menu: can't get the source model from the note filter model "
                                       "connected to the note list view"));
        return;
    }

    QModelIndex clickedFilterModelItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedFilterModelItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = pNoteFilterModel->mapToSource(clickedFilterModelItemIndex);
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: note item index mapped from the proxy index of note filter model is invalid"));
        return;
    }

    const NoteModelItem * pItem = pNoteModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the note item's context menu: no item corresponding to the clicked item's index"));
        return;
    }

    if (!m_pNotebookItemView) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the note item's context menu: no notebook item view is set to the note list view "
                                       "for notebooks reference"));
        return;
    }

    delete m_pNoteItemContextMenu;
    m_pNoteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteListView,slot)); \
        menu->addAction(pAction); \
    }

    const NotebookItem * pCurrentNotebookItem = currentNotebookItem();

    bool canCreateNote = (pCurrentNotebookItem
                          ? pCurrentNotebookItem->canCreateNotes()
                          : false);

    ADD_CONTEXT_MENU_ACTION(tr("Create new note"), m_pNoteItemContextMenu,
                            onCreateNewNoteAction, QString(), canCreateNote);

    ADD_CONTEXT_MENU_ACTION(tr("Open in separate window"), m_pNoteItemContextMenu,
                            onOpenNoteInSeparateWindowAction, pItem->localUid(), true);

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pNoteItemContextMenu,
                            onDeleteNoteAction, pItem->localUid(),
                            !pItem->isSynchronizable());

    bool canUpdateNote = (pCurrentNotebookItem
                          ? pCurrentNotebookItem->canUpdateNotes()
                          : false);
    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pNoteItemContextMenu, onEditNoteAction,
                            pItem->localUid(), canUpdateNote);

    if (pCurrentNotebookItem)
    {
        QStringList otherNotebookNames;
        const NotebookModel * pNotebookModel = qobject_cast<const NotebookModel*>(m_pNotebookItemView->model());
        if (pNotebookModel)
        {
            otherNotebookNames = pNotebookModel->notebookNames(NotebookModel::NotebookFilters(NotebookModel::NotebookFilter::CanCreateNotes));
            const QString & currentNotebookName = pCurrentNotebookItem->name();
            auto it = std::lower_bound(otherNotebookNames.constBegin(), otherNotebookNames.constEnd(),
                                       currentNotebookName);
            if ((it != otherNotebookNames.constEnd()) && (*it == currentNotebookName)) {
                int offset = static_cast<int>(std::distance(otherNotebookNames.constBegin(), it));
                QStringList::iterator nit = otherNotebookNames.begin() + offset;
                Q_UNUSED(otherNotebookNames.erase(nit))
            }
        }

        if (!otherNotebookNames.isEmpty())
        {
            QMenu * pTargetNotebooksSubMenu = m_pNoteItemContextMenu->addMenu(tr("Move to notebook"));
            for(auto it = otherNotebookNames.constBegin(), end = otherNotebookNames.constEnd(); it != end; ++it)
            {
                QStringList dataPair;
                dataPair.reserve(2);
                dataPair << pItem->localUid();
                dataPair << *it;
                ADD_CONTEXT_MENU_ACTION(*it, pTargetNotebooksSubMenu, onMoveToOtherNotebookAction,
                                        dataPair, true);
            }
        }
    }

    if (pItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pNoteItemContextMenu,
                                onUnfavoriteAction, pItem->localUid(), true);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pNoteItemContextMenu,
                                onFavoriteAction, pItem->localUid(), true);
    }

    m_pNoteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pNoteItemContextMenu,
                            onShowNoteInfoAction, pItem->localUid(), true);

    m_pNoteItemContextMenu->show();
    m_pNoteItemContextMenu->exec(pEvent->globalPos());
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
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: wrong model connected to the note list view"));
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't get the source model from the note filter model "
                                       "connected to the note list view"));
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(current);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't retrieve the new current note item for note model index"));
        return;
    }

    emit currentNoteChanged(pItem->localUid());
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
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't find the notebook model item corresponding "
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
