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
#include "Utils.h"

#include <lib/model/favorites/FavoritesModel.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/SidePanelsFiltering.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <limits>
#include <set>
#include <utility>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view::FavoriteItemView", errorDescription);                 \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gFavoriteItemViewGroupKey = "FavoriteItemView"sv;

} // namespace

FavoriteItemView::FavoriteItemView(QWidget * parent) :
    AbstractNoteFilteringTreeView{QStringLiteral("favorited item"), parent}
{
    // This prevents the ugly behaviour when a favorited item is selected: due
    // to auto scroll the type column might get totally hidden if there's not
    // enough horizontal room to display the whole name of the selected item
    QAbstractItemView::setAutoScroll(false);
}

void FavoriteItemView::unfavoriteSelectedItems()
{
    QNDEBUG(
        "view::FavoriteItemView", "FavoriteItemView::unfavoriteSelectedItems");

    auto * favoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!favoritesModel)) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Non-favorites model has been set to the favorites item view");
        return;
    }

    const auto indexes = selectedIndexes();
    Q_ASSERT(indexes.size() <= std::numeric_limits<int>::max());

    std::set<int> rows;
    for (int i = 0, size = static_cast<int>(indexes.size()); i < size; ++i) {
        const auto & currentIndex = indexes.at(i);
        if (Q_UNLIKELY(!currentIndex.isValid())) {
            continue;
        }

        const int row = currentIndex.row();
        const auto it = rows.find(row);
        if (it != rows.end()) {
            continue;
        }

        Q_UNUSED(rows.insert(row))
    }

    if (rows.empty()) {
        QNDEBUG(
            "view::FavoriteItemView",
            "No favorited items are selected, nothing to unfavorite");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot unfavorite the favorited items"),
            tr("No favorited items are selected currently"),
            tr("Please select the favorited items you want to unfavorite")))

        return;
    }

    const int numRows = static_cast<int>(rows.size());
    if (favoritesModel->removeRows(*(rows.begin()), numRows)) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Successfully unfavorited the selected items");
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
    return QString::fromUtf8(
        gFavoriteItemViewGroupKey.data(), gFavoriteItemViewGroupKey.size());
}

QString FavoriteItemView::selectedItemsArrayKey() const
{
    return QStringLiteral("LastSelectedFavoritedItemsLocalUids");
}

QString FavoriteItemView::selectedItemsKey() const
{
    return QStringLiteral("LastSelectedFavoritedItemLocalUid");
}

bool FavoriteItemView::shouldFilterBySelectedItems(
    const Account & account) const
{
    ApplicationSettings appSettings{
        account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    const auto filterBySelectedFavoritedItem = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedFavoritedItems);

    appSettings.endGroup();

    if (!filterBySelectedFavoritedItem.isValid()) {
        return true;
    }

    return filterBySelectedFavoritedItem.toBool();
}

QStringList FavoriteItemView::localIdsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    const auto * favoritesModel = qobject_cast<const FavoritesModel *>(model());

    if (Q_UNLIKELY(!favoritesModel)) {
        QNWARNING("view::FavoriteItemView", "Non-favorites model is used");
        return {};
    }

    QStringList favoritedItemLocalIds;

    const auto & notebookLocalIds =
        noteFiltersManager.notebookLocalIdsInFilter();

    for (const auto & notebookLocalId: std::as_const(notebookLocalIds)) {
        const auto * item = favoritesModel->itemForLocalId(notebookLocalId);
        if (!item) {
            continue;
        }

        favoritedItemLocalIds << notebookLocalId;
    }

    const auto & tagLocalIds = noteFiltersManager.tagLocalIdsInFilter();

    for (const auto & tagLocalId: std::as_const(tagLocalIds)) {
        const auto * item = favoritesModel->itemForLocalId(tagLocalId);
        if (!item) {
            continue;
        }

        favoritedItemLocalIds << tagLocalId;
    }

    const auto & savedSearchLocalId =
        noteFiltersManager.savedSearchLocalIdInFilter();

    const auto * savedSearchItem =
        favoritesModel->itemForLocalId(savedSearchLocalId);

    if (savedSearchItem) {
        favoritedItemLocalIds << savedSearchLocalId;
    }

    return favoritedItemLocalIds;
}

