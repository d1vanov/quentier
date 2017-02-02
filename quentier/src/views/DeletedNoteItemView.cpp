#include "DeletedNoteItemView.h"
#include "../models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>

#define REPORT_ERROR(error) \
    { \
        QNLocalizedString errorDescription = QNLocalizedString(error, this); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

DeletedNoteItemView::DeletedNoteItemView(QWidget * parent) :
    ItemView(parent),
    m_pDeletedNoteItemContextMenu(Q_NULLPTR)
{}

void DeletedNoteItemView::onRestoreNoteAction()
{
    QNDEBUG(QStringLiteral("DeletedNoteItemView::onRestoreNoteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't restore note, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Non-note model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't restore note, "
                     "can't get note's local uid from QAction")
        return;
    }

    QModelIndex index = pNoteModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR("Internal error: can't restore note, "
                     "the note model returned invalid index for the local uid "
                     "of note to be restored")
        return;
    }

    QModelIndex deletionTimestampIndex = pNoteModel->index(index.row(), NoteModel::Columns::DeletionTimestamp,
                                                           index.parent());
    bool res = pNoteModel->setData(deletionTimestampIndex, QVariant(qint64(-1)), Qt::EditRole);
    if (res) {
        QNDEBUG(QStringLiteral("Successfully restored the note"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The note model refused to restore the deleted note; "
                                              "Check the status bar for message from the note model "
                                              "explaining why the action was not successful")));
}

void DeletedNoteItemView::onDeleteNotePermanentlyAction()
{
    QNDEBUG(QStringLiteral("DeletedNoteItemView::onDeleteNotePermanentlyAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't delete the note permanently, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Non-note model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't delete the note permanently, "
                     "can't get note's local uid from QAction")
        return;
    }

    QModelIndex index = pNoteModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR("Internal error: can't delete the note permanently, "
                     "the note model returned invalid index for the local uid "
                     "of note to be restored")
        return;
    }

    int confirm = warningMessageBox(this, tr("Confirm the permanent deletion of note"),
                                    tr("Are you sure you want to delete the note permanently?"),
                                    tr("Note that this action is not reversible, you won't be able to restore "
                                       "the permanently deleted note"), QMessageBox::Ok | QMessageBox::No);
    if (confirm != QMessageBox::Ok) {
        QNDEBUG(QStringLiteral("The permanent deletion of note was not confirmed"));
        return;
    }

    bool res = pNoteModel->removeRow(index.row(), index.parent());
    if (res) {
        QNDEBUG(QStringLiteral("Successfully removed the note completely from the model"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The note model refused to delete the note permanently; "
                                              "Check the status bar for message from the note model "
                                              "explaining why the action was not successful")));
}

void DeletedNoteItemView::onShowDeletedNoteInfoAction()
{
    QNDEBUG(QStringLiteral("DeletedNoteItemView::onShowDeletedNoteInfoAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR("Internal error: can't show the deleted note info, "
                     "can't cast the slot invoker to QAction")
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Non-note model is used"));
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR("Internal error: can't show the deleted note info, "
                     "can't get note's local uid from QAction")
        return;
    }

    emit deletedNoteInfoRequested(itemLocalUid);
}

void DeletedNoteItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("DeletedNoteItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: deleted notes item view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("Non-note model is used, not doing anything"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const NoteModelItem * pItem = pNoteModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR("Can't show the context menu for deleted note: "
                     "internal error, can't find the corresponding item "
                     "within the note model");
        return;
    }

    delete m_pDeletedNoteItemContextMenu;
    m_pDeletedNoteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(DeletedNoteItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Restore note"), m_pDeletedNoteItemContextMenu,
                            onRestoreNoteAction, pItem->localUid(), true);

    if (pNoteModel->account().type() == Account::Type::Local) {
        ADD_CONTEXT_MENU_ACTION(tr("Delete permanently"), m_pDeletedNoteItemContextMenu,
                                onDeleteNotePermanentlyAction, pItem->localUid(), true);
    }

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pDeletedNoteItemContextMenu,
                            onShowDeletedNoteInfoAction, pItem->localUid(), true);

    m_pDeletedNoteItemContextMenu->show();
    m_pDeletedNoteItemContextMenu->exec(pEvent->globalPos());
}

} // namespace quentier
