/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H

#include "AbstractNoteFilteringTreeView.h"

namespace quentier {

class Account;
class NoteFiltersManager;
class TagModel;

class TagItemView final : public AbstractNoteFilteringTreeView
{
    Q_OBJECT
public:
    explicit TagItemView(QWidget * parent = nullptr);

Q_SIGNALS:
    void newTagCreationRequested();
    void tagInfoRequested();

private:
    // AbstractNoteFilteringTreeView interface
    void saveItemsState() override;
    void restoreItemsState(
        const AbstractItemModel & itemModel) override;

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
    void onAboutToAddTag();
    void onAddedTag(const QModelIndex & index);

    void onAboutToUpdateTag(const QModelIndex & index);
    void onUpdatedTag(const QModelIndex & index);

    void onAboutToRemoveTags();
    void onRemovedTags();

    void onCreateNewTagAction();
    void onRenameTagAction();
    void onDeleteTagAction();

    void onEditTagAction();
    void onPromoteTagAction();
    void onDemoteTagAction();
    void onRemoveFromParentTagAction();
    void onMoveTagToParentAction();
    void onShowTagInfoAction();
    void onDeselectAction();

    void onFavoriteAction();
    void onUnfavoriteAction();

    void onTagParentChanged(const QModelIndex & index);

    void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void setTagsExpanded(
        const QStringList & tagLocalIds, const TagModel & model);

    void setLinkedNotebooksExpanded(
        const QStringList & linkedNotebookGuids, const TagModel & model);

    void setFavoritedFlag(const QAction & action, bool favorited);

    void setSelectedTagsToNoteFiltersManager(const QStringList & tagLocalIds);
    void clearTagsFromNoteFiltersManager();

private:
    QMenu * m_pTagItemContextMenu = nullptr;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_TAG_ITEM_VIEW_H
