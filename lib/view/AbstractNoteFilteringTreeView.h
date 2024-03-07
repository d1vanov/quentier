/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "TreeView.h"

#include <quentier/types/ErrorString.h>

#include <QPointer>

namespace quentier {

class Account;
class AbstractItemModel;
class ApplicationSettings;
class NoteFiltersManager;

/**
 * @brief The AbstractNoteFilteringTreeView class is an abstract base class for
 * tree views supporting selection of either single or multiple items
 * simultaneously and allowing filtering of notes based on the selected items.
 * Subclasses of AbstractNoteFilteringTreeView are intended to be used along
 * with AbstractItemModel subclasses.
 */
class AbstractNoteFilteringTreeView : public TreeView
{
    Q_OBJECT
public:
    explicit AbstractNoteFilteringTreeView(
        QString modelTypeName, QWidget * parent = nullptr);

    ~AbstractNoteFilteringTreeView() override;

    void setNoteFiltersManager(NoteFiltersManager & noteFiltersManager);
    void setModel(QAbstractItemModel * model) override;

    /**
     * @return      Valid model index if the selection exists and contains
     *              exactly one row and invalid model index otherwise
     */
    [[nodiscard]] QModelIndex currentlySelectedItemIndex() const;

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
    virtual void restoreItemsState(const AbstractItemModel & itemModel) = 0;

    /**
     * View's group key for ApplicationSettings entry to save/load selected
     * items
     */
    [[nodiscard]] virtual QString selectedItemsGroupKey() const = 0;

    /**
     * Array key for ApplicationSettings entry to save/load selected items
     */
    [[nodiscard]] virtual QString selectedItemsArrayKey() const = 0;

    /**
     * Item key for ApplicationSettings entry to save/load selected items
     */
    [[nodiscard]] virtual QString selectedItemsKey() const = 0;

    /**
     * @brief shouldFilterBySelectedItems method tells whether filtering by
     * selected items should be enabled for this view
     */
    [[nodiscard]] virtual bool shouldFilterBySelectedItems(
        const Account & account) const = 0;

    /**
     * @brief localIdsInNoteFiltersManager method provides local uids of items
     * which are used to filter notes via the passed in NoteFiltersManager
     */
    [[nodiscard]] virtual QStringList localIdsInNoteFiltersManager(
        const NoteFiltersManager & noteFiltersManager) const = 0;

    /**
     * @brief setItemLocalIdsToNoteFiltersManager method is used to set local
     * uids of view's items to the passed in NoteFiltersManager
     */
    virtual void setItemLocalIdsToNoteFiltersManager(
        const QStringList & itemLocalIds,
        NoteFiltersManager & noteFiltersManager) = 0;

    /**
     * @brief removeItemLocalIdsFromNoteFiltersManager method is used to remove
     * local uids of view's items from the passed in NoteFiltersManager
     */
    virtual void removeItemLocalIdsFromNoteFiltersManager(
        NoteFiltersManager & noteFiltersManager) = 0;

    /**
     * @brief connectToModel method should connect model specific signals to
     * view specific views
     */
    virtual void connectToModel(AbstractItemModel & itemModel) = 0;

    /**
     * @brief deleteItem method should attempt to delete the item pointed to
     * by the passed in index from the model
     */
    virtual void deleteItem(
        const QModelIndex & itemIndex, AbstractItemModel & model) = 0;

    /**
     * @brief processSelectedItem method is an optional method which
     * the subclass can implement if it needs to do any kind of special
     * processing on the selected items
     */
    virtual void processSelectedItem(
        const QString & itemLocalId, AbstractItemModel & itemModel)
    {
        Q_UNUSED(itemLocalId)
        Q_UNUSED(itemModel)
    }

private Q_SLOTS:
    void onAllItemsListed();
    void onItemCollapsedOrExpanded(const QModelIndex & index);
    void onNoteFilterChanged();
    void onNoteFiltersManagerReady();

    void selectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected) override;

protected:
    void saveSelectedItems(
        const Account & account, const QStringList & itemLocalIds);

    void restoreSelectedItems(const AbstractItemModel & model);

    void selectAllItemsRootItem(const AbstractItemModel & model);

    void prepareForModelChange();
    void postProcessModelChange();

    [[nodiscard]] bool trackItemsStateEnabled() const noexcept;
    void setTrackItemsStateEnabled(bool enabled) noexcept;

    [[nodiscard]] bool trackSelectionEnabled() const noexcept;
    void setTrackSelectionEnabled(bool enabled) noexcept;

    void saveAllItemsRootItemExpandedState(
        ApplicationSettings & appSettings, const QString & settingsKey,
        const QModelIndex & allItemsRootItemIndex);

private:
    void disconnectFromNoteFiltersManagerFilterChanged();
    void connectToNoteFiltersManagerFilterChanged();

    void handleNoSelectedItems(const Account & account);

    void setItemsToNoteFiltersManager(const QStringList & itemLocalIds);
    void clearItemsFromNoteFiltersManager();

    void selectionChangedImpl(
        const QItemSelection & selected, const QItemSelection & deselected);

private:
    const QString m_modelTypeName;

    QPointer<NoteFiltersManager> m_noteFiltersManager;

    QStringList m_itemLocalIdsPendingNoteFiltersManagerReadiness;
    bool m_restoreSelectedItemsWhenNoteFiltersManagerReady = false;

    bool m_trackingItemsState = false;
    bool m_trackingSelection = false;
    bool m_modelReady = false;
};

} // namespace quentier
