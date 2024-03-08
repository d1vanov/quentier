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

#include "AbstractNoteFilteringTreeView.h"
#include "ItemSelectionModel.h"

#include <lib/model/common/AbstractItemModel.h>
#include <lib/preferences/keys/Files.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>

#include <utility>

namespace quentier {

#define MSLOG_BASE(level, message)                                             \
    QNLOG_PRIVATE_BASE(                                                        \
        (QString::fromUtf8("view:") + m_modelTypeName).toUtf8(),               \
        "[" << m_modelTypeName << "]: " << message, level)

#define MSTRACE(message)   MSLOG_BASE(Trace, message)
#define MSDEBUG(message)   MSLOG_BASE(Debug, message)
#define MSINFO(message)    MSLOG_BASE(Info, message)
#define MSWARNING(message) MSLOG_BASE(Warning, message)
#define MSERROR(message)   MSLOG_BASE(Error, message)

#define REPORT_ERROR(error)                                                    \
    {                                                                          \
        ErrorString errorDescription(error);                                   \
        MSWARNING(errorDescription);                                           \
        Q_EMIT notifyError(errorDescription);                                  \
    }

AbstractNoteFilteringTreeView::AbstractNoteFilteringTreeView(
    QString modelTypeName, QWidget * parent) :
    TreeView{parent},
    m_modelTypeName{std::move(modelTypeName)}
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);

    QObject::connect(
        this, &AbstractNoteFilteringTreeView::expanded, this,
        &AbstractNoteFilteringTreeView::onItemCollapsedOrExpanded);

    QObject::connect(
        this, &AbstractNoteFilteringTreeView::collapsed, this,
        &AbstractNoteFilteringTreeView::onItemCollapsedOrExpanded);
}

AbstractNoteFilteringTreeView::~AbstractNoteFilteringTreeView() = default;

void AbstractNoteFilteringTreeView::setNoteFiltersManager(
    NoteFiltersManager & noteFiltersManager)
{
    if (!m_noteFiltersManager.isNull()) {
        if (m_noteFiltersManager.data() == &noteFiltersManager) {
            MSDEBUG("Already using this note filters manager");
            return;
        }

        disconnectFromNoteFiltersManagerFilterChanged();
    }

    m_noteFiltersManager = &noteFiltersManager;
    connectToNoteFiltersManagerFilterChanged();
}

void AbstractNoteFilteringTreeView::setModel(QAbstractItemModel * model)
{
    MSDEBUG("AbstractNoteFilteringTreeView::setModel");

    auto * previousModel = qobject_cast<AbstractItemModel *>(this->model());
    if (previousModel) {
        previousModel->disconnect(this);
    }

    m_itemLocalIdsPendingNoteFiltersManagerReadiness.clear();
    m_modelReady = false;
    m_trackingSelection = false;
    m_trackingItemsState = false;

    auto * itemModel = qobject_cast<AbstractItemModel *>(model);
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model has been set to the item view");
        TreeView::setModel(model);
        return;
    }

    connectToModel(*itemModel);

    TreeView::setModel(model);

    auto * oldSelectionModel = selectionModel();
    setSelectionModel(new ItemSelectionModel(itemModel, this));
    if (oldSelectionModel) {
        oldSelectionModel->disconnect(this);
        QObject::disconnect(oldSelectionModel);
        oldSelectionModel->deleteLater();
    }

    if (itemModel->allItemsListed()) {
        MSDEBUG("All items are already listed within the model");
        restoreItemsState(*itemModel);
        restoreSelectedItems(*itemModel);
        m_modelReady = true;
        m_trackingSelection = true;
        m_trackingItemsState = true;
        return;
    }

    QObject::connect(
        itemModel, &AbstractItemModel::notifyAllItemsListed, this,
        &AbstractNoteFilteringTreeView::onAllItemsListed);
}

QModelIndex AbstractNoteFilteringTreeView::currentlySelectedItemIndex() const
{
    MSDEBUG("AbstractNoteFilteringTreeView::currentlySelectedItemIndex");

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return {};
    }

    const auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("The selection contains no model indexes");
        return {};
    }

    return singleRow(indexes, *itemModel, 0);
}

