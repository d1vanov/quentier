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

#include "SavedSearchItemView.h"
#include "../SettingsNames.h"
#include "../models/SavedSearchModel.h"
#include "../dialogs/AddOrEditSavedSearchDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <QMenu>
#include <QContextMenuEvent>
#include <QScopedPointer>

#define LAST_SELECTED_SAVED_SEARCH_KEY QStringLiteral("LastSelectedSavedSearchLocalUid")

#define REPORT_ERROR(error) \
    { \
        ErrorString errorDescription(error); \
        QNWARNING(errorDescription); \
        emit notifyError(errorDescription); \
    }

namespace quentier {

SavedSearchItemView::SavedSearchItemView(QWidget * parent) :
    ItemView(parent),
    m_pSavedSearchItemContextMenu(Q_NULLPTR),
    m_trackingSelection(false),
    m_modelReady(false)
{}

void SavedSearchItemView::setModel(QAbstractItemModel * pModel)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::setModel"));

    SavedSearchModel * pPreviousModel = qobject_cast<SavedSearchModel*>(model());
    if (pPreviousModel)
    {
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,notifyError,ErrorString),
                            this, QNSIGNAL(SavedSearchItemView,notifyError,ErrorString));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                            this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,aboutToAddSavedSearch),
                            this, QNSLOT(SavedSearchItemView,onAboutToAddSavedSearch));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,addedSavedSearch,const QModelIndex&),
                            this, QNSLOT(SavedSearchItemView,onAddedSavedSearch,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,aboutToUpdateSavedSearch,const QModelIndex&),
                            this, QNSLOT(SavedSearchItemView,onAboutToUpdateSavedSearch,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,updatedSavedSearch,const QModelIndex&),
                            this, QNSLOT(SavedSearchItemView,onUpdatedSavedSearch,const QModelIndex&));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,aboutToRemoveSavedSearches),
                            this, QNSLOT(SavedSearchItemView,onAboutToRemoveSavedSearches));
        QObject::disconnect(pPreviousModel, QNSIGNAL(SavedSearchModel,removedSavedSearches),
                            this, QNSLOT(SavedSearchItemView,onRemovedSavedSearches));
    }

    m_modelReady = false;
    m_trackingSelection = false;

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(pModel);
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model has been set to the saved search item view"));
        ItemView::setModel(pModel);
        return;
    }

    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyError,ErrorString),
                     this, QNSIGNAL(SavedSearchItemView,notifyError,ErrorString));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,aboutToAddSavedSearch),
                     this, QNSLOT(SavedSearchItemView,onAboutToAddSavedSearch));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,addedSavedSearch,const QModelIndex&),
                     this, QNSLOT(SavedSearchItemView,onAddedSavedSearch,const QModelIndex&));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,aboutToUpdateSavedSearch,const QModelIndex&),
                     this, QNSLOT(SavedSearchItemView,onAboutToUpdateSavedSearch,const QModelIndex&));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,updatedSavedSearch,const QModelIndex&),
                     this, QNSLOT(SavedSearchItemView,onUpdatedSavedSearch,const QModelIndex&));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,aboutToRemoveSavedSearches),
                     this, QNSLOT(SavedSearchItemView,onAboutToRemoveSavedSearches));
    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,removedSavedSearches),
                     this, QNSLOT(SavedSearchItemView,onRemovedSavedSearches));

    ItemView::setModel(pModel);

    if (pSavedSearchModel->allSavedSearchesListed()) {
        QNDEBUG(QStringLiteral("All saved searches are already listed within the model"));
        restoreLastSavedSelection(*pSavedSearchModel);
        m_modelReady = true;
        m_trackingSelection = true;
        return;
    }

    QObject::connect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                     this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));
}

QModelIndex SavedSearchItemView::currentlySelectedItemIndex() const
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::currentlySelectedItemIndex"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return QModelIndex();
    }

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNDEBUG(QStringLiteral("No selection model in the view"));
        return QModelIndex();
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The selection contains no model indexes"));
        return QModelIndex();
    }

    return singleRow(indexes, *pSavedSearchModel, SavedSearchModel::Columns::Name);
}

