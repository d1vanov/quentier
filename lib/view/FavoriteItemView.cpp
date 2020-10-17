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

#include "FavoriteItemView.h"

#include <lib/model/favorites/FavoritesModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <set>

#define LAST_SELECTED_FAVORITED_ITEM_KEY                                       \
    QStringLiteral("LastSelectedFavoritedItemLocalUid")

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:favorites", errorDescription);                         \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

FavoriteItemView::FavoriteItemView(QWidget * parent) : ItemView(parent) {}

void FavoriteItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG("view:favorites", "FavoriteItemView::setModel");

    auto * pPreviousModel = qobject_cast<FavoritesModel *>(model());
    if (pPreviousModel) {
        QObject::disconnect(
            pPreviousModel, &FavoritesModel::notifyError, this,
            &FavoriteItemView::notifyError);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::notifyAllItemsListed, this,
            &FavoriteItemView::onAllItemsListed);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::aboutToAddItem, this,
            &FavoriteItemView::onAboutToAddItem);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::addedItem, this,
            &FavoriteItemView::onAddedItem);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::aboutToUpdateItem, this,
            &FavoriteItemView::onAboutToUpdateItem);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::updatedItem, this,
            &FavoriteItemView::onUpdatedItem);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::aboutToRemoveItems, this,
            &FavoriteItemView::onAboutToRemoveItems);

        QObject::disconnect(
            pPreviousModel, &FavoritesModel::removedItems, this,
            &FavoriteItemView::onRemovedItems);
    }

    m_modelReady = false;
    m_trackingSelection = false;

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(pModel);
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(
        pFavoritesModel, &FavoritesModel::notifyError, this,
        &FavoriteItemView::notifyError);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::aboutToAddItem, this,
        &FavoriteItemView::onAboutToAddItem);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::addedItem, this,
        &FavoriteItemView::onAddedItem);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::aboutToUpdateItem, this,
        &FavoriteItemView::onAboutToUpdateItem);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::updatedItem, this,
        &FavoriteItemView::onUpdatedItem);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::aboutToRemoveItems, this,
        &FavoriteItemView::onAboutToRemoveItems);

    QObject::connect(
        pFavoritesModel, &FavoritesModel::removedItems, this,
        &FavoriteItemView::onRemovedItems);

    ItemView::setModel(pModel);

    if (pFavoritesModel->allItemsListed()) {
        QNDEBUG(
            "view:favorites",
            "All favorites model's items are already "
                << "listed within the model");

        restoreLastSavedSelection(*pFavoritesModel);
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(
        pFavoritesModel, &FavoritesModel::notifyAllItemsListed, this,
        &FavoriteItemView::onAllItemsListed);
}

QModelIndex FavoriteItemView::currentlySelectedItemIndex() const
{
    QNDEBUG("view:favorites", "FavoriteItemView::currentlySelectedItemIndex");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return {};
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("view:favorites", "No selection model in the view");
        return {};
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("view:favorites", "The selection contains no model indexes");
        return {};
    }

    return singleRow(
        indexes, *pFavoritesModel,
        static_cast<int>(FavoritesModel::Column::DisplayName));
}

void FavoriteItemView::deleteSelectedItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::deleteSelectedItems");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return;
    }

    auto indexes = selectedIndexes();
    std::set<int> rows;
    for (int i = 0, size = indexes.size(); i < size; ++i) {
        const auto & currentIndex = indexes.at(i);
        if (Q_UNLIKELY(!currentIndex.isValid())) {
            continue;
        }

        int row = currentIndex.row();
        auto it = rows.find(row);
        if (it != rows.end()) {
            continue;
        }

        Q_UNUSED(rows.insert(row))
    }

    if (rows.empty()) {
        QNDEBUG(
            "view:favorites",
            "No favorited items are selected, nothing to "
                << "unfavorite");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot unfavorite the favorited items"),
            tr("No favorited items are selected currently"),
            tr("Please select the favorited items you want to unfavorite")))

        return;
    }

    int numRows = static_cast<int>(rows.size());
    bool res = pFavoritesModel->removeRows(*(rows.begin()), numRows);
    if (res) {
        QNDEBUG(
            "view:favorites",
            "Successfully unfavorited the selected "
                << "items");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The favorited items model refused to unfavorite the selected "
           "item(s); Check the status bar for message from the favorites model "
           "explaining why the items could not be unfavorited")))
}

void FavoriteItemView::onAllItemsListed()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAllItemsListed");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't cast the model set to the favorites item "
                       "view to the favorites model"))
        return;
    }

    QObject::disconnect(
        pFavoritesModel, &FavoritesModel::notifyAllItemsListed, this,
        &FavoriteItemView::onAllItemsListed);

    restoreLastSavedSelection(*pFavoritesModel);
    m_modelReady = true;
    m_trackingSelection = true;
}

void FavoriteItemView::onAboutToAddItem()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToAddItem");
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onAddedItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAddedItem");

    Q_UNUSED(index)
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onAboutToUpdateItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToUpdateItem");

    Q_UNUSED(index)
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onUpdatedItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onUpdatedItem");

    Q_UNUSED(index)
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onAboutToRemoveItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToRemoveItems");
    prepareForFavoritesModelChange();
}

void FavoriteItemView::onRemovedItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onRemovedItems");
    postProcessFavoritedModelChange();
}

