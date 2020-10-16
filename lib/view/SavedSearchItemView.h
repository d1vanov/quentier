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

#ifndef QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H

#include "AbstractMultiSelectionItemView.h"

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class SavedSearchItemView : public AbstractMultiSelectionItemView
{
    Q_OBJECT
public:
    explicit SavedSearchItemView(QWidget * parent = nullptr);

Q_SIGNALS:
    void newSavedSearchCreationRequested();
    void savedSearchInfoRequested();

private:
    // AbstractMultiSelectionItemView interface
    virtual void saveItemsState() override;
    virtual void restoreItemsState(const ItemModel & model) override;

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

    virtual void connectToModel(ItemModel & model) override;

    virtual void deleteItem(
        const QModelIndex & itemIndex, ItemModel & model) override;

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

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void setFavoritedFlag(const QAction & action, const bool favorited);

private:
    QMenu * m_pSavedSearchItemContextMenu = nullptr;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H