void FavoriteItemView::setItemLocalIdsToNoteFiltersManager(
    const QStringList & itemLocalIds, NoteFiltersManager & noteFiltersManager)
{
    auto * favoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!favoritesModel)) {
        QNWARNING("view::FavoriteItemView", "Non-favorites model is used");
        return;
    }

    QString savedSearchLocalId;
    QStringList notebookLocalIds;
    QStringList tagLocalIds;

    for (const auto & itemLocalId: std::as_const(itemLocalIds)) {
        const auto * item = favoritesModel->itemForLocalId(itemLocalId);
        if (Q_UNLIKELY(!item)) {
            QNDEBUG(
                "view::FavoriteItemView",
                "Could not find favorited item for local id " << itemLocalId);
            continue;
        }

        switch (item->type()) {
        case FavoritesModelItem::Type::Notebook:
            notebookLocalIds << itemLocalId;
            break;
        case FavoritesModelItem::Type::Tag:
            tagLocalIds << itemLocalId;
            break;
        case FavoritesModelItem::Type::SavedSearch:
            savedSearchLocalId = itemLocalId;
            break;
        default:
            break;
        }
    }

    noteFiltersManager.setItemsToFilter(
        savedSearchLocalId, notebookLocalIds, tagLocalIds);
}

void FavoriteItemView::removeItemLocalIdsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    // NOTE: deliberately unimplemented as this method doesn't make much sense
    // for FavoritesItemView
    Q_UNUSED(noteFiltersManager)
}

void FavoriteItemView::connectToModel(AbstractItemModel & model)
{
    auto * favoritesModel = qobject_cast<FavoritesModel *>(&model);
    if (Q_UNLIKELY(!favoritesModel)) {
        QNWARNING("view::FavoriteItemView", "Non-favorites model is used");
        return;
    }

    QObject::connect(
        favoritesModel, &FavoritesModel::aboutToAddItem, this,
        &FavoriteItemView::onAboutToAddItem);

    QObject::connect(
        favoritesModel, &FavoritesModel::addedItem, this,
        &FavoriteItemView::onAddedItem);

    QObject::connect(
        favoritesModel, &FavoritesModel::aboutToUpdateItem, this,
        &FavoriteItemView::onAboutToUpdateItem);

    QObject::connect(
        favoritesModel, &FavoritesModel::updatedItem, this,
        &FavoriteItemView::onUpdatedItem);

    QObject::connect(
        favoritesModel, &FavoritesModel::aboutToRemoveItems, this,
        &FavoriteItemView::onAboutToRemoveItems);

    QObject::connect(
        favoritesModel, &FavoritesModel::removedItems, this,
        &FavoriteItemView::onRemovedItems);
}

void FavoriteItemView::deleteItem(
    const QModelIndex & itemIndex, AbstractItemModel & model)
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::deleteItem");

    const QString localId = model.localIdForItemIndex(itemIndex);
    if (Q_UNLIKELY(localId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the favorited item "
                       "meant to be unfavorited"))
        return;
    }

    if (model.removeRow(itemIndex.row(), itemIndex.parent())) {
        QNDEBUG("view::FavoriteItemView", "Successfully unfavorited the item");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The favorites model refused to unfavorite the item; "
           "Check the status bar for message from the favorites model "
           "explaining why the item could not be unfavorited")))
}

void FavoriteItemView::processSelectedItem(
    const QString & itemLocalId, AbstractItemModel & itemModel)
{
    const auto * favoritesModel =
        qobject_cast<const FavoritesModel *>(&itemModel);

    if (Q_UNLIKELY(!favoritesModel)) {
        return;
    }

    const auto * item = favoritesModel->itemForLocalId(itemLocalId);
    if (Q_UNLIKELY(!item)) {
        return;
    }

    if (item->type() != FavoritesModelItem::Type::Note) {
        return;
    }

    Q_EMIT favoritedNoteSelected(itemLocalId);
}

void FavoriteItemView::onAboutToAddItem()
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onAboutToAddItem");
    prepareForModelChange();
}

void FavoriteItemView::onAddedItem(const QModelIndex & index)
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onAddedItem");

    Q_UNUSED(index)
    postProcessModelChange();
}

