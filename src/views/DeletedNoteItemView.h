#ifndef QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H
#define QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/types/ErrorString.h>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModel)

class DeletedNoteItemView: public ItemView
{
    Q_OBJECT
public:
    explicit DeletedNoteItemView(QWidget * parent = Q_NULLPTR);

    QModelIndex currentlySelectedItemIndex() const;

    void restoreCurrentlySelectedNote();
    void deleteCurrentlySelectedNotePermanently();
    void showCurrentlySelectedNoteInfo();

Q_SIGNALS:
    void deletedNoteInfoRequested(QString deletedNoteLocalUid);
    void notifyError(ErrorString error);

private Q_SLOTS:
    void onRestoreNoteAction();
    void onDeleteNotePermanentlyAction();
    void onShowDeletedNoteInfoAction();

private:
    void restoreNote(const QModelIndex & index, NoteModel & model);
    void deleteNotePermanently(const QModelIndex & index, NoteModel & model);

private:
    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    QMenu *     m_pDeletedNoteItemContextMenu;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H
