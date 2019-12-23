/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FavoritesModel)

class FavoriteItemView: public ItemView
{
    Q_OBJECT
public:
    explicit FavoriteItemView(QWidget * parent = nullptr);

    virtual void setModel(QAbstractItemModel * pModel) override;

    /**
     * @return          Valid model index if the selection exists and contains
     *                  exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(ErrorString error);
    void favoritedItemInfoRequested();

public Q_SLOTS:
    void deleteSelectedItems();

private Q_SLOTS:
    void onAllItemsListed();

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

    virtual void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void restoreLastSavedSelection(const FavoritesModel & model);

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

    void prepareForFavoritesModelChange();
    void postProcessFavoritedModelChange();

private:
    QMenu *     m_pFavoriteItemContextMenu;
    bool        m_trackingSelection;
    bool        m_modelReady;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_FAVORITE_ITEM_VIEW_H
