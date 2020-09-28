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

#include "SavedSearchItemView.h"

#include <lib/dialog/AddOrEditSavedSearchDialog.h>
#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/preferences/SettingsNames.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <memory>

#define SAVED_SEARCH_ITEM_VIEW_GROUP_KEY QStringLiteral("SavedSearchItemView")

#define LAST_SELECTED_SAVED_SEARCH_KEY                                         \
    QStringLiteral("LastSelectedSavedSearchLocalUid")

#define LAST_SELECTED_SAVED_SEARCHES_KEY                                       \
    QStringLiteral("LastSelectedSavedSearchLocalUids")

#define ALL_SAVED_SEARCHES_ROOT_ITEM_EXPANDED_KEY                              \
    QStringLiteral("AllSavedSearchesRootItemExpanded")

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:saved_search", errorDescription);                      \
        Q_EMIT notifyError(errorDescription);                                  \
    }

namespace quentier {

SavedSearchItemView::SavedSearchItemView(QWidget * parent) :
    AbstractMultiSelectionItemView(QStringLiteral("saved search"), parent)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void SavedSearchItemView::saveItemsState()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::saveItemsState");

    const auto * pSavedSearchModel = qobject_cast<const SavedSearchModel *>(
        model());

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    ApplicationSettings appSettings(
        pSavedSearchModel->account(), QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(SAVED_SEARCH_ITEM_VIEW_GROUP_KEY);

    auto allSavedSearchesRootItemIndex = pSavedSearchModel->index(0, 0);

    appSettings.setValue(
        ALL_SAVED_SEARCHES_ROOT_ITEM_EXPANDED_KEY,
        isExpanded(allSavedSearchesRootItemIndex));

    appSettings.endGroup();
}

void SavedSearchItemView::restoreItemsState(const ItemModel & model)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::restoreItemsState");

    const auto * pSavedSearchModel = qobject_cast<const SavedSearchModel *>(
        &model);

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(SAVED_SEARCH_ITEM_VIEW_GROUP_KEY);

    auto allSavedSearchesRootItemExpandedPreference =
        appSettings.value(ALL_SAVED_SEARCHES_ROOT_ITEM_EXPANDED_KEY);

    appSettings.endGroup();

    bool wasTrackingTagItemsState = trackItemsStateEnabled();
    setTrackItemsStateEnabled(false);

    bool allSavedSearchesRootItemExpanded = true;
    if (allSavedSearchesRootItemExpandedPreference.isValid()) {
        allSavedSearchesRootItemExpanded = allSavedSearchesRootItemExpandedPreference.toBool();
    }

    auto allTagsRootItemIndex = model.index(0, 0);
    setExpanded(allTagsRootItemIndex, allSavedSearchesRootItemExpanded);

    setTrackItemsStateEnabled(wasTrackingTagItemsState);
}

QString SavedSearchItemView::selectedItemsGroupKey() const
{
    return SAVED_SEARCH_ITEM_VIEW_GROUP_KEY;
}

QString SavedSearchItemView::selectedItemsArrayKey() const
{
    return LAST_SELECTED_SAVED_SEARCHES_KEY;
}

QString SavedSearchItemView::selectedItemsKey() const
{
    return LAST_SELECTED_SAVED_SEARCH_KEY;
}

bool SavedSearchItemView::shouldFilterBySelectedItems(
    const Account & account) const
{
    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(SIDE_PANELS_FILTER_BY_SELECTION_SETTINGS_GROUP_NAME);

    const auto filterBySelectedSavedSearch =
        appSettings.value(FILTER_BY_SELECTED_SAVED_SEARCH_SETTINGS_KEY);

    appSettings.endGroup();

    if (!filterBySelectedSavedSearch.isValid()) {
        return true;
    }

    return filterBySelectedSavedSearch.toBool();
}

QStringList SavedSearchItemView::localUidsInNoteFiltersManager(
    const NoteFiltersManager & noteFiltersManager) const
{
    const auto & savedSearchLocalUid =
        noteFiltersManager.savedSearchLocalUidInFilter();

    if (savedSearchLocalUid.isEmpty()) {
        return {};
    }

    return QStringList() << savedSearchLocalUid;
}