void SavedSearchItemView::deleteSelectedItem()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::deleteSelectedItem"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        QNDEBUG(QStringLiteral("No saved searches are selected, nothing to delete"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current saved search"),
                                       tr("No saved search is selected currently"),
                                       tr("Please select the saved search you want to delete")))
        return;
    }

    QModelIndex index = singleRow(indexes, *pSavedSearchModel, SavedSearchModel::Columns::Name);
    if (!index.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one saved search within the selection"));
        Q_UNUSED(informationMessageBox(this, tr("Cannot delete current saved search"),
                                       tr("More than one saved search is currently selected"),
                                       tr("Please select only the saved search you want to delete")))
        return;
    }

    deleteItem(index, *pSavedSearchModel);
}

void SavedSearchItemView::onAllSavedSearchesListed()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAllSavedSearchesListed"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't cast the model set to the saved search item view "
                                       "to the saved search model"))
        return;
    }

    QObject::disconnect(pSavedSearchModel, QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                        this, QNSLOT(SavedSearchItemView,onAllSavedSearchesListed));

    restoreLastSavedSelection(*pSavedSearchModel);
    m_modelReady = true;
    m_trackingSelection = true;
}

void SavedSearchItemView::onAboutToAddSavedSearch()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAboutToAddSavedSearch"));
    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onAddedSavedSearch(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAddedSavedSearch"));

    Q_UNUSED(index)
    postProcessSavedSearchModelChange();
}

void SavedSearchItemView::onAboutToUpdateSavedSearch(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAboutToUpdateSavedSearch"));

    Q_UNUSED(index)
    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onUpdatedSavedSearch(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onUpdatedSavedSearch"));

    Q_UNUSED(index)
    postProcessSavedSearchModelChange();
}

void SavedSearchItemView::onAboutToRemoveSavedSearches()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onAboutToRemoveSavedSearches"));
    prepareForSavedSearchModelChange();
}

void SavedSearchItemView::onRemovedSavedSearches()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onRemovedSavedSearches"));
    postProcessSavedSearchModelChange();
}

void SavedSearchItemView::onCreateNewSavedSearchAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onCreateNewSavedSearchAction"));
    emit newSavedSearchCreationRequested();
}

void SavedSearchItemView::onRenameSavedSearchAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onRenameSavedSearchAction"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename saved search, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename saved search, "
                                       "can't get saved search's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't rename saved search: the model returned invalid "
                                       "index for the saved search's local uid"))
        return;
    }

    edit(itemIndex);
}

void SavedSearchItemView::onDeleteSavedSearchAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onDeleteSavedSearchAction"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't delete saved search, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't delete saved search, "
                                       "can't get saved search's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't delete saved search: the model returned invalid "
                                       "index for the saved search's local uid"))
        return;
    }

    deleteItem(itemIndex, *pSavedSearchModel);
}

void SavedSearchItemView::onEditSavedSearchAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onEditSavedSearchAction"));

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't edit saved search, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    QString itemLocalUid = pAction->data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't edit saved search, "
                                       "can't get saved search's local uid from QAction"))
        return;
    }

    QScopedPointer<AddOrEditSavedSearchDialog> pEditSavedSearchDialog(new AddOrEditSavedSearchDialog(pSavedSearchModel, this, itemLocalUid));
    pEditSavedSearchDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pEditSavedSearchDialog->exec())
}

void SavedSearchItemView::onShowSavedSearchInfoAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onShowSavedSearchInfoAction"));
    emit savedSearchInfoRequested();
}

void SavedSearchItemView::onDeselectAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onDeselectAction"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't clear the saved search selection: no selection model in the view"))
        return;
    }

    pSelectionModel->clearSelection();
}

void SavedSearchItemView::onFavoriteAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onFavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't favorite saved search, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, true);
}

void SavedSearchItemView::onUnfavoriteAction()
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::onUnfavoriteAction"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't unfavorite saved search, "
                                       "can't cast the slot invoker to QAction"))
        return;
    }

    setFavoritedFlag(*pAction, false);
}

