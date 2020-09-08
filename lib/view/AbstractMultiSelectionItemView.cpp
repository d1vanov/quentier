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

#include "AbstractMultiSelectionItemView.h"
#include "ItemSelectionModel.h"

#include <lib/model/ItemModel.h>
#include <lib/preferences/SettingsNames.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

namespace quentier {

#define MSLOG_BASE(level, message)                                             \
    __QNLOG_BASE(                                                              \
        (QString::fromUtf8("view:") + m_modelTypeName).toUtf8(),               \
        "[" << m_modelTypeName << "]: " << message, level)

#define MSTRACE(message) MSLOG_BASE(Trace, message)
#define MSDEBUG(message) MSLOG_BASE(Debug, message)
#define MSINFO(message) MSLOG_BASE(Info, message)
#define MSWARNING(message) MSLOG_BASE(Warning, message)
#define MSERROR(message) MSLOG_BASE(Error, message)

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        MSWARNING(errorDescription);                                           \
        Q_EMIT notifyError(errorDescription);                                  \
    }

AbstractMultiSelectionItemView::AbstractMultiSelectionItemView(
    const QString & modelTypeName, QWidget * parent) :
    ItemView(parent),
    m_modelTypeName(modelTypeName)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    QObject::connect(
        this, &AbstractMultiSelectionItemView::expanded, this,
        &AbstractMultiSelectionItemView::onItemCollapsedOrExpanded);

    QObject::connect(
        this, &AbstractMultiSelectionItemView::collapsed, this,
        &AbstractMultiSelectionItemView::onItemCollapsedOrExpanded);
}

AbstractMultiSelectionItemView::~AbstractMultiSelectionItemView() = default;

void AbstractMultiSelectionItemView::setNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    if (!m_pNoteFiltersManager.isNull()) {
        if (m_pNoteFiltersManager.data() == &noteFiltersManager) {
            MSDEBUG("Already using this note filters manager");
            return;
        }

        disconnectFromNoteFiltersManagerFilterChanged();
    }

    m_pNoteFiltersManager = &noteFiltersManager;
    connectToNoteFiltersManagerFilterChanged();
}

void AbstractMultiSelectionItemView::setModel(QAbstractItemModel * pModel)
{
    MSDEBUG("AbstractMultiSelectionItemView::setModel");

    auto * pPreviousModel = qobject_cast<ItemModel*>(model());
    if (pPreviousModel) {
        pPreviousModel->disconnect(this);
    }

    m_itemLocalUidsPendingNoteFiltersManagerReadiness.clear();
    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingItemsState = false;

    auto * pItemModel = qobject_cast<ItemModel*>(pModel);
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model has been set to the item view");
        ItemView::setModel(pModel);
        return;
    }

    connectToModel(*pItemModel);

    ItemView::setModel(pModel);

    auto * pOldSelectionModel = selectionModel();
    setSelectionModel(new ItemSelectionModel(pItemModel, this));
    if (pOldSelectionModel) {
        pOldSelectionModel->disconnect(this);
        QObject::disconnect(pOldSelectionModel);
        pOldSelectionModel->deleteLater();
    }

    if (pItemModel->allItemsListed()) {
        MSDEBUG("All items are already listed within the model");
        restoreItemsState(*pItemModel);
        restoreSelectedItems(*pItemModel);
        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingItemsState = true;
        return;
    }

    QObject::connect(
        pItemModel, &ItemModel::notifyAllItemsListed, this,
        &AbstractMultiSelectionItemView::onAllItemsListed);
}

QModelIndex AbstractMultiSelectionItemView::currentlySelectedItemIndex() const
{
    MSDEBUG("AbstractMultiSelectionItemView::currentlySelectedItemIndex");

    auto * pItemModel = qobject_cast<ItemModel*>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return {};
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        MSDEBUG("No selection model in the view");
        return {};
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("The selection contains no model indexes");
        return {};
    }

    return singleRow(indexes, *pItemModel, 0);
}

void AbstractMultiSelectionItemView::deleteSelectedItem()
{
    MSDEBUG("AbstractMultiSelectionItemView::deleteSelectedItem");

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("No items are selected, nothing to deete");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot delete current item"),
            tr("No item is selected currently"),
            tr("Please select the item you want to delete")))

        return;
    }

    auto index = singleRow(indexes, *pItemModel, 0);

    if (!index.isValid()) {
        MSDEBUG("Not exactly one item within the selection");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot delete current item"),
            tr("More than one item is currently selected"),
            tr("Please select only the item you want to delete")))

        return;
    }

    deleteItem(index, *pItemModel);
}

