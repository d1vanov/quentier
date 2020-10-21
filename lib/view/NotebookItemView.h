/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H

#include "AbstractNoteFilteringTreeView.h"

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(INotebookModelItem)
QT_FORWARD_DECLARE_CLASS(NoteModel)
QT_FORWARD_DECLARE_CLASS(NotebookModel)
QT_FORWARD_DECLARE_CLASS(NotebookItem)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)
QT_FORWARD_DECLARE_CLASS(StackItem)

class NotebookItemView : public AbstractNoteFilteringTreeView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = nullptr);

    void setNoteModel(const NoteModel * pNoteModel);

Q_SIGNALS:
    void newNotebookCreationRequested();
    void notebookInfoRequested();

private:
    // AbstractNoteFilteringTreeView interface
    virtual void saveItemsState() override;
    virtual void restoreItemsState(const ItemModel & itemModel) override;

    virtual QString selectedItemsGroupKey() const override;
    virtual QString selectedItemsArrayKey() const override;
    virtual QString selectedItemsKey() const override;

    virtual bool shouldFilterBySelectedItems(
        const Account & account) const override;

    virtual QStringList localUidsInNoteFiltersManager(
        const NoteFiltersManager & noteFiltersManager) const override;

    virtual void setItemLocalUidsToNoteFiltersManager(
        const QStringList & itemLocalUids,
        NoteFiltersManager & noteFiltersManager) override;

    virtual void removeItemLocalUidsFromNoteFiltersManager(
        NoteFiltersManager & noteFiltersManager) override;

    virtual void connectToModel(ItemModel & itemModel) override;

    virtual void deleteItem(
        const QModelIndex & itemIndex, ItemModel & model) override;

private Q_SLOTS:
    void onAboutToAddNotebook();
    void onAddedNotebook(const QModelIndex & index);

    void onAboutToUpdateNotebook(const QModelIndex & index);
    void onUpdatedNotebook(const QModelIndex & index);

    void onAboutToRemoveNotebooks();
    void onRemovedNotebooks();

    void onCreateNewNotebookAction();
    void onRenameNotebookAction();
    void onDeleteNotebookAction();
    void onSetNotebookDefaultAction();
    void onShowNotebookInfoAction();

    void onEditNotebookAction();
    void onMoveNotebookToStackAction();
    void onRemoveNotebookFromStackAction();

    void onRenameNotebookStackAction();
    void onRemoveNotebooksFromStackAction();

    void onFavoriteAction();
    void onUnfavoriteAction();

    void onNotebookStackRenamed(
        const QString & previousStackName, const QString & newStackName,
        const QString & linkedNotebookGuid);

    void onNotebookStackChanged(const QModelIndex & notebookIndex);

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void showNotebookItemContextMenu(
        const NotebookItem & item, const QPoint & point, NotebookModel & model);

    void showNotebookStackItemContextMenu(
        const StackItem & item, const INotebookModelItem & modelItem,
        const QPoint & point, NotebookModel & model);

    void setStacksExpanded(
        const QStringList & expandedStackNames, const NotebookModel & model,
        const QString & linkedNotebookGuid);

    void setLinkedNotebooksExpanded(
        const QStringList & expandedLinkedNotebookGuids,
        const NotebookModel & model);

    void setFavoritedFlag(const QAction & action, const bool favorited);

    // Helper structs and methods to access common data pieces in slots

    struct NotebookCommonData
    {
        NotebookModel * m_pModel = nullptr;
        QModelIndex m_index;
        QAction * m_pAction = nullptr;
    };

    bool fetchCurrentNotebookCommonData(
        NotebookCommonData & data, ErrorString & errorDescription) const;

    struct NotebookItemData : public NotebookCommonData
    {
        QString m_localUid;
    };

    bool fetchCurrentNotebookItemData(
        NotebookItemData & itemData, ErrorString & errorDescription) const;

    struct NotebookStackData : public NotebookCommonData
    {
        QString m_stack;
        QString m_id;
    };

    bool fetchCurrentNotebookStackData(
        NotebookStackData & stackData, ErrorString & errorDescription) const;

private:
    QMenu * m_pNotebookItemContextMenu = nullptr;
    QMenu * m_pNotebookStackItemContextMenu = nullptr;

    QPointer<const NoteModel> m_pNoteModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_NOTEBOOK_ITEM_VIEW_H
