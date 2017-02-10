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

#ifndef QUENTIER_VIEWS_SAVED_SEARCH_ITEM_VIEW_H
#define QUENTIER_VIEWS_SAVED_SEARCH_ITEM_VIEW_H

#include "ItemView.h"
#include <quentier/types/ErrorString.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class SavedSearchItemView: public ItemView
{
    Q_OBJECT
public:
    explicit SavedSearchItemView(QWidget * parent = Q_NULLPTR);

    virtual void setModel(QAbstractItemModel * pModel) Q_DECL_OVERRIDE;

    /**
     * @return valid model index if the selection exists and contains exactly one row and invalid model index otherwise
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

    virtual void selectionChanged(const QItemSelection & selected,
                                  const QItemSelection & deselected) Q_DECL_OVERRIDE;

    virtual void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void deleteItem(const QModelIndex & itemIndex, SavedSearchModel & model);

    void restoreLastSavedSelection(const SavedSearchModel & model);

    void selectionChangedImpl(const QItemSelection & selected,
                              const QItemSelection & deselected);

    void setFavoritedFlag(const QAction & action, const bool favorited);

    void prepareForSavedSearchModelChange();
    void postProcessSavedSearchModelChange();

private:
    QMenu *     m_pSavedSearchItemContextMenu;
    bool        m_trackingSelection;
    bool        m_modelReady;
};

} // namespace quentier

#endif // QUENTIER_VIEWS_SAVED_SEARCH_ITEM_VIEW_H
