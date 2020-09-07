/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_VIEW_ABSTRACT_MULTI_SELECTION_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_ABSTRACT_MULTI_SELECTION_ITEM_VIEW_H

#include "ItemView.h"

#include <quentier/types/ErrorString.h>

#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(ItemModel)
QT_FORWARD_DECLARE_CLASS(NoteFiltersManager)

/**
 * @brief The AbstractMultiSelectionItemView class is an abstract base class for
 * tree views supporting selection of multiple items simultaneously. Subclasses
 * of AbstractMultiSelectionItemView are intended to be used along with
 * ItemModel subclasses.
 */
class AbstractMultiSelectionItemView : public ItemView
{
    Q_OBJECT
public:
    explicit AbstractMultiSelectionItemView(
        const QString & modelTypeName, QWidget * parent = nullptr);

    virtual ~AbstractMultiSelectionItemView() override;

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);

    virtual void setModel(QAbstractItemModel * pModel) override;

    /**
     * @return      Valid model index if the selection exists and contains
     *              exactly one row and invalid model index otherwise
     */
    QModelIndex currentlySelectedItemIndex() const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

public Q_SLOTS:
    void deleteSelectedItem();

protected:
    /**
     * @brief saveItemsState method should persist expanded/collapsed states
     * of model items so that they can be restored later when needed
     */
    virtual void saveItemsState() = 0;

    /**
     * @brief restoreItemsState method should restore previously persisted
     * expanded/collapsed states of model items
     */
    virtual void restoreItemsState(const ItemModel & itemModel) = 0;

    /**
     * View's group key for ApplicationSettings entry to save/load selected
     * items
     */
    virtual QString selectedItemsGroupKey() const = 0;

    /**
     * Array key for ApplicationSettings entry to save/load selected items
     */
    virtual QString selectedItemsArrayKey() const = 0;

    /**
     * Item key for ApplicationSettings entry to save/load selected items
     */
    virtual QString selectedItemsKey() const = 0;

    /**
     * @brief shouldFilterBySelectedItems method tells whether filtering by
     * selected items should be enabled for this view
     */
    virtual bool shouldFilterBySelectedItems(const Account & account) const = 0;

    /**
     * @brief localUidsInNoteFiltersManager method provides local uids of items
     * which are used to filter notes via the passed in NoteFiltersManager
     */
    virtual QStringList localUidsInNoteFiltersManager(
        const NoteFiltersManager & noteFiltersManager) const = 0;

    /**
     * @brief setItemLocalUidsToNoteFiltersManager method is used to set local
     * uids of view's items to the passed in NoteFiltersManager
     */
    virtual void setItemLocalUidsToNoteFiltersManager(
        const QStringList & itemLocalUids,
        NoteFiltersManager & noteFiltersManager) = 0;

    /**
     * @brief removeItemLocalUidsFromNoteFiltersManager method is used to remove
     * local uids of view's items from the passed in NoteFiltersManager
     */
    virtual void removeItemLocalUidsFromNoteFiltersManager(
        NoteFiltersManager & noteFiltersManager) = 0;

    /**
     * @brief connectToModel method should connect model specific signals to
     * view specific views
     */
    virtual void connectToModel(ItemModel & itemModel) = 0;

    /**
     * @brief deleteItem method should attempt to delete the item pointed to
     * by the passed in index from the model
     */
    virtual void deleteItem(
        const QModelIndex & itemIndex, ItemModel & model) = 0;

private Q_SLOTS:
    void onAllItemsListed();
    void onItemCollapsedOrExpanded(const QModelIndex & index);
    void onNoteFilterChanged();
    void onNoteFiltersManagerReady();

    virtual void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

protected:
    void saveSelectedItems(
        const Account & account, const QStringList & itemLocalUids);

    void restoreSelectedItems(const ItemModel & model);

    void selectAllItemsRootItem(const ItemModel & model);

    void prepareForModelChange();
    void postProcessModelChange();

private:
    void disconnectFromNoteFiltersManagerFilterChanged();
    void connectToNoteFiltersManagerFilterChanged();

    void handleNoSelectedItems(const Account & account);

    void setItemsToNoteFiltersManager(const QStringList & itemLocalUids);
    void clearItemsFromNoteFiltersManager();

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

private:
    const QString m_modelTypeName;

    QPointer<NoteFiltersManager> m_pNoteFiltersManager;

    QStringList m_itemLocalUidsPendingNoteFiltersManagerReadiness;
    bool m_restoreSelectedItemsWhenNoteFiltersManagerReady = false;

    bool m_trackingItemsState = false;
    bool m_trackingSelection = false;
    bool m_modelReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_ABSTRACT_MULTI_SELECTION_ITEM_VIEW_H
