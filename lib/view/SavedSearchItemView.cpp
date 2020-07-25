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
#include <lib/model/SavedSearchModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <QContextMenuEvent>
#include <QMenu>

#include <memory>

#define LAST_SELECTED_SAVED_SEARCH_KEY                                         \
    QStringLiteral("LastSelectedSavedSearchLocalUid")                          \
// LAST_SELECTED_SAVED_SEARCH_KEY

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        QNWARNING("view:saved_search", errorDescription);                      \
        Q_EMIT notifyError(errorDescription);                                  \
    }                                                                          \
// REPORT_ERROR

namespace quentier {

SavedSearchItemView::SavedSearchItemView(QWidget * parent) :
    ItemView(parent)
{}

void SavedSearchItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::setModel");

    auto * pPreviousModel = qobject_cast<SavedSearchModel*>(model());
    if (pPreviousModel)
    {
        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::notifyError,
            this,
            &SavedSearchItemView::notifyError);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::notifyAllSavedSearchesListed,
            this,
            &SavedSearchItemView::onAllSavedSearchesListed);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::aboutToAddSavedSearch,
            this,
            &SavedSearchItemView::onAboutToAddSavedSearch);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::addedSavedSearch,
            this,
            &SavedSearchItemView::onAddedSavedSearch);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::aboutToUpdateSavedSearch,
            this,
            &SavedSearchItemView::onAboutToUpdateSavedSearch);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::updatedSavedSearch,
            this,
            &SavedSearchItemView::onUpdatedSavedSearch);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::aboutToRemoveSavedSearches,
            this,
            &SavedSearchItemView::onAboutToRemoveSavedSearches);

        QObject::disconnect(
            pPreviousModel,
            &SavedSearchModel::removedSavedSearches,
            this,
            &SavedSearchItemView::onRemovedSavedSearches);
    }

    m_modelReady = false;
    m_trackingSelection = false;

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(pModel);
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model has been set to "
            << "the saved search item view");
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::notifyError,
        this,
        &SavedSearchItemView::notifyError);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::aboutToAddSavedSearch,
        this,
        &SavedSearchItemView::onAboutToAddSavedSearch);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::addedSavedSearch,
        this,
        &SavedSearchItemView::onAddedSavedSearch);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::aboutToUpdateSavedSearch,
        this,
        &SavedSearchItemView::onAboutToUpdateSavedSearch);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::updatedSavedSearch,
        this,
        &SavedSearchItemView::onUpdatedSavedSearch);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::aboutToRemoveSavedSearches,
        this,
        &SavedSearchItemView::onAboutToRemoveSavedSearches);

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::removedSavedSearches,
        this,
        &SavedSearchItemView::onRemovedSavedSearches);

    ItemView::setModel(pModel);

    if (pSavedSearchModel->allSavedSearchesListed()) {
        QNDEBUG("view:saved_search", "All saved searches are already listed "
            << "within the model");
        restoreLastSavedSelection(*pSavedSearchModel);
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(
        pSavedSearchModel,
        &SavedSearchModel::notifyAllSavedSearchesListed,
        this,
        &SavedSearchItemView::onAllSavedSearchesListed);
}

QModelIndex SavedSearchItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::currentlySelectedItemIndex");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return {};
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG("view:saved_search", "No selection model in the view");
        return {};
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG("view:saved_search", "The selection contains no model indexes");
        return {};
    }

    return singleRow(
        indexes,
        *pSavedSearchModel,
        SavedSearchModel::Columns::Name);
}

void SavedSearchItemView::deleteSelectedItem()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::deleteSelectedItem");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty())
    {
        QNDEBUG("view:saved_search", "No saved searches are selected, nothing "
            << "to delete");

        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current saved search"),
            tr("No saved search is selected currently"),
            tr("Please select the saved search you want to delete")))

        return;
    }

    auto index = singleRow(
        indexes,
        *pSavedSearchModel,
        SavedSearchModel::Columns::Name);

    if (!index.isValid())
    {
        QNDEBUG("view:saved_search", "Not exactly one saved search within "
            << "the selection");

        Q_UNUSED(informationMessageBox(
            this,
            tr("Cannot delete current saved search"),
            tr("More than one saved search is currently selected"),
            tr("Please select only the saved search you want to delete")))

        return;
    }

    deleteItem(index, *pSavedSearchModel);
}

