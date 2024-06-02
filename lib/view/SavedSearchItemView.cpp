/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "SavedSearchItemView.h"
#include "Utils.h"

#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/SidePanelsFiltering.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <memory>

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view::SavedSearchItemView", errorDescription);              \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gSavedSearchItemViewGroupKey = "SavedSearchItemView"sv;

constexpr auto gAllSavedSearchesRootItemExpandedKey =
    "AllSavedSearchesRootItemExpanded"sv;

} // namespace

SavedSearchItemView::SavedSearchItemView(QWidget * parent) :
    AbstractNoteFilteringTreeView{QStringLiteral("saved search"), parent}
{
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void SavedSearchItemView::saveItemsState()
{
    QNDEBUG("view::SavedSearchItemView", "SavedSearchItemView::saveItemsState");

    const auto * savedSearchModel =
        qobject_cast<const SavedSearchModel *>(model());

    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    ApplicationSettings appSettings{
        savedSearchModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gSavedSearchItemViewGroupKey);

    saveAllItemsRootItemExpandedState(
        appSettings, gAllSavedSearchesRootItemExpandedKey,
        savedSearchModel->allItemsRootItemIndex());

    appSettings.endGroup();
}

void SavedSearchItemView::restoreItemsState(const AbstractItemModel & model)
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::restoreItemsState");

    const auto * savedSearchModel =
        qobject_cast<const SavedSearchModel *>(&model);

    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    ApplicationSettings appSettings{
        model.account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(gSavedSearchItemViewGroupKey);

    const auto allSavedSearchesRootItemExpandedPreference =
        appSettings.value(gAllSavedSearchesRootItemExpandedKey);

    appSettings.endGroup();

    const bool wasTrackingTagItemsState = trackItemsStateEnabled();
    setTrackItemsStateEnabled(false);

    bool allSavedSearchesRootItemExpanded = true;
    if (allSavedSearchesRootItemExpandedPreference.isValid()) {
        allSavedSearchesRootItemExpanded =
            allSavedSearchesRootItemExpandedPreference.toBool();
    }

    const auto allTagsRootItemIndex = savedSearchModel->allItemsRootItemIndex();
    setExpanded(allTagsRootItemIndex, allSavedSearchesRootItemExpanded);

    setTrackItemsStateEnabled(wasTrackingTagItemsState);
}

QString SavedSearchItemView::selectedItemsGroupKey() const
{
    return QString::fromUtf8(
        gSavedSearchItemViewGroupKey.data(),
        gSavedSearchItemViewGroupKey.size());
}

QString SavedSearchItemView::selectedItemsArrayKey() const
{
    return QStringLiteral("LastSelectedSavedSearchLocalUids");
}

QString SavedSearchItemView::selectedItemsKey() const
{
    return QStringLiteral("LastSelectedSavedSearchLocalUid");
}

bool SavedSearchItemView::shouldFilterBySelectedItems(
    const Account & account) const
{
    ApplicationSettings appSettings{
        account, preferences::keys::files::userInterface};

    appSettings.beginGroup(preferences::keys::sidePanelsFilterBySelectionGroup);

    const auto filterBySelectedSavedSearch = appSettings.value(
        preferences::keys::sidePanelsFilterBySelectedSavedSearch);

    appSettings.endGroup();

    if (!filterBySelectedSavedSearch.isValid()) {
        return true;
    }

    return filterBySelectedSavedSearch.toBool();
}

QStringList SavedSearchItemView::localIdsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    const auto & savedSearchLocalId =
        noteFiltersManager.savedSearchLocalIdInFilter();

    if (savedSearchLocalId.isEmpty()) {
        return {};
    }

    return QStringList() << savedSearchLocalId;
}

void SavedSearchItemView::setItemLocalIdsToNoteFiltersManager(
    const QStringList & itemLocalIds, NoteFiltersManager & noteFiltersManager)
{
    if (Q_UNLIKELY(itemLocalIds.isEmpty())) {
        noteFiltersManager.removeSavedSearchFromFilter();
        return;
    }

    noteFiltersManager.setSavedSearchLocalIdToFilter(itemLocalIds[0]);
}

void SavedSearchItemView::removeItemLocalIdsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.removeSavedSearchFromFilter();
}