void SavedSearchItemView::selectionChanged(const QItemSelection & selected,
                                           const QItemSelection & deselected)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::selectionChanged"));

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void SavedSearchItemView::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Detected Qt error: saved search item view received "
                                 "context menu event with null pointer "
                                 "to the context menu event"));
        return;
    }

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QModelIndex clickedItemIndex = indexAt(pEvent->pos());
    if (Q_UNLIKELY(!clickedItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Clicked item index is not valid, not doing anything"));
        return;
    }

    const SavedSearchModelItem * pItem = pSavedSearchModel->itemForIndex(clickedItemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't show the context menu for the saved search model item: "
                                       "no item corresponding to the clicked item's index"))
        return;
    }

    delete m_pSavedSearchItemContextMenu;
    m_pSavedSearchItemContextMenu = new QMenu(this);

#define ADD_CONTEXT_MENU_ACTION(name, menu, slot, data, enabled) \
    { \
        QAction * pAction = new QAction(name, menu); \
        pAction->setData(data); \
        pAction->setEnabled(enabled); \
        QObject::connect(pAction, QNSIGNAL(QAction,triggered), this, QNSLOT(SavedSearchItemView,slot)); \
        menu->addAction(pAction); \
    }

    ADD_CONTEXT_MENU_ACTION(tr("Create new saved search") + QStringLiteral("..."),
                            m_pSavedSearchItemContextMenu, onCreateNewSavedSearchAction,
                            pItem->m_localUid, true);

    m_pSavedSearchItemContextMenu->addSeparator();

    bool canUpdate = (pSavedSearchModel->flags(clickedItemIndex) & Qt::ItemIsEditable);
    ADD_CONTEXT_MENU_ACTION(tr("Rename"), m_pSavedSearchItemContextMenu,
                            onRenameSavedSearchAction, pItem->m_localUid, canUpdate);

    ADD_CONTEXT_MENU_ACTION(tr("Delete"), m_pSavedSearchItemContextMenu,
                            onDeleteSavedSearchAction, pItem->m_localUid,
                            !pItem->m_isSynchronizable);

    ADD_CONTEXT_MENU_ACTION(tr("Edit") + QStringLiteral("..."),
                            m_pSavedSearchItemContextMenu, onEditSavedSearchAction,
                            pItem->m_localUid, canUpdate);

    if (!pItem->m_isFavorited) {
        ADD_CONTEXT_MENU_ACTION(tr("Favorite"), m_pSavedSearchItemContextMenu,
                                onFavoriteAction, pItem->m_localUid, canUpdate);
    }
    else {
        ADD_CONTEXT_MENU_ACTION(tr("Unfavorite"), m_pSavedSearchItemContextMenu,
                                onUnfavoriteAction, pItem->m_localUid, canUpdate);
    }

    m_pSavedSearchItemContextMenu->addSeparator();

    ADD_CONTEXT_MENU_ACTION(tr("Clear selection"), m_pSavedSearchItemContextMenu,
                            onDeselectAction, QString(), true);

    ADD_CONTEXT_MENU_ACTION(tr("Info") + QStringLiteral("..."), m_pSavedSearchItemContextMenu,
                            onShowSavedSearchInfoAction, pItem->m_localUid, true);

    m_pSavedSearchItemContextMenu->show();
    m_pSavedSearchItemContextMenu->exec(pEvent->globalPos());
}

void SavedSearchItemView::deleteItem(const QModelIndex & itemIndex, SavedSearchModel & model)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::deleteItem"));

    const SavedSearchModelItem * pItem = model.itemForIndex(itemIndex);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't find the saved search model item meant to be deleted"))
        return;
    }

    int confirm = warningMessageBox(this, tr("Confirm the saved search deletion"),
                                    tr("Are you sure you want to delete this saved search?"),
                                    tr("Note that this action is not reversible!"),
                                    QMessageBox::Ok | QMessageBox::No);
    if (confirm != QMessageBox::Ok) {
        QNDEBUG(QStringLiteral("Saved search deletion was not confirmed"));
        return;
    }

    bool res = model.removeRow(itemIndex.row(), itemIndex.parent());
    if (res) {
        QNDEBUG(QStringLiteral("Successfully deleted saved search"));
        return;
    }

    Q_UNUSED(internalErrorMessageBox(this, tr("The saved search model refused to delete the saved search; "
                                              "Check the status bar for message from the saved search model "
                                              "explaining why the saved search could not be deleted")))
}