void AbstractNoteFilteringTreeView::deleteSelectedItem()
{
    MSDEBUG("AbstractNoteFilteringTreeView::deleteSelectedItem");

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    const auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("No items are selected, nothing to deete");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot delete current item"),
            tr("No item is selected currently"),
            tr("Please select the item you want to delete")))

        return;
    }

    const auto index = singleRow(indexes, *itemModel, 0);
    if (!index.isValid()) {
        MSDEBUG("Not exactly one item within the selection");

        Q_UNUSED(informationMessageBox(
            this, tr("Cannot delete current item"),
            tr("More than one item is currently selected"),
            tr("Please select only the item you want to delete")))

        return;
    }

    deleteItem(index, *itemModel);
}

void AbstractNoteFilteringTreeView::onAllItemsListed()
{
    MSDEBUG("AbstractNoteFilteringTreeView::onAllItemsListed");

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't cast the model set to the item view "
                       "to the item model"))
        return;
    }

    QObject::disconnect(
        itemModel, &AbstractItemModel::notifyAllItemsListed, this,
        &AbstractNoteFilteringTreeView::onAllItemsListed);

    restoreItemsState(*itemModel);
    restoreSelectedItems(*itemModel);

    m_modelReady = true;
    m_trackingSelection = true;
    m_trackingItemsState = true;
}

void AbstractNoteFilteringTreeView::onItemCollapsedOrExpanded(
    const QModelIndex & index)
{
    MSTRACE(
        "AbstractNoteFilteringTreeView::onItemCollapsedOrExpanded: "
        << "index: valid = " << (index.isValid() ? "true" : "false")
        << ", row = " << index.row() << ", column = " << index.column()
        << ", parent: valid = " << (index.parent().isValid() ? "true" : "false")
        << ", row = " << index.parent().row()
        << ", column = " << index.parent().column());

    if (!m_trackingItemsState) {
        MSDEBUG("Not tracking the items state at this moment");
        return;
    }

    saveItemsState();
}

