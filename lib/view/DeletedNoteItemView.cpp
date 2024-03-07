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

#include "DeletedNoteItemView.h"

#include <lib/model/note/NoteModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view::DeletedNoteItemView", errorDescription);              \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

DeletedNoteItemView::DeletedNoteItemView(QWidget * parent) : TreeView{parent} {}

QModelIndex DeletedNoteItemView::currentlySelectedItemIndex() const
{
    const auto * noteModel = qobject_cast<const NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return {};
    }

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        QNDEBUG("view::DeletedNoteItemView", "No selection model in the view");
        return {};
    }

    const auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(
            "view::DeletedNoteItemView",
            "The selection contains no model indexes");
        return {};
    }

    return singleRow(
        indexes, *noteModel, static_cast<int>(NoteModel::Column::Title));
}

void DeletedNoteItemView::restoreCurrentlySelectedNote()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::restoreCurrentlySelectedNote");

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const auto currentItemIndex = currentlySelectedItemIndex();
    restoreNote(currentItemIndex, *noteModel);
}

void DeletedNoteItemView::deleteCurrentlySelectedNotePermanently()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::deleteCurrentlySelectedNotePermanently");

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const auto currentItemIndex = currentlySelectedItemIndex();
    deleteNotePermanently(currentItemIndex, *noteModel);
}

void DeletedNoteItemView::showCurrentlySelectedNoteInfo()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::showCurrentlySelectedNoteInfo");

    const auto * noteModel = qobject_cast<const NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const auto currentItemIndex = currentlySelectedItemIndex();
    if (!currentItemIndex.isValid()) {
        QNDEBUG("view::DeletedNoteItemView", "No item is selected");
        return;
    }

    const auto * pItem = noteModel->itemForIndex(currentItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the deleted note info: no note model item "
                       "corresponding to the currently selected index"))
        return;
    }

    Q_EMIT deletedNoteInfoRequested(pItem->localId());
}

void DeletedNoteItemView::onRestoreNoteAction()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::onRestoreNoteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't restore note, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't restore note, "
                       "can't get note's local uid from QAction"))
        return;
    }

    const auto index = noteModel->indexForLocalId(itemLocalId);
    restoreNote(index, *noteModel);
}

void DeletedNoteItemView::onDeleteNotePermanentlyAction()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::onDeleteNotePermanentlyAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete the note permanently, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete the note permanently, "
                       "can't get note's local uid from QAction"))
        return;
    }

    const auto index = noteModel->indexForLocalId(itemLocalId);
    deleteNotePermanently(index, *noteModel);
}

void DeletedNoteItemView::onShowDeletedNoteInfoAction()
{
    QNDEBUG(
        "view::DeletedNoteItemView",
        "DeletedNoteItemView::onShowDeletedNoteInfoAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't show the deleted note info, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG("view::DeletedNoteItemView", "Non-note model is used");
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't show the deleted note info, "
                       "can't get note's local uid from QAction"))
        return;
    }

    Q_EMIT deletedNoteInfoRequested(itemLocalId);
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

    const auto deletionTimestampIndex = model.index(
        index.row(), static_cast<int>(NoteModel::Column::DeletionTimestamp),
        index.parent());

    if (model.setData(
            deletionTimestampIndex, QVariant(qint64(-1)), Qt::EditRole))
    {
        QNDEBUG("view::DeletedNoteItemView", "Successfully restored the note");
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

    const NoteModelItem * noteModelItem = model.itemForIndex(index);
    if (Q_UNLIKELY(!noteModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to get note model item "
                       "corresponding to index"));
        return;
    }

    int confirm = QMessageBox::No;
    if (!noteModelItem->guid().isEmpty()) {
        confirm = warningMessageBox(
            this, tr("Confirm the permanent deletion of note"),
            tr("Are you sure you want to delete the note permanently?"),
            tr("Evernote prohibits thirdparty clients from permanently "
               "deleting data items which were synchronized with the service, "
               "it can only be done properly via the official Evernote client. "
               "If you delete this note permanenly from Quentier, its deletion "
               "will not be reflected in Evernote service unless you manually "
               "delete it via the official Evernote client as well. Note that "
               "deleting the note in Quentier is not reversible, you won't be "
               "able to restore this note unless you modify it somehow via the "
               "official Evernote client so that it can be synchronized into "
               "Quentier once again."),
            QMessageBox::Ok | QMessageBox::No);
    }
    else {
        confirm = warningMessageBox(
            this, tr("Confirm the permanent deletion of note"),
            tr("Are you sure you want to delete the note permanently?"),
            tr("Note that this action is not reversible, you won't be able to "
               "restore the permanently deleted note."),
            QMessageBox::Ok | QMessageBox::No);
    }

    if (confirm != QMessageBox::Ok) {
        QNDEBUG(
            "view::DeletedNoteItemView",
            "The permanent deletion of note was not confirmed");
        return;
    }

    if (model.removeRow(index.row(), index.parent())) {
        QNDEBUG(
            "view::DeletedNoteItemView",
            "Successfully removed the note completely from the model");
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
    QNDEBUG(
        "view::DeletedNoteItemView", "DeletedNoteItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "view::DeletedNoteItemView",
            "Detected Qt error: deleted notes item "
                << "view received context menu event with null pointer to "
                << "the context menu event");
        return;
    }

    auto * noteModel = qobject_cast<NoteModel *>(model());
    if (Q_UNLIKELY(!noteModel)) {
        QNDEBUG(
            "view::DeletedNoteItemView",
            "Non-note model is used, not doing "
                << "anything");
        return;
    }

    const auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::DeletedNoteItemView",
            "Clicked item index is not valid, not "
                << "doing anything");
        return;
    }

    const auto * item = noteModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for deleted note: "
                       "internal error, can't find the corresponding "
                       "item within the note model"))
        return;
    }

    delete m_deletedNoteItemContextMenu;
    m_deletedNoteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * action = new QAction(name, menu);                            \
        action->setData(data);                                                 \
        action->setEnabled(enabled);                                           \
        QObject::connect(                                                      \
            action, &QAction::triggered, this, &DeletedNoteItemView::slot);    \
        menu->addAction(action);                                               \
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Restore note"), m_deletedNoteItemContextMenu, onRestoreNoteAction,
        item->localId(), true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Delete permanently"), m_deletedNoteItemContextMenu,
        onDeleteNotePermanentlyAction, item->localId(), true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."), m_deletedNoteItemContextMenu,
        onShowDeletedNoteInfoAction, item->localId(), true);

    m_deletedNoteItemContextMenu->show();
    m_deletedNoteItemContextMenu->exec(pEvent->globalPos());
}

} // namespace quentier