void SavedSearchItemView::restoreLastSavedSelection(const SavedSearchModel & model)
{
    QNDEBUG(QStringLiteral("SavedSearchItemView::restoreLastSavedSelection"));

    QItemSelectionModel * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't restore the last selected saved search: no selection model in the view"))
        return;
    }

    ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchItemView"));
    QString lastSelectedSavedSearchLocalUid = appSettings.value(LAST_SELECTED_SAVED_SEARCH_KEY).toString();
    appSettings.endGroup();

    if (lastSelectedSavedSearchLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Found no last selected saved search local uid"));
        return;
    }

    QNTRACE(QStringLiteral("Last selected saved search local uid: ") << lastSelectedSavedSearchLocalUid);

    QModelIndex lastSelectedSavedSearchIndex = model.indexForLocalUid(lastSelectedSavedSearchLocalUid);
    if (!lastSelectedSavedSearchIndex.isValid()) {
        QNDEBUG(QStringLiteral("Saved search model returned invalid index for the last selected saved search local uid"));
        return;
    }

    pSelectionModel->select(lastSelectedSavedSearchIndex, QItemSelectionModel::ClearAndSelect |
                                                          QItemSelectionModel::Rows |
                                                          QItemSelectionModel::Current);
}

void SavedSearchItemView::selectionChangedImpl(const QItemSelection & selected,
                                               const QItemSelection & deselected)
{
    QNTRACE(QStringLiteral("SavedSearchItemView::selectionChangedImpl"));

    if (!m_trackingSelection) {
        QNTRACE(QStringLiteral("Not tracking selection at this time, skipping"));
        return;
    }

    Q_UNUSED(deselected)

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QModelIndexList selectedIndexes = selected.indexes();

    if (selectedIndexes.isEmpty()) {
        QNDEBUG(QStringLiteral("The new selection is empty"));
        return;
    }

    // Need to figure out how many rows the new selection covers; if exactly 1,
    // persist this selection so that it can be resurrected on the next startup
    QModelIndex sourceIndex = singleRow(selectedIndexes, *pSavedSearchModel, SavedSearchModel::Columns::Name);
    if (!sourceIndex.isValid()) {
        QNDEBUG(QStringLiteral("Not exactly one row is selected"));
        return;
    }

    const SavedSearchModelItem * pSavedSearchItem = pSavedSearchModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't find the saved search model item corresponding "
                                       "to the selected index"))
        return;
    }

    QNTRACE(QStringLiteral("Currently selected saved search item: ") << *pSavedSearchItem);

    ApplicationSettings appSettings(pSavedSearchModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchItemView"));
    appSettings.setValue(LAST_SELECTED_SAVED_SEARCH_KEY, pSavedSearchItem->m_localUid);
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Persisted the currently selected saved search local uid: ")
            << pSavedSearchItem->m_localUid);
}

void SavedSearchItemView::setFavoritedFlag(const QAction & action, const bool favorited)
{
    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    QString itemLocalUid = action.data().toString();
    if (Q_UNLIKELY(itemLocalUid.isEmpty())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't set the favorited flag for the saved search, "
                                       "can't get saved search's local uid from QAction"))
        return;
    }

    QModelIndex itemIndex = pSavedSearchModel->indexForLocalUid(itemLocalUid);
    if (Q_UNLIKELY(!itemIndex.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't set the favorited flag for the saved search, the model "
                                       "returned invalid index for the saved search's local uid"))
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
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    m_trackingSelection = false;
}

void SavedSearchItemView::postProcessSavedSearchModelChange()
{
    if (!m_modelReady) {
        QNDEBUG(QStringLiteral("The model is not ready yet"));
        return;
    }

    SavedSearchModel * pSavedSearchModel = qobject_cast<SavedSearchModel*>(model());
    if (Q_UNLIKELY(!pSavedSearchModel)) {
        QNDEBUG(QStringLiteral("Non-saved search model is used"));
        return;
    }

    restoreLastSavedSelection(*pSavedSearchModel);
    m_trackingSelection = true;
}

} // namespace quentier
