/*
 * Copyright 2016 Dmitry Ivanov
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
    void onEditNotebookAction();
    void onMoveNotebookToStackAction();
    void onRemoveNotebookFromStackAction();

    void onRenameNotebookStackAction();
    void onRemoveNotebooksFromStackAction();

    void onNotebookStackItemCollapsedOrExpanded(const QModelIndex & index);

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

    void saveNotebookStackItemsState();
    void restoreNotebookStackItemsState(const NotebookModel & model);

    void restoreLastSavedSelectionOrAutoSelectNotebook(const NotebookModel & model);
    void autoSelectNotebook(const NotebookModel & model);

private:
    QMenu *     m_pNotebookItemContextMenu;
    QMenu *     m_pNotebookStackItemContextMenu;
    bool        m_restoringNotebookStackItemsState;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_NOTEBOOK_ITEM_VIEW_H
