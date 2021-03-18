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

#ifndef QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H

#include "AbstractNoteFilteringTreeView.h"

namespace quentier {

class SavedSearchModel;

class SavedSearchItemView final : public AbstractNoteFilteringTreeView
{
    Q_OBJECT
public:
    explicit SavedSearchItemView(QWidget * parent = nullptr);

Q_SIGNALS:
    void newSavedSearchCreationRequested();
    void savedSearchInfoRequested();

private:
    // AbstractNoteFilteringTreeView interface
    void saveItemsState() override;
    void restoreItemsState(const AbstractItemModel & model) override;

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

    void connectToModel(AbstractItemModel & model) override;

    void deleteItem(
        const QModelIndex & itemIndex, AbstractItemModel & model) override;

private Q_SLOTS:
    void onAboutToAddSavedSearch();
    void onAddedSavedSearch(const QModelIndex & index);

    void onAboutToUpdateSavedSearch(const QModelIndex & index);
    void onUpdatedSavedSearch(const QModelIndex & index);

    void onAboutToRemoveSavedSearches();
    void onRemovedSavedSearches();

    void onCreateNewSavedSearchAction();
    void onRenameSavedSearchAction();
    void onDeleteSavedSearchAction();

    void onEditSavedSearchAction();
    void onShowSavedSearchInfoAction();
    void onDeselectAction();

    void onFavoriteAction();
    void onUnfavoriteAction();

    void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void setFavoritedFlag(const QAction & action, bool favorited);

private:
    QMenu * m_pSavedSearchItemContextMenu = nullptr;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H