void SavedSearchItemView::onAllSavedSearchesListed()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onAllSavedSearchesListed");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't cast the model set to the saved search "
                       "item view to the saved search model"))
        return;
    }

    QObject::disconnect(
        pSavedSearchModel,
        &SavedSearchModel::notifyAllSavedSearchesListed,
        this,
        &SavedSearchItemView::onAllSavedSearchesListed);

    restoreLastSavedSelection(*pSavedSearchModel);
    m_modelReady = true;
    m_trackingSelection = true;
}

void SavedSearchItemView::onAboutToAddSavedSearch()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onAboutToAddSavedSearch");

    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onAddedSavedSearch(const QModelIndex & index)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onAddedSavedSearch");

    Q_UNUSED(index)
    postProcessSavedSearchModelChange();
}

void SavedSearchItemView::onAboutToUpdateSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onAboutToUpdateSavedSearch");

    Q_UNUSED(index)
    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onUpdatedSavedSearch(const QModelIndex & index)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onUpdatedSavedSearch");

    Q_UNUSED(index)
    postProcessSavedSearchModelChange();
}

void SavedSearchItemView::onAboutToRemoveSavedSearches()
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::onAboutToRemoveSavedSearches");

    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onRemovedSavedSearches()
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::onRemovedSavedSearches");
    postProcessSavedSearchModelChange();
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
        "view:saved_search",
        "SavedSearchItemView::onRenameSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
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
        "view:saved_search",
        "SavedSearchItemView::onDeleteSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used")
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
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
        "view:saved_search",
        "SavedSearchItemView::onEditSavedSearchAction");

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto * pAction = qobject_cast<QAction*>(sender());
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
        pSavedSearchModel,
        this,
        itemLocalUid);

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

    auto * pAction = qobject_cast<QAction*>(sender());
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

    auto * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't unfavorite saved search, "
                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void SavedSearchItemView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void SavedSearchItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("view:saved_search", "Detected Qt error: saved search item "
            << "view received context menu event with null pointer to "
            << "the context menu event");
        return;
    }

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG("view:saved_search", "Clicked item index is not valid, not "
            << "doing anything");
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

    delete m_pSavedSearchItemContextMenu;
    m_pSavedSearchItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled)               \
    {                                                                          \
        QAction * pAction = new QAction(name, menu);                           \
        pAction->setData(data);                                                \
        pAction->setEnabled(enabled);                                          \
        QObject::connect(                                                      \
            pAction,                                                           \
            &QAction::triggered,                                               \
            this,                                                              \
            &SavedSearchItemView::slot);                                       \
        menu->addAction(pAction);                                              \
    }                                                                          \
// ADD_CONTEXT_MENU_ACTION

    ADD_CONTEXT_MENU_ACTION(
        tr("Create new saved search") + QStringLiteral("..."),
        m_pSavedSearchItemContextMenu,
        onCreateNewSavedSearchAction,
        pItem->m_localUid,
        true);

    m_pSavedSearchItemContextMenu->addSeparator();

    bool canUpdate =
        (pSavedSearchModel->flags(clickedItemIndex) & Qt::ItemIsEditable);

    ADD_CONTEXT_MENU_ACTION(
        tr("Rename"),
        m_pSavedSearchItemContextMenu,
        onRenameSavedSearchAction,
        pItem->m_localUid,
        canUpdate);

    if (pItem->m_guid.isEmpty()) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Delete"),
            m_pSavedSearchItemContextMenu,
            onDeleteSavedSearchAction,
            pItem->m_localUid,
            true);
    }

    ADD_CONTEXT_MENU_ACTION(
        tr("Edit") + QStringLiteral("..."),
        m_pSavedSearchItemContextMenu,
        onEditSavedSearchAction,
        pItem->m_localUid,
        canUpdate);

    if (!pItem->m_isFavorited) {
        ADD_CONTEXT_MENU_ACTION(
            tr("Favorite"),
            m_pSavedSearchItemContextMenu,
            onFavoriteAction,
            pItem->m_localUid,
            canUpdate);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(
            tr("Unfavorite"),
            m_pSavedSearchItemContextMenu,
            onUnfavoriteAction,
            pItem->m_localUid,
            canUpdate);
    }

    m_pSavedSearchItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(
        tr("Clear selection"),
        m_pSavedSearchItemContextMenu,
        onDeselectAction,
        QString(),
        true);

    ADD_CONTEXT_MENU_ACTION(
        tr("Info") + QStringLiteral("..."),
        m_pSavedSearchItemContextMenu,
        onShowSavedSearchInfoAction,
        pItem->m_localUid,
        true);

    m_pSavedSearchItemContextMenu->show();
    m_pSavedSearchItemContextMenu->exec(pEvent->globalPos());
}