void FavoriteItemView::onRenameFavoritedItemAction()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onRenameFavoritedItemAction");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item, "
                       "can't get favorited item's local uid from QAction"))
        return;
    }

    auto itemIndex = pFavoritesModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item: "
                       "the model returned invalid index for "
                       "the favorited item's local uid"))
        return;
    }

    edit(itemIndex);
}

void FavoriteItemView::onUnfavoriteItemAction()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onUnfavoriteItemAction");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item, can't get "
                       "favorited item's local uid from QAction"))
        return;
    }

    auto itemIndex = pFavoritesModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item: the model "
                       "returned invalid index for the favorited item's local "
                       "uid"))
        return;
    }

    Q_UNUSED(pFavoritesModel->removeRow(itemIndex.row(), itemIndex.parent()))
}

void FavoriteItemView::onDeselectAction()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onDeselectAction");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't clear the favorited items selection: "
                       "no selection model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void FavoriteItemView::onShowFavoritedItemInfoAction()
{
    QNDEBUG(
        "view:favorites", "FavoriteItemView::onShowFavoritedItemInfoAction");

    Q_EMIT favoritedItemInfoRequested();
}

void FavoriteItemView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("view:favorites", "FavoriteItemView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void FavoriteItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:favorites", "FavoriteItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "view:favorites",
            "Detected Qt error: favorite item view "
                << "received context menu event with null pointer to the "
                   "context "
                << "menu event");
        return;
    }

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view:favorites",
            "Clicked item index is not valid, not doing "
                << "anything");
        return;
    }

    const auto * pItem = pFavoritesModel->itemAtRow(clickedItemIndex.row());
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the favorites model "
                       "item: no item corresponding to the clicked item's "
                       "index"))
        return;
    }

    delete m_pFavoriteItemContextMenu;
    m_pFavoriteItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction, &QAction::triggered, this, &FavoriteItemView::slot);      \
        menu->addAction(pAction);                                              \
    }

    bool canUpdate =
        (pFavoritesModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"), m_pFavoriteItemContextMenu, onRenameFavoritedItemAction,
        pItem->localUid(), canUpdate);

    ADD_CONTEXT_MENU_ACTION(
        tr("Unfavorite"), m_pFavoriteItemContextMenu, onUnfavoriteItemAction,
        pItem->localUid(), true);

    m_pFavoriteItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Clear selection"), m_pFavoriteItemContextMenu, onDeselectAction,
        QString(), true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."), m_pFavoriteItemContextMenu,
        onShowFavoritedItemInfoAction, pItem->localUid(), true);

    m_pFavoriteItemContextMenu->show();
    m_pFavoriteItemContextMenu->exec(pEvent->globalPos());
}

void FavoriteItemView::restoreLastSavedSelection(const FavoritesModel & model)
{
    QNDEBUG("view:favorites", "FavoriteItemView::restoreLastSavedSelection");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore the last selected favorited model "
                       "item: no selection model in the view"))
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("FavoriteItemView"));

    QString lastSelectedItemLocalUid =
        appSettings.value(LAST_SELECTED_FAVORITED_ITEM_KEY).toString();

    appSettings.endGroup();

    if (lastSelectedItemLocalUid.isEmpty()) {
        QNDEBUG(
            "view:favorites",
            "Found no last selected favorited item local "
                << "uid");
        return;
    }

    QNTRACE(
        "view:favorites",
        "Last selected item local uid: " << lastSelectedItemLocalUid);

    auto lastSelectedItemIndex =
        model.indexForLocalUid(lastSelectedItemLocalUid);

    if (!lastSelectedItemIndex.isValid()) {
        QNDEBUG(
            "view:favorites",
            "Favorites model returned invalid index for "
                << "the last selected favorited item local uid");
        return;
    }

    pSelectionModel->select(
        lastSelectedItemIndex,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void FavoriteItemView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNTRACE("view:favorites", "FavoriteItemView::selectionChangedImpl");

    if (!m_trackingSelection) {
        QNTRACE(
            "view:favorites",
            "Not tracking selection at this time, "
                << "skipping");
        return;
    }

    Q_UNUSED(deselected)

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG("view:favorites", "Non-favorited items model is used");
        return;
    }

    auto selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        QNDEBUG("view:favorites", "The new selection is empty");
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    auto sourceIndex = singleRow(
        selectedIndexes, *pFavoritesModel,
        static_cast<int>(FavoritesModel::Column::DisplayName));

    if (!sourceIndex.isValid()) {
        QNDEBUG("view:favorites", "Not exactly one row is selected");
        return;
    }

    const auto * pItem = pFavoritesModel->itemAtRow(sourceIndex.row());
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the favorited model "
                       "item corresponding to the selected index"))
        return;
    }

    ApplicationSettings appSettings(
        pFavoritesModel->account(), QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(QStringLiteral("FavoriteItemView"));
    appSettings.setValue(LAST_SELECTED_FAVORITED_ITEM_KEY, pItem->localUid());
    appSettings.endGroup();

    QNDEBUG(
        "view:favorites",
        "Persisted the currently selected favorited "
            << "item's local uid: " << pItem->localUid());
}

void FavoriteItemView::prepareForFavoritesModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:favorites", "The model is not ready yet");
        return;
    }

    m_trackingSelection = false;
}

void FavoriteItemView::postProcessFavoritedModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:favorites", "The model is not ready yet");
        return;
    }

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG("view:favorites", "Non-favorited items model is used");
        return;
    }

    restoreLastSavedSelection(*pFavoritesModel);
    m_trackingSelection = true;
}

} // namespace quentier
