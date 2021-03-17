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

#ifndef QUENTIER_LIB_VIEW_FAVORITE_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_FAVORITE_ITEM_VIEW_H

#include "AbstractNoteFilteringTreeView.h"

#include <quentier/types/ErrorString.h>

namespace quentier {

class FavoritesModel;

class FavoriteItemView final : public AbstractNoteFilteringTreeView
{
    Q_OBJECT
public:
    explicit FavoriteItemView(QWidget * parent = nullptr);

    void unfavoriteSelectedItems();

Q_SIGNALS:
    void favoritedItemInfoRequested();
    void favoritedNoteSelected(QString noteLocalUid);

private:
    // AbstractNoteFilteringTreeView interface
    void saveItemsState() override {}

    void restoreItemsState(const AbstractItemModel & model) override
    {
        Q_UNUSED(model)
    }

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

    void processSelectedItem(
        const QString & itemLocalId, AbstractItemModel & itemModel) override;

private Q_SLOTS:
    void onAboutToAddItem();
    void onAddedItem(const QModelIndex & index);

    void onAboutToUpdateItem(const QModelIndex & index);
    void onUpdatedItem(const QModelIndex & index);

    void onAboutToRemoveItems();
    void onRemovedItems();

    void onRenameFavoritedItemAction();
    void onUnfavoriteItemAction();
    void onDeselectAction();
    void onShowFavoritedItemInfoAction();

    void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    QMenu * m_pFavoriteItemContextMenu = nullptr;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_FAVORITE_ITEM_VIEW_H