void SavedSearchItemView::deleteItem(
    const QModelIndex & itemIndex, SavedSearchModel & model)
{
    QNDEBUG("view:saved_search", "SavedSearchItemView::deleteItem");

    const auto * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the saved search "
                       "model item meant to be deleted"))
        return;
    }

    int confirm = warningMessageBox(
        this,
        tr("Confirm the saved search deletion"),
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

void SavedSearchItemView::restoreLastSavedSelection(
    const SavedSearchModel & model)
{
    QNDEBUG(
        "view:saved_search",
        "SavedSearchItemView::restoreLastSavedSelection");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore the last selected saved search: "
                       "no selection model in the view"))
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchItemView"));

    QString lastSelectedSavedSearchLocalUid = appSettings.value(
        LAST_SELECTED_SAVED_SEARCH_KEY).toString();

    appSettings.endGroup();

    if (lastSelectedSavedSearchLocalUid.isEmpty()) {
        QNDEBUG("view:saved_search", "Found no last selected saved search "
            << "local uid");
        return;
    }

    QNTRACE("view:saved_search", "Last selected saved search local uid: "
        << lastSelectedSavedSearchLocalUid);

    auto lastSelectedSavedSearchIndex = model.indexForLocalUid(
        lastSelectedSavedSearchLocalUid);

    if (!lastSelectedSavedSearchIndex.isValid()) {
        QNDEBUG("view:saved_search", "Saved search model returned invalid "
            << "index for the last selected saved search local uid");
        return;
    }

    pSelectionModel->select(
        lastSelectedSavedSearchIndex,
        QItemSelectionModel::ClearAndSelect |
        QItemSelectionModel::Rows |
        QItemSelectionModel::Current);
}

void SavedSearchItemView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNTRACE("view:saved_search", "SavedSearchItemView::selectionChangedImpl");

    if (!m_trackingSelection) {
        QNTRACE("view:saved_search", "Not tracking selection at this time, "
            << "skipping");
        return;
    }

    Q_UNUSED(deselected)

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    auto selectedIndexes = selected.indexes();
    if (selectedIndexes.isEmpty()) {
        QNDEBUG("view:saved_search", "The new selection is empty");
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    auto sourceIndex = singleRow(
        selectedIndexes,
        *pSavedSearchModel,
        SavedSearchModel::Columns::Name);

    if (!sourceIndex.isValid()) {
        QNDEBUG("view:saved_search", "Not exactly one row is selected");
        return;
    }

    const auto * pSavedSearchItem = pSavedSearchModel->itemForIndex(
        sourceIndex);

    if (Q_UNLIKELY(!pSavedSearchItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the saved search "
                       "model item corresponding to the selected index"))
        return;
    }

    QNTRACE("view:saved_search", "Currently selected saved search item: "
        << *pSavedSearchItem);

    ApplicationSettings appSettings(
        pSavedSearchModel->account(),
        QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(QStringLiteral("SavedSearchItemView"));

    appSettings.setValue(
        LAST_SELECTED_SAVED_SEARCH_KEY,
        pSavedSearchItem->m_localUid);

    appSettings.endGroup();

    QNDEBUG("view:saved_search", "Persisted the currently selected saved "
        << "search local uid: " << pSavedSearchItem->m_localUid);
}

void SavedSearchItemView::setFavoritedFlag(
    const QAction & action, const bool favorited)
{
    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
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

void SavedSearchItemView::prepareForSavedSearchModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:saved_search", "The model is not ready yet");
        return;
    }

    m_trackingSelection = false;
}

void SavedSearchItemView::postProcessSavedSearchModelChange()
{
    if (!m_modelReady) {
        QNDEBUG("view:saved_search", "The model is not ready yet");
        return;
    }

    auto * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG("view:saved_search", "Non-saved search model is used");
        return;
    }

    restoreLastSavedSelection(*pSavedSearchModel);
    m_trackingSelection = true;
}

} // namespace quentier
