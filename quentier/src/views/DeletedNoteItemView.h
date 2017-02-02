#ifndef QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H
#define QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/utility/QNLocalizedString.h>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

class DeletedNoteItemView: public ItemView
{
    Q_OBJECT
public:
    explicit DeletedNoteItemView(QWidget * parent = Q_NULLPTR);

Q_SIGNALS:
    void deletedNoteInfoRequested(QString deletedNoteLocalUid);
    void notifyError(QNLocalizedString error);

private Q_SLOTS:
    void onRestoreNoteAction();
    void onDeleteNotePermanentlyAction();
    void onShowDeletedNoteInfoAction();

private:
    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    QMenu *     m_pDeletedNoteItemContextMenu;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_DELETED_NOTE_ITEM_VIEW_H