void SavedSearchItemView::setItemLocalUidsToNoteFiltersManager(
    const QStringList & itemLocalUids,
    NoteFiltersManager & noteFiltersManager)
{
    if (Q_UNLIKELY(itemLocalUids.isEmpty())) {
        noteFiltersManager.removeSavedSearchFromFilter();
        return;
    }

    noteFiltersManager.setSavedSearchLocalUidToFilter(itemLocalUids[0]);
}

void SavedSearchItemView::removeItemLocalUidsFromNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    noteFiltersManager.removeSavedSearchFromFilter();
}

void SavedSearchItemView::connectToModel(ItemModel & model)
{
    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(&model);
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNWARNING("view:saved_search", "Non-saved search model is used");
        return;
    }

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::aboutToAddSavedSearch, this,
        &SavedSearchItemView::onAboutToAddSavedSearch);

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::addedSavedSearch, this,
        &SavedSearchItemView::onAddedSavedSearch);

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::aboutToUpdateSavedSearch, this,
        &SavedSearchItemView::onAboutToUpdateSavedSearch);

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::updatedSavedSearch, this,
        &SavedSearchItemView::onUpdatedSavedSearch);

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::aboutToRemoveSavedSearches, this,
        &SavedSearchItemView::onAboutToRemoveSavedSearches);

    QObject::connect(
        pSavedSearchModel, &SavedSearchModel::removedSavedSearches, this,
        &SavedSearchItemView::onRemovedSavedSearches);
}

void SavedSearchItemView::deleteItem(
    const QModelIndex & itemIndex, ItemModel & model)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::deleteItem");

    QString localUid = model.localUidForItemIndex(itemIndex);
    if (Q_UNLIKELY(localUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag item "
                       "meant to be deleted"))
        return;
    }

    int confirm = warningMessageBox(
        this, tr("Confirm the saved search deletion"),
        tr("Are you sure you want to delete this saved search?"),
        tr("Note that this action is not reversible!"),
        QMessageBox::Ok | QMessageBox::No);

    if (confirm != QMessageBox::Ok) {
        QNDEBUG("view:saved_search", "Saved search deletion was not confirmed");
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG("view:saved_search", "Successfully deleted saved search");
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
        "view:saved_search", "SavedSearchItemView::onAboutToAddSavedSearch");

    prepareForModelChange();
}

void SavedSearchItemView::onAddedSavedSearch(const QModelIndex & index)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onAddedSavedSearch");

    Q_UNUSED(index)
    postProcessModelChange();
}

void SavedSearchItemView::onAboutToUpdateSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "view:saved_search", "SavedSearchItemView::onAboutToUpdateSavedSearch");

    Q_UNUSED(index)
    prepareForModelChange();
}

void SavedSearchItemView::onUpdatedSavedSearch(const QModelIndex & index)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onUpdatedSavedSearch");

    Q_UNUSED(index)
    postProcessModelChange();
}

void SavedSearchItemView::onAboutToRemoveSavedSearches()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onAboutToRemoveSavedSearches");

    prepareForModelChange();
}

void SavedSearchItemView::onRemovedSavedSearches()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onRemovedSavedSearches");
    postProcessModelChange();
}

void SavedSearchItemView::onCreateNewSavedSearchAction()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onCreateNewSavedSearchAction");

    Q_EMIT newSavedSearchCreationRequested();
}

void SavedSearchItemView::onRenameSavedSearchAction()
{
    QNDEBUG(
        "view:saved_search", "SavedSearchItemView::onRenameSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search, "
                       "can't get saved search's local uid from QAction"))
        return;
    }

    auto itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't rename saved search: the model "
                       "returned invalid index for the saved search's local "
                       "uid"))
        return;
    }

    edit(itemIndex);
}

void SavedSearchItemView::onDeleteSavedSearchAction()
{
    QNDEBUG(
        "view:saved_search", "SavedSearchItemView::onDeleteSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used")
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search, "
                       "can't get saved search's local uid from QAction"))
        return;
    }

    auto itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't delete saved search: the model "
                       "returned invalid index for the saved search's local "
                       "uid"))
        return;
    }

    deleteItem(itemIndex, *pSavedSearchModel);
}

