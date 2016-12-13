#ifndef QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
#define QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NotebookModel)
QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NotebookStackItem)

class NotebookItemView: public ItemView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = Q_NULLPTR);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QNLocalizedString error);
    void newNotebookCreationRequested();

public Q_SLOTS:
    void deleteSelectedItem();

private Q_SLOTS:
    void onAllNotebooksListed();

    void onCreateNewNotebookAction();
    void onRenameNotebookAction();
    void onDeleteNotebookAction();
    void onSetNotebookDefaultAction();
    void onMoveNotebookToStackAction();

    void onRenameNotebookStackAction();
    void onDeleteNotebookStackAction();

    virtual void selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void deleteItem(const QModelIndex & itemIndex, NotebookModel & model);
    void showNotebookItemContextMenu(const NotebookItem & item,
                                     const QPoint & point, NotebookModel & model);
    void showNotebookStackItemContextMenu(const NotebookStackItem & item,
                                          const QPoint & point, NotebookModel & model);

    // Returns the valid index if all indexes in the list point to the same row;
    // otherwise returns invalid model index
    QModelIndex singleRow(const QModelIndexList & indexes, const quentier::NotebookModel & model) const;

private:
    void selectLastUsedOrDefaultNotebook(const NotebookModel & model);

private:
    QMenu *     m_pNotebookItemContextMenu;
    QMenu *     m_pNotebookStackItemContextMenu;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