void AbstractMultiSelectionItemView::onAllItemsListed()
{
    MSDEBUG("AbstractMultiSelectionItemView::onAllItemsListed");

    auto * pItemModel = qobject_cast<ItemModel*>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't cast the model set to the item view "
                       "to the item model"))
        return;
    }

    QObject::disconnect(
        pItemModel, &ItemModel::notifyAllItemsListed, this,
        &AbstractMultiSelectionItemView::onAllItemsListed);

    restoreItemsState(*pItemModel);
    restoreSelectedItems(*pItemModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingItemsState = true;
}

void AbstractMultiSelectionItemView::onItemCollapsedOrExpanded(
    const QModelIndex & index)
{
    MSTRACE(
        "AbstractMultiSelectionItemView::onItemCollapsedOrExpanded: "
            << "index: valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column()
            << ", parent: valid = "
            << (index.parent().isValid() ? "true" : "false")
            << ", row = " << index.parent().row()
            << ", column = " << index.parent().column());

    if (!m_trackingItemsState) {
        MSDEBUG("Not tracking the items state at this moment");
        return;
    }

    saveItemsState();
}

void AbstractMultiSelectionItemView::onNoteFilterChanged()
{
    MSDEBUG("AbstractMultiSelectionItemView::onNoteFilterChanged");

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    if (!shouldFilterBySelectedItems(pItemModel->account())) {
        MSDEBUG("Filtering by selected items is disabled, won't do anything");
        return;
    }

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        MSDEBUG("Filter by search string is active");
        selectAllItemsRootItem(*pItemModel);
        return;
    }

    if (!m_pNoteFiltersManager->savedSearchLocalUidInFilter().isEmpty()) {
        QNDEBUG("view:tag", "Filter by saved search is active");
        selectAllItemsRootItem(*pItemModel);
        return;
    }

    const auto localUids = localUidsInNoteFiltersManager(
        *m_pNoteFiltersManager);

    if (localUids.isEmpty()) {
        MSDEBUG("No items in note filter");
        selectAllItemsRootItem(*pItemModel);
        return;
    }

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        QNWARNING("view:tag", "No selection model, can't update selection");
        return;
    }

    // Check whether current selection matches tag local uids
    bool selectionIsActual = true;
    auto indexes = pSelectionModel->selectedIndexes();
    if (indexes.size() != localUids.size()) {
        selectionIsActual = false;
    }
    else {
        for (const auto & index: qAsConst(indexes)) {
            const auto localUid = pItemModel->localUidForItemIndex(index);
            if (localUid.isEmpty()) {
                selectionIsActual = false;
                break;
            }

            if (!localUids.contains(localUid)) {
                selectionIsActual = false;
                break;
            }
        }
    }

    if (selectionIsActual) {
        MSDEBUG("Selected items match those in note filter");
        return;
    }

    QItemSelection selection;
    for (const auto & localUid: qAsConst(localUids)) {
        auto index = pItemModel->indexForLocalUid(localUid);
        selection.select(index, index);
    }

    pSelectionModel->select(
        selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void AbstractMultiSelectionItemView::onNoteFiltersManagerReady()
{
    MSDEBUG("AbstractMultiSelectionItemView::onNoteFiltersManagerReady");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    QObject::disconnect(
        m_pNoteFiltersManager.data(), &NoteFiltersManager::ready, this,
        &AbstractMultiSelectionItemView::onNoteFiltersManagerReady);

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    if (m_restoreSelectedItemsWhenNoteFiltersManagerReady) {
        m_restoreSelectedItemsWhenNoteFiltersManagerReady = false;

        if (m_itemLocalUidsPendingNoteFiltersManagerReadiness.isEmpty()) {
            restoreSelectedItems(*pItemModel);
            return;
        }
    }

    QStringList itemLocalUids = m_itemLocalUidsPendingNoteFiltersManagerReadiness;
    m_itemLocalUidsPendingNoteFiltersManagerReadiness.clear();

    const auto & account = pItemModel->account();
    saveSelectedItems(account, itemLocalUids);

    if (shouldFilterBySelectedItems(account)) {
        if (!itemLocalUids.isEmpty()) {
            setItemsToNoteFiltersManager(itemLocalUids);
        }
        else {
            clearItemsFromNoteFiltersManager();
        }
    }
}

void AbstractMultiSelectionItemView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    MSDEBUG("AbstractMultiSelectionItemView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    ItemView::selectionChanged(selected, deselected);
}

void AbstractMultiSelectionItemView::prepareForModelChange()
{
    MSDEBUG("AbstractMultiSelectionItemView::prepareForModelChange");

    if (!m_modelReady) {
        MSDEBUG("The model is not ready yet");
        return;
    }

    saveItemsState();
    m_trackingSelection = false;
    m_trackingItemsState = false;
}

void AbstractMultiSelectionItemView::postProcessModelChange()
{
    MSDEBUG("AbstractMultiSelectionItemView::postProcessModelChange");

    if (!m_modelReady) {
        MSDEBUG("The model is not ready yet");
        return;
    }

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    restoreItemsState(*pItemModel);
    m_trackingItemsState = true;

    restoreSelectedItems(*pItemModel);
    m_trackingSelection = true;
}

bool AbstractMultiSelectionItemView::trackItemsStateEnabled() const
{
    return m_trackingItemsState;
}

void AbstractMultiSelectionItemView::setTrackItemsStateEnabled(
    const bool enabled)
{
    m_trackingItemsState = enabled;
}

bool AbstractMultiSelectionItemView::trackSelectionEnabled() const
{
    return m_trackingSelection;
}

void AbstractMultiSelectionItemView::setTrackSelectionEnabled(
    const bool enabled)
{
    m_trackingSelection = enabled;
}

void AbstractMultiSelectionItemView::disconnectFromNoteFiltersManagerFilterChanged()
{
    QObject::disconnect(
        m_pNoteFiltersManager.data(), &NoteFiltersManager::filterChanged, this,
        &AbstractMultiSelectionItemView::onNoteFilterChanged);
}

void AbstractMultiSelectionItemView::connectToNoteFiltersManagerFilterChanged()
{
    QObject::connect(
        m_pNoteFiltersManager.data(), &NoteFiltersManager::filterChanged, this,
        &AbstractMultiSelectionItemView::onNoteFilterChanged,
        Qt::UniqueConnection);
}

void AbstractMultiSelectionItemView::saveSelectedItems(
    const Account & account, const QStringList & itemLocalUids)
{
    MSDEBUG(
        "AbstractMultiSelectionItemView::saveSelectedItems: "
            << itemLocalUids.join(QStringLiteral(", ")));

    const QString groupKey = selectedItemsGroupKey();
    const QString arrayKey = selectedItemsArrayKey();
    const QString itemKey = selectedItemsKey();

    ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(groupKey);

    appSettings.beginWriteArray(arrayKey, itemLocalUids.size());

    int i = 0;
    for (const auto & itemLocalUid: qAsConst(itemLocalUids)) {
        appSettings.setArrayIndex(i);
        appSettings.setValue(itemKey, itemLocalUid);
        ++i;
    }

    appSettings.endArray();
    appSettings.endGroup();
}

void AbstractMultiSelectionItemView::restoreSelectedItems(
    const ItemModel & model)
{
    MSDEBUG("AbstractMultiSelectionItemView::restoreSelectedItems");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore last selected items: "
                       "no selection model in the view"))
        return;
    }

    QStringList selectedItemLocalUids;

    if (shouldFilterBySelectedItems(model.account())) {
        if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
            MSDEBUG("Note filters manager is null");
            return;
        }

        if (Q_UNLIKELY(!m_pNoteFiltersManager->isReady())) {
            MSDEBUG("Note filters manager is not ready yet");

            m_restoreSelectedItemsWhenNoteFiltersManagerReady = true;

            QObject::connect(
                m_pNoteFiltersManager.data(), &NoteFiltersManager::ready, this,
                &AbstractMultiSelectionItemView::onNoteFiltersManagerReady,
                Qt::UniqueConnection);

            return;
        }

        selectedItemLocalUids = m_pNoteFiltersManager->tagLocalUidsInFilter();
    }
    else {
        MSDEBUG("Filtering by selected items is switched off");

        const QString groupKey = selectedItemsGroupKey();
        const QString arrayKey = selectedItemsArrayKey();
        const QString itemKey = selectedItemsKey();

        ApplicationSettings appSettings(model.account(), QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(groupKey);

        int size = appSettings.beginReadArray(arrayKey);
        selectedItemLocalUids.reserve(size);

        for (int i = 0; i < size; ++i) {
            appSettings.setArrayIndex(i);

            selectedItemLocalUids
                << appSettings.value(itemKey).toString();
        }

        appSettings.endArray();

        if (selectedItemLocalUids.isEmpty()) {
            // Backward compatibility
            selectedItemLocalUids
                << appSettings.value(itemKey).toString();
        }

        appSettings.endGroup();
    }

    if (selectedItemLocalUids.isEmpty()) {
        MSDEBUG("Found no last selected item local uids");
        return;
    }

    MSTRACE(
        "Selecting item local uids: "
            << selectedItemLocalUids.join(QStringLiteral(", ")));

    QModelIndexList selectedItemIndexes;
    selectedItemIndexes.reserve(selectedItemLocalUids.size());

    for (const auto & selectedItemLocalUid: qAsConst(selectedItemLocalUids)) {
        auto selectedItemIndex = model.indexForLocalUid(selectedItemLocalUid);
        if (Q_UNLIKELY(!selectedItemIndex.isValid())) {
            MSDEBUG(
                "Item model returned invalid index for item local uid: "
                    << selectedItemLocalUid);
            continue;
        }

        selectedItemIndexes << selectedItemIndex;
    }

    if (Q_UNLIKELY(selectedItemIndexes.isEmpty())) {
        MSDEBUG("Found no valid model indexes of last selected items");
        return;
    }

    QItemSelection selection;
    for (const auto & selectedItemIndex: qAsConst(selectedItemIndexes)) {
        selection.select(selectedItemIndex, selectedItemIndex);
    }

    pSelectionModel->select(
        selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void AbstractMultiSelectionItemView::selectAllItemsRootItem(
    const ItemModel & model)
{
    MSDEBUG("AbstractMultiSelectionItemView::selectAllItemsRootItem");

    auto * pSelectionModel = selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't select all items root item: "
                       "no selection model in the view"))
        return;
    }

    m_trackingSelection = false;

    pSelectionModel->select(
        model.index(0, 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);

    m_trackingSelection = true;

    handleNoSelectedItems(model.account());
}

