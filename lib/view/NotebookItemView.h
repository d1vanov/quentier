/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include "AbstractNoteFilteringTreeView.h"

namespace quentier {

class Account;
class INotebookModelItem;
class NoteModel;
class NotebookModel;
class NotebookItem;
class NoteFiltersManager;
class StackItem;

class NotebookItemView : public AbstractNoteFilteringTreeView
{
    Q_OBJECT
public:
    explicit NotebookItemView(QWidget * parent = nullptr);

    void setNoteModel(const NoteModel * noteModel);

Q_SIGNALS:
    void newNotebookCreationRequested();
    void notebookInfoRequested();

private:
    // AbstractNoteFilteringTreeView interface
    void saveItemsState() override;
    void restoreItemsState(const AbstractItemModel & itemModel) override;

    [[nodiscard]] QString selectedItemsGroupKey() const override;
    [[nodiscard]] QString selectedItemsArrayKey() const override;
    [[nodiscard]] QString selectedItemsKey() const override;

    [[nodiscard]] bool shouldFilterBySelectedItems(
        const Account & account) const override;

    [[nodiscard]] QStringList localIdsInNoteFiltersManager(
        const NoteFiltersManager & noteFiltersManager) const override;

    void setItemLocalIdsToNoteFiltersManager(
        const QStringList & itemLocalIds,
        NoteFiltersManager & noteFiltersManager) override;

    void removeItemLocalIdsFromNoteFiltersManager(
        NoteFiltersManager & noteFiltersManager) override;

    void connectToModel(AbstractItemModel & itemModel) override;

    void deleteItem(
        const QModelIndex & itemIndex, AbstractItemModel & model) override;

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

    void contextMenuEvent(QContextMenuEvent * event) override;

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
        NotebookModel * m_model = nullptr;
        QModelIndex m_index;
        QAction * m_action = nullptr;
    };

    [[nodiscard]] bool fetchCurrentNotebookCommonData(
        NotebookCommonData & data, ErrorString & errorDescription) const;

    struct NotebookItemData : public NotebookCommonData
    {
        QString m_localId;
    };

    [[nodiscard]] bool fetchCurrentNotebookItemData(
        NotebookItemData & itemData, ErrorString & errorDescription) const;

    struct NotebookStackData : public NotebookCommonData
    {
        QString m_stack;
        QString m_id;
    };

    bool fetchCurrentNotebookStackData(
        NotebookStackData & stackData, ErrorString & errorDescription) const;

private:
    QMenu * m_notebookItemContextMenu = nullptr;
    QMenu * m_notebookStackItemContextMenu = nullptr;
    QPointer<const NoteModel> m_noteModel;
};

} // namespace quentier