void SavedSearchItemView::onEditSavedSearchAction()
{
    QNDEBUG(
        "view:saved_search", "SavedSearchItemView::onEditSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't edit saved search, "
                       "can't get saved search's local uid from QAction"))
        return;
    }

    auto pEditSavedSearchDialog = std::make_unique<AddOrEditSavedSearchDialog>(
        pSavedSearchModel, this, itemLocalUid);

    pEditSavedSearchDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditSavedSearchDialog->exec())
}

void SavedSearchItemView::onShowSavedSearchInfoAction()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onShowSavedSearchInfoAction");

    Q_EMIT savedSearchInfoRequested();
}

void SavedSearchItemView::onDeselectAction()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onDeselectAction");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't clear the saved search selection: "
                       "no selection model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void SavedSearchItemView::onFavoriteAction()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onFavoriteAction");

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't favorite saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void SavedSearchItemView::onUnfavoriteAction()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onUnfavoriteAction");

    auto * pAction = qobject_cast<QAction *>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void SavedSearchItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(
            "view:saved_search",
            "Detected Qt error: saved search item view received context menu "
            "event with null pointer to the context menu event");
        return;
    }

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(
            "view:saved_search",
            "Clicked item index is not valid, not doing anything");
        return;
    }

    const auto * pItem = pSavedSearchModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't show the context menu for the saved search "
                       "model item: no item corresponding to the clicked "
                       "item's index"))
        return;
    }

    const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        QNDEBUG(
            "view:saved_search",
            "Ignoring the context menu event for non-saved search item");
        return;
    }

    delete m_pSavedSearchItemContextMenu;
    m_pSavedSearchItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction, &QAction::triggered, this, &SavedSearchItemView::slot);   \
        menu->addAction(pAction);                                              \
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new saved search") + QStringLiteral("..."),
        m_pSavedSearchItemContextMenu, onCreateNewSavedSearchAction,
        pSavedSearchItem->localUid(), true);

    m_pSavedSearchItemContextMenu->addSeparator();

    bool canUpdate =
        (pSavedSearchModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"), m_pSavedSearchItemContextMenu, onRenameSavedSearchAction,
        pSavedSearchItem->localUid(), canUpdate);

    if (pSavedSearchItem->guid().isEmpty()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete"), m_pSavedSearchItemContextMenu,
            onDeleteSavedSearchAction, pSavedSearchItem->localUid(), true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."), m_pSavedSearchItemContextMenu,
        onEditSavedSearchAction, pSavedSearchItem->localUid(), canUpdate);

    if (!pSavedSearchItem->isFavorited()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"), m_pSavedSearchItemContextMenu, onFavoriteAction,
            pSavedSearchItem->localUid(), canUpdate);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"), m_pSavedSearchItemContextMenu, onUnfavoriteAction,
            pSavedSearchItem->localUid(), canUpdate);
    }

    m_pSavedSearchItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Clear selection"), m_pSavedSearchItemContextMenu, onDeselectAction,
        QString(), true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."), m_pSavedSearchItemContextMenu,
        onShowSavedSearchInfoAction, pSavedSearchItem->localUid(), true);

    m_pSavedSearchItemContextMenu->show();
    m_pSavedSearchItemContextMenu->exec(pEvent->globalPos());
}

void SavedSearchItemView::setFavoritedFlag(
    const QAction & action, const bool favorited)
{
    auto * pSavedSearchModel = qobject_cast<SavedSearchModel *>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the saved search, can't get saved search's "
                       "local uid from QAction"))
        return;
    }

    auto itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't set the favorited flag "
                       "for the saved search, the model returned "
                       "invalid index for the saved search's local uid"))
        return;
    }

    if (favorited) {
        pSavedSearchModel->favoriteSavedSearch(itemIndex);
    }
    else {
        pSavedSearchModel->unfavoriteSavedSearch(itemIndex);
    }
}

} // namespace quentier