void FavoriteItemView::onAboutToUpdateItem(const QModelIndex & index)
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onAboutToUpdateItem");

    Q_UNUSED(index)
    prepareForModelChange();
}

void FavoriteItemView::onUpdatedItem(const QModelIndex & index)
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onUpdatedItem");

    Q_UNUSED(index)
    postProcessModelChange();
}

void FavoriteItemView::onAboutToRemoveItems()
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onAboutToRemoveItems");
    prepareForModelChange();
}

void FavoriteItemView::onRemovedItems()
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onRemovedItems");
    postProcessModelChange();
}

void FavoriteItemView::onRenameFavoritedItemAction()
{
    QNDEBUG(
        "view::FavoriteItemView",
        "FavoriteItemView::onRenameFavoritedItemAction");

    auto * favoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!favoritesModel)) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Non-favorites model has been set to the favorites item view");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item, "
                       "can't get favorited item's local id from QAction"))
        return;
    }

    const auto itemIndex = favoritesModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename favorited item: "
                       "the model returned invalid index for "
                       "the favorited item's local id"))
        return;
    }

    edit(itemIndex);
}

void FavoriteItemView::onUnfavoriteItemAction()
{
    QNDEBUG(
        "view::FavoriteItemView", "FavoriteItemView::onUnfavoriteItemAction");

    auto * favoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!favoritesModel)) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Non-favorites model has been set to "
                << "the favorites item view");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item, can't cast "
                       "the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item, can't get "
                       "favorited item's local id from QAction"))
        return;
    }

    const auto itemIndex = favoritesModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite item: the model "
                       "returned invalid index for the favorited item's local "
                       "id"))
        return;
    }

    Q_UNUSED(favoritesModel->removeRow(itemIndex.row(), itemIndex.parent()))
}

void FavoriteItemView::onDeselectAction()
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::onDeselectAction");

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't clear the favorited items selection: "
                       "no selection model in the view"))
        return;
    }

    selectionModel->clearSelection();
}

void FavoriteItemView::onShowFavoritedItemInfoAction()
{
    QNDEBUG(
        "view::FavoriteItemView",
        "FavoriteItemView::onShowFavoritedItemInfoAction");

    Q_EMIT favoritedItemInfoRequested();
}

void FavoriteItemView::contextMenuEvent(QContextMenuEvent * event)
{
    QNDEBUG("view::FavoriteItemView", "FavoriteItemView::contextMenuEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "view::FavoriteItemView",
            "Detected Qt error: favorite item view received context menu event "
                << "with null pointer to the context menu event");
        return;
    }

    auto * favoritesModel = qobject_cast<FavoritesModel *>(model());
    if (Q_UNLIKELY(!favoritesModel)) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Non-favorites model has been set to the favorites item view");
        return;
    }

    const auto clickedItemIndex = indexAt(event->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::FavoriteItemView",
            "Clicked item index is not valid, not doing anything");
        return;
    }

    const auto * item = favoritesModel->itemAtRow(clickedItemIndex.row());
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the favorites model "
                       "item: no item corresponding to the clicked item's "
                       "index"))
        return;
    }

    delete m_favoriteItemContextMenu;
    m_favoriteItemContextMenu = new QMenu(this);

    const bool canUpdate =
        favoritesModel->flags(clickedItemIndex) & Qt::ItemIsEditable;

    addContextMenuAction(
        tr("Rename"), *m_favoriteItemContextMenu, this,
        [this] { onRenameFavoritedItemAction(); }, item->localId(),
        canUpdate ? ActionState::Enabled : ActionState::Disabled);

    addContextMenuAction(
        tr("Unfavorite"), *m_favoriteItemContextMenu, this,
        [this] { onUnfavoriteItemAction(); }, item->localId(),
        ActionState::Enabled);

    m_favoriteItemContextMenu->addSeparator();

    addContextMenuAction(
        tr("Clear selection"), *m_favoriteItemContextMenu, this,
        [this] { onDeselectAction(); }, QString{}, ActionState::Enabled);

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_favoriteItemContextMenu, this,
        [this] { onShowFavoritedItemInfoAction(); }, item->localId(),
        ActionState::Enabled);

    m_favoriteItemContextMenu->show();
    m_favoriteItemContextMenu->exec(event->globalPos());
}

} // namespace quentier