void SavedSearchItemView::connectToModel(AbstractItemModel & model)
{
    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(&model);
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNWARNING(
            "view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    QObject::connect(
        savedSearchModel, &SavedSearchModel::aboutToAddSavedSearch, this,
        &SavedSearchItemView::onAboutToAddSavedSearch);

    QObject::connect(
        savedSearchModel, &SavedSearchModel::addedSavedSearch, this,
        &SavedSearchItemView::onAddedSavedSearch);

    QObject::connect(
        savedSearchModel, &SavedSearchModel::aboutToUpdateSavedSearch, this,
        &SavedSearchItemView::onAboutToUpdateSavedSearch);

    QObject::connect(
        savedSearchModel, &SavedSearchModel::updatedSavedSearch, this,
        &SavedSearchItemView::onUpdatedSavedSearch);

    QObject::connect(
        savedSearchModel, &SavedSearchModel::aboutToRemoveSavedSearches, this,
        &SavedSearchItemView::onAboutToRemoveSavedSearches);

    QObject::connect(
        savedSearchModel, &SavedSearchModel::removedSavedSearches, this,
        &SavedSearchItemView::onRemovedSavedSearches);
}

void SavedSearchItemView::deleteItem(
    const QModelIndex & itemIndex, AbstractItemModel & model)
{
    QNDEBUG("view::SavedSearchItemView", "SavedSearchItemView::deleteItem");

    const QString localId = model.localIdForItemIndex(itemIndex);
    if (Q_UNLIKELY(localId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the saved search item "
                       "meant to be deleted"))
        return;
    }

    const int confirm = warningMessageBox(
        this, tr("Confirm the saved search deletion"),
        tr("Are you sure you want to delete this saved search?"),
        tr("Note that this action is not reversible!"),
        QMessageBox::Ok | QMessageBox::No);

    if (confirm != QMessageBox::Ok) {
        QNDEBUG(
            "view::SavedSearchItemView",
            "Saved search deletion was not confirmed");
        return;
    }

    if (model.removeRow(itemIndex.row(), itemIndex.parent())) {
        QNDEBUG(
            "view::SavedSearchItemView", "Successfully deleted saved search");
        return;
    }

    Q_UNUSED(internalErrorMessageBox(
        this,
        tr("The saved search model refused to delete the saved search; "
           "Check the status bar for message from the saved search model "
           "explaining why the saved search could not be deleted")))
}

void SavedSearchItemView::onAboutToAddSavedSearch()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onAboutToAddSavedSearch");

    prepareForModelChange();
}

void SavedSearchItemView::onAddedSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::onAddedSavedSearch");

    Q_UNUSED(index)
    postProcessModelChange();
}

void SavedSearchItemView::onAboutToUpdateSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onAboutToUpdateSavedSearch");

    Q_UNUSED(index)
    prepareForModelChange();
}

void SavedSearchItemView::onUpdatedSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onUpdatedSavedSearch");

    Q_UNUSED(index)
    postProcessModelChange();
}

void SavedSearchItemView::onAboutToRemoveSavedSearches()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onAboutToRemoveSavedSearches");

    prepareForModelChange();
}

void SavedSearchItemView::onRemovedSavedSearches()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onRemovedSavedSearches");

    postProcessModelChange();
}

void SavedSearchItemView::onCreateNewSavedSearchAction()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onCreateNewSavedSearchAction");

    Q_EMIT newSavedSearchCreationRequested();
}

void SavedSearchItemView::onRenameSavedSearchAction()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onRenameSavedSearchAction");

    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search, "
                       "can't get saved search's local id from QAction"))
        return;
    }

    const auto itemIndex = savedSearchModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search: the model "
                       "returned invalid index for the saved search's local "
                       "id"))
        return;
    }

    edit(itemIndex);
}

void SavedSearchItemView::onDeleteSavedSearchAction()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onDeleteSavedSearchAction");

    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used")
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search, "
                       "can't get saved search's local id from QAction"))
        return;
    }

    const auto itemIndex = savedSearchModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search: the model "
                       "returned invalid index for the saved search's local "
                       "id"))
        return;
    }

    deleteItem(itemIndex, *savedSearchModel);
}

void SavedSearchItemView::onEditSavedSearchAction()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onEditSavedSearchAction");

    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    const QString itemLocalId = action->data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit saved search, "
                       "can't get saved search's local id from QAction"))
        return;
    }

    const auto editSavedSearchDialog =
        std::make_unique<AddOrEditSavedSearchDialog>(
            savedSearchModel, this, itemLocalId);

    editSavedSearchDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(editSavedSearchDialog->exec())
}

void SavedSearchItemView::onShowSavedSearchInfoAction()
{
    QNDEBUG(
        "view::SavedSearchItemView",
        "SavedSearchItemView::onShowSavedSearchInfoAction");

    Q_EMIT savedSearchInfoRequested();
}

