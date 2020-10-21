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
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <set>

#define FAVORITE_ITEM_VIEW_GROUP_KEY QStringLiteral("FavoriteItemView")

#define LAST_SELECTED_FAVORITED_ITEM_KEY                                       \
    QStringLiteral("LastSelectedFavoritedItemLocalUid")

#define LAST_SELECTED_FAVORITED_ITEMS_KEY                                      \
    QStringLiteral("LastSelectedFavoritedItemsLocalUids")

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:favorites", errorDescription);                         \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

FavoriteItemView::FavoriteItemView(QWidget * parent) :
    AbstractMultiSelectionItemView(QStringLiteral("favorited item"), parent)
{
    // This prevents the ugly behaviour when a favorited item is selected: due
    // to auto scroll the type column might get totally hidden if there's not
    // enough horizontal room to display the whole name of the selected item
    QAbstractItemView::setAutoScroll(false);
}

void FavoriteItemView::unfavoriteSelectedItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::unfavoriteSelectedItems");

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

QString FavoriteItemView::selectedItemsGroupKey() const
{
    return FAVORITE_ITEM_VIEW_GROUP_KEY;
}

QString FavoriteItemView::selectedItemsArrayKey() const
{
    return LAST_SELECTED_FAVORITED_ITEMS_KEY;
}

QString FavoriteItemView::selectedItemsKey() const
{
    return LAST_SELECTED_FAVORITED_ITEM_KEY;
}

bool FavoriteItemView::shouldFilterBySelectedItems(
    const Account & account) const
{
    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(SIDE_PANELS_FILTER_BY_SELECTION_SETTINGS_GROUP_NAME);

    const auto filterBySelectedFavoritedItem =
        appSettings.value(FILTER_BY_SELECTED_FAVORITED_ITEM_SETTINGS_KEY);

    appSettings.endGroup();

    if (!filterBySelectedFavoritedItem.isValid()) {
        return true;
    }

    return filterBySelectedFavoritedItem.toBool();
}

QStringList FavoriteItemView::localUidsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    const auto * pFavoritesModel =
        qobject_cast<const FavoritesModel *>(model());

    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNWARNING("view:favorites", "Non-favorites model is used");
        return {};
    }

    QStringList favoritedItemLocalUids;

    const auto & notebookLocalUids =
        noteFiltersManager.notebookLocalUidsInFilter();

    for (const auto & notebookLocalUid: qAsConst(notebookLocalUids)) {
        const auto * pItem = pFavoritesModel->itemForLocalUid(notebookLocalUid);
        if (!pItem) {
            continue;
        }

        favoritedItemLocalUids << notebookLocalUid;
    }

    const auto & tagLocalUids =
        noteFiltersManager.tagLocalUidsInFilter();

    for (const auto & tagLocalUid: qAsConst(tagLocalUids)) {
        const auto * pItem = pFavoritesModel->itemForLocalUid(tagLocalUid);
        if (!pItem) {
            continue;
        }

        favoritedItemLocalUids << tagLocalUid;
    }

    const auto & savedSearchLocalUid =
        noteFiltersManager.savedSearchLocalUidInFilter();

    const auto * pSavedSearchItem =
        pFavoritesModel->itemForLocalUid(savedSearchLocalUid);

    if (pSavedSearchItem) {
        favoritedItemLocalUids << savedSearchLocalUid;
    }

    return favoritedItemLocalUids;
}

void FavoriteItemView::setItemLocalUidsToNoteFiltersManager(
    const QStringList & itemLocalUids,
    NoteFiltersManager & noteFiltersManager)
{
    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNWARNING("view:favorites", "Non-favorites model is used");
        return;
    }

    QString savedSearchLocalUid;
    QStringList notebookLocalUids;
    QStringList tagLocalUids;

    for (const auto & itemLocalUid: qAsConst(itemLocalUids))
    {
        const auto * pItem = pFavoritesModel->itemForLocalUid(itemLocalUid);
        if (Q_UNLIKELY(!pItem)) {
            QNDEBUG(
                "view:favorites",
                "Could not find favorited item for local uid " << itemLocalUid);
            continue;
        }

        switch (pItem->type())
        {
        case FavoritesModelItem::Type::Notebook:
            notebookLocalUids << itemLocalUid;
            break;
        case FavoritesModelItem::Type::Tag:
            tagLocalUids << itemLocalUid;
            break;
        case FavoritesModelItem::Type::SavedSearch:
            savedSearchLocalUid = itemLocalUid;
            break;
        default:
            break;
        }
    }

    noteFiltersManager.setItemsToFilter(
        savedSearchLocalUid,
        notebookLocalUids,
        tagLocalUids);
}

void FavoriteItemView::removeItemLocalUidsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    // NOTE: deliberately unimplemented as this method doesn't make much sense
    // for FavoritesItemView
    Q_UNUSED(noteFiltersManager)
}

void FavoriteItemView::connectToModel(ItemModel & model)
{
    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(&model);
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNWARNING("view:favorites", "Non-favorites model is used");
        return;
    }

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
}

void FavoriteItemView::deleteItem(
    const QModelIndex & itemIndex, ItemModel & model)
{
    QNDEBUG("view:favorites", "FavoriteItemView::deleteItem");

    QString localUid = model.localUidForItemIndex(itemIndex);
    if (Q_UNLIKELY(localUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the favorited item "
                       "meant to be unfavorited"))
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG("view:favorites", "Successfully unfavorited the item");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The favorites model refused to unfavorite the item; "
           "Check the status bar for message from the favorites model "
           "explaining why the item could not be unfavorited")))
}

void FavoriteItemView::processSelectedItem(
    const QString & itemLocalUid, ItemModel & itemModel)
{
    const auto * pFavoritesModel =
        qobject_cast<const FavoritesModel *>(&itemModel);

    if (Q_UNLIKELY(!pFavoritesModel)) {
        return;
    }

    const auto * pItem = pFavoritesModel->itemForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!pItem)) {
        return;
    }

    if (pItem->type() != FavoritesModelItem::Type::Note) {
        return;
    }

    Q_EMIT favoritedNoteSelected(itemLocalUid);
}

void FavoriteItemView::onAboutToAddItem()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToAddItem");
    prepareForModelChange();
}

void FavoriteItemView::onAddedItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAddedItem");

    Q_UNUSED(index)
    postProcessModelChange();
}

void FavoriteItemView::onAboutToUpdateItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToUpdateItem");

    Q_UNUSED(index)
    prepareForModelChange();
}

void FavoriteItemView::onUpdatedItem(const QModelIndex & index)
{
    QNDEBUG("view:favorites", "FavoriteItemView::onUpdatedItem");

    Q_UNUSED(index)
    postProcessModelChange();
}

void FavoriteItemView::onAboutToRemoveItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onAboutToRemoveItems");
    prepareForModelChange();
}

void FavoriteItemView::onRemovedItems()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onRemovedItems");
    postProcessModelChange();
}

void FavoriteItemView::onRenameFavoritedItemAction()
{
    QNDEBUG("view:favorites", "FavoriteItemView::onRenameFavoritedItemAction");

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to the favorites item view");
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

void FavoriteItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:favorites", "FavoriteItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "view:favorites",
            "Detected Qt error: favorite item view received context menu event "
                << "with null pointer to the context menu event");
        return;
    }

    auto * pFavoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!pFavoritesModel)) {
        QNDEBUG(
            "view:favorites",
            "Non-favorites model has been set to the favorites item view");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view:favorites",
            "Clicked item index is not valid, not doing anything");
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

} // namespace quentier
