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

#include "DeletedNoteItemView.h"

#include <lib/model/NoteModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:deleted_notes", errorDescription);                     \
        Q_EMIT notifyError(errorDescription);                                  \
    }                                                                          \
// REPORT_ERROR

namespace quentier {

DeletedNoteItemView::DeletedNoteItemView(QWidget * parent) :
    ItemView(parent)
{}

QModelIndex DeletedNoteItemView::currentlySelectedItemIndex() const
{
    const auto * pNoteModel = qobject_cast<const NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return {};
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("view:deleted_notes", "No selection model in the view");
        return {};
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("view:deleted_notes", "The selection contains no model "
            << "indexes");
        return {};
    }

    return singleRow(indexes, *pNoteModel, NoteModel::Columns::Title);
}

void DeletedNoteItemView::restoreCurrentlySelectedNote()
{
    QNDEBUG(
        "view:deleted_notes",
        "DeletedNoteItemView::restoreCurrentlySelectedNote");

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    auto currentItemIndex = currentlySelectedItemIndex();
    restoreNote(currentItemIndex, *pNoteModel);
}

void DeletedNoteItemView::deleteCurrentlySelectedNotePermanently()
{
    QNDEBUG(
        "view",
        "DeletedNoteItemView::deleteCurrentlySelectedNotePermanently");

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    auto currentItemIndex = currentlySelectedItemIndex();
    deleteNotePermanently(currentItemIndex, *pNoteModel);
}

void DeletedNoteItemView::showCurrentlySelectedNoteInfo()
{
    QNDEBUG(
        "view:deleted_notes",
        "DeletedNoteItemView::showCurrentlySelectedNoteInfo");

    const auto * pNoteModel = qobject_cast<const NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    auto currentItemIndex = currentlySelectedItemIndex();
    if (!currentItemIndex.isValid()) {
        QNDEBUG("view:deleted_notes", "No item is selected");
        return;
    }

    const auto * pItem = pNoteModel->itemForIndex(currentItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the deleted note info: no note model item "
                       "corresponding to the currently selected index"))
        return;
    }

    Q_EMIT deletedNoteInfoRequested(pItem->localUid());
}

void DeletedNoteItemView::onRestoreNoteAction()
{
    QNDEBUG("view:deleted_notes", "DeletedNoteItemView::onRestoreNoteAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't restore note, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't restore note, "
                       "can't get note's local uid from QAction"))
        return;
    }

    auto index = pNoteModel->indexForLocalUid(itemLocalUid);
    restoreNote(index, *pNoteModel);
}

void DeletedNoteItemView::onDeleteNotePermanentlyAction()
{
    QNDEBUG(
        "view:deleted_notes",
        "DeletedNoteItemView::onDeleteNotePermanentlyAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete the note permanently, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete the note permanently, "
                       "can't get note's local uid from QAction"))
        return;
    }

    auto index = pNoteModel->indexForLocalUid(itemLocalUid);
    deleteNotePermanently(index, *pNoteModel);
}

void DeletedNoteItemView::onShowDeletedNoteInfoAction()
{
    QNDEBUG(
        "view:deleted_notes",
        "DeletedNoteItemView::onShowDeletedNoteInfoAction");

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't show the deleted note info, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used");
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't show the deleted note info, "
                       "can't get note's local uid from QAction"))
        return;
    }

    Q_EMIT deletedNoteInfoRequested(itemLocalUid);
}

void DeletedNoteItemView::restoreNote(
    const QModelIndex & index, NoteModel & model)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't restore note, "
                       "the index of the note to be restored is invalid "))
        return;
    }

    auto deletionTimestampIndex = model.index(
        index.row(),
        NoteModel::Columns::DeletionTimestamp,
        index.parent());

    bool res = model.setData(
        deletionTimestampIndex,
        QVariant(qint64(-1)),
        Qt::EditRole);

    if (res) {
        QNDEBUG("view:deleted_notes", "Successfully restored the note");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The note model refused to restore the deleted note; Check "
           "the status bar for message from the note model explaining why "
           "the action was not successful")));
}

void DeletedNoteItemView::deleteNotePermanently(
    const QModelIndex & index, NoteModel & model)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete the note permanently, "
                       "the index of the note to be deleted permanently "
                       "is invalid"))
        return;
    }

    int confirm = warningMessageBox(
        this,
        tr("Confirm the permanent deletion of note"),
        tr("Are you sure you want to delete the note permanently?"),
        tr("Note that this action is not reversible, you won't be able to "
           "restore the permanently deleted note"),
        QMessageBox::Ok | QMessageBox::No);

    if (confirm != QMessageBox::Ok) {
        QNDEBUG("view:deleted_notes", "The permanent deletion of note was not "
            << "confirmed");
        return;
    }

    bool res = model.removeRow(index.row(), index.parent());
    if (res) {
        QNDEBUG("view:deleted_notes", "Successfully removed the note "
            << "completely from the model");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The note model refused to delete the note permanently; Check "
           "the status bar for message from the note model explaining why "
           "the action was not successful")));
}

void DeletedNoteItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:deleted_notes", "DeletedNoteItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("view:deleted_notes", "Detected Qt error: deleted notes item "
            << "view received context menu event with null pointer to "
            << "the context menu event");
        return;
    }

    auto * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG("view:deleted_notes", "Non-note model is used, not doing "
            << "anything");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG("view:deleted_notes", "Clicked item index is not valid, not "
            << "doing anything");
        return;
    }

    const auto * pItem = pNoteModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for deleted note: "
                       "internal error, can't find the corresponding "
                       "item within the note model"))
        return;
    }

    delete m_pDeletedNoteItemContextMenu;
    m_pDeletedNoteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction,                                                           \
            &QAction::triggered,                                               \
            this,                                                              \
            &DeletedNoteItemView::slot);                                       \
        menu->addAction(pAction);                                              \
    }                                                                          \
// ADD_CONTEXT_MENU_ACTION

    ADD_CONTEXT_MENU_ACTION(
        tr("Restore note"),
        m_pDeletedNoteItemContextMenu,
        onRestoreNoteAction,
        pItem->localUid(),
        true);

    if (pItem->guid().isEmpty()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete permanently"),
            m_pDeletedNoteItemContextMenu,
            onDeleteNotePermanentlyAction,
            pItem->localUid(),
            true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pDeletedNoteItemContextMenu,
        onShowDeletedNoteInfoAction,
        pItem->localUid(),
        true);

    m_pDeletedNoteItemContextMenu->show();
    m_pDeletedNoteItemContextMenu->exec(pEvent->globalPos());
}

} // namespace quentier