void AbstractNoteFilteringTreeView::onNoteFilterChanged()
{
    MSDEBUG("AbstractNoteFilteringTreeView::onNoteFilterChanged");

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    if (!shouldFilterBySelectedItems(itemModel->account())) {
        MSDEBUG("Filtering by selected items is disabled, won't do anything");
        return;
    }

    if (Q_UNLIKELY(m_noteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (m_noteFiltersManager->isFilterBySearchStringActive()) {
        MSDEBUG("Filter by search string is active");
        selectAllItemsRootItem(*itemModel);
        return;
    }

    const auto localIds = localIdsInNoteFiltersManager(*m_noteFiltersManager);

    if (localIds.isEmpty()) {
        MSDEBUG("No items in note filter");
        selectAllItemsRootItem(*itemModel);
        return;
    }

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        MSWARNING("No selection model, can't update selection");
        return;
    }

    // Check whether current selection matches item local ids
    bool selectionIsActual = true;
    const auto indexes = selectionModel->selectedIndexes();
    if (indexes.size() != localIds.size()) {
        selectionIsActual = false;
    }
    else {
        for (const auto & index: std::as_const(indexes)) {
            const auto localId = itemModel->localIdForItemIndex(index);
            if (localId.isEmpty()) {
                selectionIsActual = false;
                break;
            }

            if (!localIds.contains(localId)) {
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
    for (const auto & localId: std::as_const(localIds)) {
        auto index = itemModel->indexForLocalId(localId);
        selection.select(index, index);
    }

    selectionModel->select(
        selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void AbstractNoteFilteringTreeView::onNoteFiltersManagerReady()
{
    MSDEBUG("AbstractNoteFilteringTreeView::onNoteFiltersManagerReady");

    if (Q_UNLIKELY(m_noteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    QObject::disconnect(
        m_noteFiltersManager.data(), &NoteFiltersManager::ready, this,
        &AbstractNoteFilteringTreeView::onNoteFiltersManagerReady);

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    if (m_restoreSelectedItemsWhenNoteFiltersManagerReady) {
        m_restoreSelectedItemsWhenNoteFiltersManagerReady = false;

        if (m_itemLocalIdsPendingNoteFiltersManagerReadiness.isEmpty()) {
            restoreSelectedItems(*itemModel);
            return;
        }
    }

    QStringList itemLocalIds = m_itemLocalIdsPendingNoteFiltersManagerReadiness;
    m_itemLocalIdsPendingNoteFiltersManagerReadiness.clear();

    const auto & account = itemModel->account();
    saveSelectedItems(account, itemLocalIds);

    if (shouldFilterBySelectedItems(account)) {
        if (!itemLocalIds.isEmpty()) {
            setItemsToNoteFiltersManager(itemLocalIds);
        }
        else {
            clearItemsFromNoteFiltersManager();
        }
    }
}

void AbstractNoteFilteringTreeView::selectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    MSDEBUG("AbstractNoteFilteringTreeView::selectionChanged");

    selectionChangedImpl(selected, deselected);
    TreeView::selectionChanged(selected, deselected);
}

void AbstractNoteFilteringTreeView::prepareForModelChange()
{
    MSDEBUG("AbstractNoteFilteringTreeView::prepareForModelChange");

    if (!m_modelReady) {
        MSDEBUG("The model is not ready yet");
        return;
    }

    saveItemsState();
    m_trackingSelection = false;
    m_trackingItemsState = false;
}

void AbstractNoteFilteringTreeView::postProcessModelChange()
{
    MSDEBUG("AbstractNoteFilteringTreeView::postProcessModelChange");

    if (!m_modelReady) {
        MSDEBUG("The model is not ready yet");
        return;
    }

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    restoreItemsState(*itemModel);
    m_trackingItemsState = true;

    restoreSelectedItems(*itemModel);
    m_trackingSelection = true;
}

bool AbstractNoteFilteringTreeView::trackItemsStateEnabled() const noexcept
{
    return m_trackingItemsState;
}

void AbstractNoteFilteringTreeView::setTrackItemsStateEnabled(
    const bool enabled) noexcept
{
    m_trackingItemsState = enabled;
}

bool AbstractNoteFilteringTreeView::trackSelectionEnabled() const noexcept
{
    return m_trackingSelection;
}

void AbstractNoteFilteringTreeView::setTrackSelectionEnabled(
    const bool enabled) noexcept
{
    m_trackingSelection = enabled;
}

void AbstractNoteFilteringTreeView::saveAllItemsRootItemExpandedState(
    ApplicationSettings & appSettings, const std::string_view settingsKey,
    const QModelIndex & allItemsRootItemIndex)
{
    // Will not save the state if the item is not expanded + there is no
    // existing entry within the app settings. Because by default items in Qt's
    // QTreeView are not expanded and saving this state in persistent
    // preferences might be premature because by default it makes sense to have
    // these root items expanded rather than not expanded.
    const bool expanded = isExpanded(allItemsRootItemIndex);
    if (!expanded && !appSettings.contains(settingsKey)) {
        return;
    }

    appSettings.setValue(settingsKey, expanded);
}

void AbstractNoteFilteringTreeView::
    disconnectFromNoteFiltersManagerFilterChanged()
{
    QObject::disconnect(
        m_noteFiltersManager.data(), &NoteFiltersManager::filterChanged, this,
        &AbstractNoteFilteringTreeView::onNoteFilterChanged);
}

void AbstractNoteFilteringTreeView::connectToNoteFiltersManagerFilterChanged()
{
    QObject::connect(
        m_noteFiltersManager.data(), &NoteFiltersManager::filterChanged, this,
        &AbstractNoteFilteringTreeView::onNoteFilterChanged,
        Qt::UniqueConnection);
}

void AbstractNoteFilteringTreeView::saveSelectedItems(
    const Account & account, const QStringList & itemLocalIds)
{
    MSDEBUG(
        "AbstractNoteFilteringTreeView::saveSelectedItems: "
        << itemLocalIds.join(QStringLiteral(", ")));

    const QString groupKey = selectedItemsGroupKey();
    const QString arrayKey = selectedItemsArrayKey();
    const QString itemKey = selectedItemsKey();

    ApplicationSettings appSettings(
        account, preferences::keys::files::userInterface);

    appSettings.beginGroup(groupKey);
    appSettings.beginWriteArray(arrayKey, itemLocalIds.size());

    int i = 0;
    for (const auto & itemLocalId: std::as_const(itemLocalIds)) {
        appSettings.setArrayIndex(i);
        appSettings.setValue(itemKey, itemLocalId);
        ++i;
    }

    appSettings.endArray();
    appSettings.endGroup();
}

void AbstractNoteFilteringTreeView::restoreSelectedItems(
    const AbstractItemModel & model)
{
    MSDEBUG("AbstractNoteFilteringTreeView::restoreSelectedItems");

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't restore last selected items: "
                       "no selection model in the view"))
        return;
    }

    QStringList selectedItemLocalIds;

    if (shouldFilterBySelectedItems(model.account())) {
        if (Q_UNLIKELY(m_noteFiltersManager.isNull())) {
            MSDEBUG("Note filters manager is null");
            return;
        }

        if (Q_UNLIKELY(!m_noteFiltersManager->isReady())) {
            MSDEBUG("Note filters manager is not ready yet");

            m_restoreSelectedItemsWhenNoteFiltersManagerReady = true;

            QObject::connect(
                m_noteFiltersManager.data(), &NoteFiltersManager::ready, this,
                &AbstractNoteFilteringTreeView::onNoteFiltersManagerReady,
                Qt::UniqueConnection);

            return;
        }

        selectedItemLocalIds =
            localIdsInNoteFiltersManager(*m_noteFiltersManager);
    }
    else {
        MSDEBUG("Filtering by selected items is switched off");

        const QString groupKey = selectedItemsGroupKey();
        const QString arrayKey = selectedItemsArrayKey();
        const QString itemKey = selectedItemsKey();

        ApplicationSettings appSettings(
            model.account(), preferences::keys::files::userInterface);

        appSettings.beginGroup(groupKey);

        int size = appSettings.beginReadArray(arrayKey);
        selectedItemLocalIds.reserve(size);

        for (int i = 0; i < size; ++i) {
            appSettings.setArrayIndex(i);

            selectedItemLocalIds << appSettings.value(itemKey).toString();
        }

        appSettings.endArray();

        if (selectedItemLocalIds.isEmpty()) {
            // Backward compatibility
            selectedItemLocalIds << appSettings.value(itemKey).toString();
        }

        appSettings.endGroup();
    }

    if (selectedItemLocalIds.isEmpty()) {
        MSDEBUG("Found no last selected item local ids");
        return;
    }

    MSTRACE(
        "Selecting item local ids: "
        << selectedItemLocalIds.join(QStringLiteral(", ")));

    QModelIndexList selectedItemIndexes;
    selectedItemIndexes.reserve(selectedItemLocalIds.size());

    for (const auto & selectedItemLocalId: std::as_const(selectedItemLocalIds))
    {
        const auto selectedItemIndex =
            model.indexForLocalId(selectedItemLocalId);
        if (Q_UNLIKELY(!selectedItemIndex.isValid())) {
            MSDEBUG(
                "Item model returned invalid index for item local id: "
                << selectedItemLocalId);
            continue;
        }

        selectedItemIndexes << selectedItemIndex;
    }

    if (Q_UNLIKELY(selectedItemIndexes.isEmpty())) {
        MSDEBUG("Found no valid model indexes of last selected items");
        return;
    }

    QItemSelection selection;
    for (const auto & selectedItemIndex: std::as_const(selectedItemIndexes)) {
        selection.select(selectedItemIndex, selectedItemIndex);
    }

    selectionModel->select(
        selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);
}

void AbstractNoteFilteringTreeView::selectAllItemsRootItem(
    const AbstractItemModel & model)
{
    MSDEBUG("AbstractNoteFilteringTreeView::selectAllItemsRootItem");

    auto * selectionModel = this->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't select all items root item: "
                       "no selection model in the view"))
        return;
    }

    m_trackingSelection = false;

    selectionModel->select(
        model.allItemsRootItemIndex(),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows |
            QItemSelectionModel::Current);

    m_trackingSelection = true;

    handleNoSelectedItems(model.account());
}

void AbstractNoteFilteringTreeView::handleNoSelectedItems(
    const Account & account)
{
    saveSelectedItems(account, QStringList());

    if (shouldFilterBySelectedItems(account)) {
        clearItemsFromNoteFiltersManager();
    }
}

void AbstractNoteFilteringTreeView::setItemsToNoteFiltersManager(
    const QStringList & itemLocalIds)
{
    MSDEBUG(
        "AbstractNoteFilteringTreeView::setItemsToNoteFiltersManager: "
        << itemLocalIds.join(QStringLiteral(", ")));

    if (Q_UNLIKELY(m_noteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (!m_noteFiltersManager->isReady()) {
        MSDEBUG(
            "Note filters manager is not ready yet, will "
            << "postpone setting item local ids to it: "
            << itemLocalIds.join(QStringLiteral(", ")));

        QObject::connect(
            m_noteFiltersManager.data(), &NoteFiltersManager::ready, this,
            &AbstractNoteFilteringTreeView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        m_itemLocalIdsPendingNoteFiltersManagerReadiness = itemLocalIds;
        return;
    }

    if (m_noteFiltersManager->isFilterBySearchStringActive()) {
        MSDEBUG(
            "Filter by search string is active, won't set "
            << "the seleted items to filter");
        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    setItemLocalIdsToNoteFiltersManager(itemLocalIds, *m_noteFiltersManager);
    connectToNoteFiltersManagerFilterChanged();
}

void AbstractNoteFilteringTreeView::clearItemsFromNoteFiltersManager()
{
    MSDEBUG("AbstractNoteFilteringTreeView::clearItemsFromNoteFiltersManager");

    if (Q_UNLIKELY(m_noteFiltersManager.isNull())) {
        MSDEBUG("Note filters manager is null");
        return;
    }

    if (!m_noteFiltersManager->isReady()) {
        MSDEBUG(
            "Note filters manager is not ready yet, will "
            << "postpone clearing items from it");

        m_itemLocalIdsPendingNoteFiltersManagerReadiness.clear();

        QObject::connect(
            m_noteFiltersManager.data(), &NoteFiltersManager::ready, this,
            &AbstractNoteFilteringTreeView::onNoteFiltersManagerReady,
            Qt::UniqueConnection);

        return;
    }

    disconnectFromNoteFiltersManagerFilterChanged();
    removeItemLocalIdsFromNoteFiltersManager(*m_noteFiltersManager);
    connectToNoteFiltersManagerFilterChanged();
}

void AbstractNoteFilteringTreeView::selectionChangedImpl(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    MSTRACE("AbstractNoteFilteringTreeView::selectionChangedImpl");

    if (!m_trackingSelection) {
        MSTRACE("Not tracking selection at this time, skipping");
        return;
    }

    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (Q_UNLIKELY(!itemModel)) {
        MSDEBUG("Non-item model is used");
        return;
    }

    const auto & account = itemModel->account();

    auto indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        MSDEBUG("The new selection is empty");
        handleNoSelectedItems(account);
        return;
    }

    // FIXME: it should not be possible to have selected items and all items
    // root item simultaneously - need to figure out what to filter out from the
    // selection using "selected" and "deselected"

    QStringList itemLocalIds;

    for (const auto & selectedIndex: std::as_const(indexes)) {
        if (!selectedIndex.isValid()) {
            continue;
        }

        // A way to ensure only one index per row
        if (selectedIndex.column() != itemModel->nameColumn()) {
            continue;
        }

        QString localId = itemModel->localIdForItemIndex(selectedIndex);
        if (localId.isEmpty()) {
            continue;
        }

        itemLocalIds << localId;
        processSelectedItem(localId, *itemModel);
    }

    if (Q_UNLIKELY(itemLocalIds.isEmpty())) {
        MSDEBUG("Found no items within the selected indexes");
        handleNoSelectedItems(account);
        return;
    }

    saveSelectedItems(account, itemLocalIds);

    if (shouldFilterBySelectedItems(account)) {
        setItemsToNoteFiltersManager(itemLocalIds);
    }
    else {
        MSDEBUG("Filtering by selected items is switched off");
    }
}

} // namespace quentier