void SavedSearchItemView::onDeselectAction()
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::onDeselectAction");

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't clear the saved search selection: "
                       "no selection model in the view"))
        return;
    }

    selectionModel->clearSelection();
}

void SavedSearchItemView::onFavoriteAction()
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::onFavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, true);
}

void SavedSearchItemView::onUnfavoriteAction()
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::onUnfavoriteAction");

    auto * action = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!action)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*action, false);
}

void SavedSearchItemView::contextMenuEvent(QContextMenuEvent * event)
{
    QNDEBUG(
        "view::SavedSearchItemView", "SavedSearchItemView::contextMenuEvent");

    if (Q_UNLIKELY(!event)) {
        QNWARNING(
            "view::SavedSearchItemView",
            "Detected Qt error: saved search item view received context menu "
            "event with null pointer to the context menu event");
        return;
    }

    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    const auto clickedItemIndex = indexAt(event->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view::SavedSearchItemView",
            "Clicked item index is not valid, not doing anything");
        return;
    }

    const auto * item = savedSearchModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the saved search "
                       "model item: no item corresponding to the clicked "
                       "item's index"))
        return;
    }

    const auto * savedSearchItem = item->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!savedSearchItem)) {
        QNDEBUG(
            "view::SavedSearchItemView",
            "Ignoring the context menu event for non-saved search item");
        return;
    }

    delete m_savedSearchItemContextMenu;
    m_savedSearchItemContextMenu = new QMenu(this);

    addContextMenuAction(
        tr("Create new saved search") + QStringLiteral("..."),
        *m_savedSearchItemContextMenu, this,
        [this] { onCreateNewSavedSearchAction(); }, savedSearchItem->localId(),
        ActionState::Enabled);

    m_savedSearchItemContextMenu->addSeparator();

    const bool canUpdate =
        (savedSearchModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    addContextMenuAction(
        tr("Rename"), *m_savedSearchItemContextMenu, this,
        [this] { onRenameSavedSearchAction(); }, savedSearchItem->localId(),
        canUpdate ? ActionState::Enabled : ActionState::Disabled);

    if (savedSearchItem->guid().isEmpty()) {
        addContextMenuAction(
            tr("Delete"), *m_savedSearchItemContextMenu, this,
            [this] { onDeleteSavedSearchAction(); }, savedSearchItem->localId(),
            ActionState::Enabled);
    }

    addContextMenuAction(
        tr("Edit") + QStringLiteral("..."), *m_savedSearchItemContextMenu, this,
        [this] { onEditSavedSearchAction(); }, savedSearchItem->localId(),
        canUpdate ? ActionState::Enabled : ActionState::Disabled);

    if (!savedSearchItem->isFavorited()) {
        addContextMenuAction(
            tr("Favorite"), *m_savedSearchItemContextMenu, this,
            [this] { onFavoriteAction(); }, savedSearchItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);
    }
    else {
        addContextMenuAction(
            tr("Unfavorite"), *m_savedSearchItemContextMenu, this,
            [this] { onUnfavoriteAction(); }, savedSearchItem->localId(),
            canUpdate ? ActionState::Enabled : ActionState::Disabled);
    }

    m_savedSearchItemContextMenu->addSeparator();

    addContextMenuAction(
        tr("Clear selection"), *m_savedSearchItemContextMenu, this,
        [this] { onDeselectAction(); }, QString{}, ActionState::Enabled);

    addContextMenuAction(
        tr("Info") + QStringLiteral("..."), *m_savedSearchItemContextMenu, this,
        [this] { onShowSavedSearchInfoAction(); }, savedSearchItem->localId(),
        ActionState::Enabled);

    m_savedSearchItemContextMenu->exec(event->globalPos());
}

void SavedSearchItemView::setFavoritedFlag(
    const QAction & action, const bool favorited)
{
    auto * savedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!savedSearchModel)) {
        QNDEBUG("view::SavedSearchItemView", "Non-saved search model is used");
        return;
    }

    const QString itemLocalId = action.data().toString();
    if (Q_UNLIKELY(itemLocalId.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the saved search, can't get saved search's "
                       "local id from QAction"))
        return;
    }

    const auto itemIndex = savedSearchModel->indexForLocalId(itemLocalId);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the saved search, the model returned "
                       "invalid index for the saved search's local id"))
        return;
    }

    if (favorited) {
        savedSearchModel->favoriteSavedSearch(itemIndex);
    }
    else {
        savedSearchModel->unfavoriteSavedSearch(itemIndex);
    }
}

} // namespace quentier