void AbstractMultiSelectionItemView::handleNoSelectedItems(
    const Account & account)
{
    saveSelectedItems(account, QStringList());

    if (shouldFilterBySelectedItems(account)) {
        clearItemsFromNoteFiltersManager();
    }
}

void AbstractMultiSelectionItemView::setItemsToNoteFiltersManager(
    const QStringList & itemLocalUids)
{
    MSDEBUG("AbstractMultiSelectionItemView::setItemsToNoteFiltersManager: "
        << itemLocalUids.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady()) {
        MSDEBUG(
            "Note filters manager is not ready yet, will "
                << "postpone setting item local uids to it: "
                << itemLocalUids.join(QStringLiteral(", ")));

        QObject::connect(
            m_pNoteFiltersManager.data(), &NoteFiltersManager::ready, this,
            &AbstractMultiSelectionItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        m_itemLocalUidsPendingNoteFiltersManagerReadiness = itemLocalUids;
        return;
    }

    if (m_pNoteFiltersManager->isFilterBySearchStringActive()) {
        MSDEBUG(
            "Filter by search string is active, won't set "
                << "the seleted items to filter");
        return;
    }

    const QString & savedSearchLocalUidInFilter =
        m_pNoteFiltersManager->savedSearchLocalUidInFilter();

    if (!savedSearchLocalUidInFilter.isEmpty()) {
        MSDEBUG(
            "Filter by saved search is active, won't set "
                << "the selected items to filter");
        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();

    setItemLocalUidsToNoteFiltersManager(
        itemLocalUids, *m_pNoteFiltersManager);

    connectToNoteFiltersManagerFilterChanged();
}

void AbstractMultiSelectionItemView::clearItemsFromNoteFiltersManager()
{
    MSDEBUG("AbstractMultiSelectionItemView::clearItemsFromNoteFiltersManager");

    if (Q_UNLIKELY(m_pNoteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (!m_pNoteFiltersManager->isReady()) {
        MSDEBUG(
            "Note filters manager is not ready yet, will "
                << "postpone clearing items from it");

        m_itemLocalUidsPendingNoteFiltersManagerReadiness.clear();

        QObject::connect(
            m_pNoteFiltersManager.data(), &NoteFiltersManager::ready, this,
            &AbstractMultiSelectionItemView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    removeItemLocalUidsFromNoteFiltersManager(*m_pNoteFiltersManager);
    connectToNoteFiltersManagerFilterChanged();
}

void AbstractMultiSelectionItemView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    MSTRACE("AbstractMultiSelectionItemView::selectionChangedImpl");

    if (!m_trackingSelection) {
        MSTRACE("Not tracking selection at this time, skipping");
        return;
    }

    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (Q_UNLIKELY(!pItemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    const auto & account = pItemModel->account();

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("The new selection is empty");
        handleNoSelectedItems(account);
        return;
    }

    // FIXME: it should not be possible to have selected items and all items
    // root item simultaneously - need to figure out what to filter out from the
    // selection using "selected" and "deselected"

    QStringList itemLocalUids;

    for (const auto & selectedIndex: qAsConst(indexes)) {
        if (!selectedIndex.isValid()) {
            continue;
        }

        // A way to ensure only one index per row
        if (selectedIndex.column() != 0) {
            continue;
        }

        QString localUid = pItemModel->localUidForItemIndex(selectedIndex);
        if (!localUid.isEmpty()) {
            itemLocalUids << localUid;
        }
    }

    if (Q_UNLIKELY(itemLocalUids.isEmpty())) {
        MSDEBUG("Found no items within the selected indexes");
        handleNoSelectedItems(account);
        return;
    }

    saveSelectedItems(account, itemLocalUids);

    if (shouldFilterBySelectedItems(account)) {
        setItemsToNoteFiltersManager(itemLocalUids);
    }
    else {
        MSDEBUG("Filtering by selected items is switched off");
    }
}

} // namespace quentier
