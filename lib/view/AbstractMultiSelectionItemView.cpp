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

#include <lib/model/ItemModel.h>
#include <lib/widget/NoteFiltersManager.h>

#include <quentier/logging/QuentierLogger.h>

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

    const auto & localUids = localUidsFromNoteFiltersManager(
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
        &AbstractMultiSelectionItemView::onNoteFilterChanged, Qt::UniqueConnection);
}

} // namespace quentier
