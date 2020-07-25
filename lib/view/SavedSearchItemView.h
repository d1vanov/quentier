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

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class SavedSearchItemView: public ItemView
{
    Q_OBJECT
public:
    explicit SavedSearchItemView(QWidget * parent = nullptr);

    virtual void setModel(QAbstractItemModel * pModel) override;

    /**
     * @return      Valid model index if the selection exists and contains
     *              exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(ErrorString error);
    void newSavedSearchCreationRequested();
    void savedSearchInfoRequested();

public Q_SLOTS:
    void deleteSelectedItem();

private Q_SLOTS:
    void onAllSavedSearchesListed();

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

    virtual void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) override;

private:
    void deleteItem(const QModelIndex & itemIndex, SavedSearchModel & model);

    void restoreLastSavedSelection(const SavedSearchModel & model);

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

    void setFavoritedFlag(const QAction & action, const bool favorited);

    void prepareForSavedSearchModelChange();
    void postProcessSavedSearchModelChange();

private:
    QMenu *     m_pSavedSearchItemContextMenu = nullptr;
    bool        m_trackingSelection = false;
    bool        m_modelReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_SAVED_SEARCH_ITEM_VIEW_H
