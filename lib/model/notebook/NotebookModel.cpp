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

#include "NotebookModel.h"

#include "AllNotebooksRootItem.h"
#include "InvisibleNotebookRootItem.h"

#include <lib/exception/Utils.h>
#include <lib/model/common/NewItemNameGenerator.hpp>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <QDataStream>
#include <QMimeData>

#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription{error};                                       \
    QNWARNING("model::NotebookModel", errorDescription << "" __VA_ARGS__);     \
    Q_EMIT notifyError(std::move(errorDescription)) // REPORT_ERROR

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr int gNotebookModelColumnCount = 7;

const auto gMimeType = "application/x-com.quentier.notebookmodeldatalist"sv;

} // namespace

NotebookModel::NotebookModel(
    Account account, local_storage::ILocalStoragePtr localStorage,
    NotebookCache & cache, QObject * parent) :
    AbstractItemModel{std::move(account), parent},
    m_localStorage{std::move(localStorage)}, m_cache{cache}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{QStringLiteral(
            "NotebookModel ctor: local storage is null")}};
    }
}

NotebookModel::~NotebookModel()
{
    delete m_allNotebooksRootItem;
    delete m_invisibleRootItem;
}

void NotebookModel::setAccount(Account account)
{
    QNTRACE("model::NotebookModel", "NotebookModel::setAccount: " << account);
    m_account = std::move(account);
}

INotebookModelItem * NotebookModel::itemForIndex(
    const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_invisibleRootItem;
    }

    return itemForId(static_cast<IndexId>(index.internalId()));
}

QModelIndex NotebookModel::indexForItem(const INotebookModelItem * item) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::indexForItem: "
            << (item ? item->toString() : QStringLiteral("<null>")));

    if (!item) {
        return {};
    }

    if (item == m_invisibleRootItem) {
        return {};
    }

    if (item == m_allNotebooksRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allNotebooksRootItemIndexId);
    }

    const auto * parentItem = item->parent();
    if (!parentItem) {
        QNWARNING(
            "model::NotebookModel",
            "Notebook model item has no parent, returning invalid index for "
                << "it: " << *item);
        return {};
    }

    QNTRACE("model::NotebookModel", "Parent item: " << *parentItem);
    const auto row = parentItem->rowForChild(item);
    QNTRACE("model::NotebookModel", "Child row: " << row);

    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Internal error: can't get the row of the child item in parent in "
                << "NotebookModel, child item: "
                << *item << "\nParent item: " << *parentItem);
        return {};
    }

    const auto id = idForItem(*item);
    if (Q_UNLIKELY(id == 0)) {
        QNWARNING(
            "model::NotebookModel",
            "The notebook model item has the internal id of 0: " << *item);
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

QModelIndex NotebookModel::indexForNotebookName(
    const QString & notebookName, const QString & linkedNotebookGuid) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();

    const auto range = nameIndex.equal_range(notebookName.toUpper());
    for (auto it = range.first; it != range.second; ++it) {
        const auto & item = *it;
        if (item.linkedNotebookGuid() == linkedNotebookGuid) {
            return indexForLocalId(item.localId());
        }
    }

    return {};
}

QModelIndex NotebookModel::indexForNotebookStack(
    const QString & stack, const QString & linkedNotebookGuid) const
{
    const auto * stackItems = findStackItems(linkedNotebookGuid);
    if (Q_UNLIKELY(!stackItems)) {
        return {};
    }

    const auto it = stackItems->find(stack);
    if (it == stackItems->end()) {
        return {};
    }

    const auto & stackItem = it.value();
    return indexForItem(&stackItem);
}

QModelIndex NotebookModel::indexForLinkedNotebookGuid(
    const QString & linkedNotebookGuid) const
{
    const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it == m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model::NotebookModel",
            "Found no model item for linked notebook guid "
                << linkedNotebookGuid);
        return {};
    }

    const LinkedNotebookRootItem & item = it.value();
    return indexForItem(&item);
}

QStringList NotebookModel::notebookNames(
    const Filters filters, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::notebookNames: filters = "
            << filters << ", linked notebook guid = " << linkedNotebookGuid
            << " (null = " << (linkedNotebookGuid.isNull() ? "true" : "false")
            << ", empty = " << (linkedNotebookGuid.isEmpty() ? "true" : "false")
            << ")");

    if (filters == Filters{Filter::NoFilter}) {
        QNTRACE("model::NotebookModel", "No filter, returning all notebook names");
        return itemNames(linkedNotebookGuid);
    }

    QStringList result;

    if ((filters & Filter::CanCreateNotes) &&
        (filters & Filter::CannotCreateNotes)) {
        QNTRACE(
            "model::NotebookModel",
            "Both can create notes and cannot create "
                << "notes filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotes) &&
        (filters & Filter::CannotUpdateNotes)) {
        QNTRACE(
            "model::NotebookModel",
            "Both can update notes and cannot update "
                << "notes filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotebook) &&
        (filters & Filter::CannotUpdateNotebook))
    {
        QNTRACE(
            "model::NotebookModel",
            "Both can update notebooks and cannot update "
                << "notebooks filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanRenameNotebook) &&
        (filters & Filter::CannotRenameNotebook))
    {
        QNTRACE(
            "model::NotebookModel",
            "Both can rename notebook and cannot rename "
                << "notebook filters are inclided, the result is empty");
        return result;
    }

    const auto & nameIndex = m_data.get<ByNameUpper>();
    result.reserve(static_cast<int>(nameIndex.size()));
    for (auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it) {
        const auto & item = *it;
        if (!notebookItemMatchesByLinkedNotebook(item, linkedNotebookGuid)) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item with different linked notebook guid: "
                    << item);
            continue;
        }

        const bool canCreateNotes = item.canCreateNotes();
        if ((filters & Filter::CanCreateNotes) && !canCreateNotes) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item for which notes cannot be created: "
                    << item);
            continue;
        }

        if ((filters & Filter::CannotCreateNotes) && canCreateNotes) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item for which notes can be created: "
                    << item);
            continue;
        }

        const bool canUpdateNotes = item.canUpdateNotes();
        if ((filters & Filter::CanUpdateNotes) && !canUpdateNotes) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item for which notes cannot be updated: "
                    << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotes) && canUpdateNotes) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item for which notes can be updated: "
                    << item);
            continue;
        }

        const bool canUpdateNotebook = item.isUpdatable();
        if ((filters & Filter::CanUpdateNotebook) && !canUpdateNotebook) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item which cannot be updated: " << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotebook) && canUpdateNotebook) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item which can be updated: " << item);
            continue;
        }

        const bool canRenameNotebook = item.nameIsUpdatable();
        if ((filters & Filter::CanRenameNotebook) && !canRenameNotebook) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item which cannot be renamed: " << item);
            continue;
        }

        if ((filters & Filter::CannotRenameNotebook) && canRenameNotebook) {
            QNTRACE(
                "model::NotebookModel",
                "Skipping notebook item which can be renamed: " << item);
            continue;
        }

        result << it->name();
    }

    return result;
}

QModelIndex NotebookModel::defaultNotebookIndex() const
{
    QNTRACE("model::NotebookModel", "NotebookModel::defaultNotebookIndex");

    if (m_defaultNotebookLocalId.isEmpty()) {
        QNDEBUG("model::NotebookModel", "No default notebook local id");
        return {};
    }

    return indexForLocalId(m_defaultNotebookLocalId);
}

QModelIndex NotebookModel::moveToStack(
    const QModelIndex & index, const QString & stack)
{
    QNDEBUG("model::NotebookModel", "NotebookModel::moveToStack: stack = " << stack);

    if (Q_UNLIKELY(stack.isEmpty())) {
        return removeFromStack(index);
    }

    auto * modelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to move "
                       "a notebook to another stack but the respective "
                       "model index has no internal id corresponding "
                       "to the notebook model item"));
        return {};
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (!notebookItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move the non-notebook item into a stack"));
        return {};
    }

    QNTRACE(
        "model::NotebookModel",
        "Notebook item to be moved to another stack: " << *notebookItem);

    if (notebookItem->stack() == stack) {
        QNDEBUG(
            "model::NotebookModel",
            "The stack of the item hasn't changed, nothing to do");
        return index;
    }

    removeModelItemFromParent(*modelItem);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(notebookItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find the notebook "
                       "item to be moved to another stack"));
        return {};
    }

    auto stackItemsWithParentPair =
        stackItemsWithParent(notebookItem->linkedNotebookGuid());

    auto & stackItems = *stackItemsWithParentPair.first;
    auto * pStackParentItem = stackItemsWithParentPair.second;

    auto & stackItem =
        findOrCreateStackItem(stack, stackItems, pStackParentItem);

    NotebookItem notebookItemCopy{*notebookItem};
    notebookItemCopy.setStack(stack);
    notebookItemCopy.setDirty(true);

    localIdIndex.replace(it, notebookItemCopy);
    updateNotebookInLocalStorage(notebookItemCopy);

    auto * newParentItem = &stackItem;
    auto parentIndex = indexForItem(newParentItem);

    const int newRow = rowForNewItem(*newParentItem, *modelItem);
    beginInsertRows(parentIndex, newRow, newRow);
    newParentItem->insertChild(newRow, modelItem);
    endInsertRows();

    QNTRACE(
        "model::NotebookModel",
        "Model item after moving to the stack: " << *modelItem);

    Q_EMIT notifyNotebookStackChanged(index);
    return indexForItem(modelItem);
}

QModelIndex NotebookModel::removeFromStack(const QModelIndex & index)
{
    QNDEBUG("model::NotebookModel", "NotebookModel::removeFromStack");

    auto * modelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find a notebook to be "
                       "removed from the stack, no item corresponding to id"));
        return {};
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!notebookItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't remove the non-notebook model item from "
                       "the stack"));
        return {};
    }

    if (notebookItem->stack().isEmpty()) {
        QNWARNING(
            "model::NotebookModel",
            "The notebook doesn't appear to have a stack but will continue the "
            "attempt to remove it from the stack anyway");
    }
    else {
        auto & localIdIndex = m_data.get<ByLocalId>();
        const auto it = localIdIndex.find(notebookItem->localId());
        if (Q_UNLIKELY(it == localIdIndex.end())) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't find the notebook item to be removed "
                           "from the stack by the local id"));
            return {};
        }

        NotebookItem notebookItemCopy{*notebookItem};
        notebookItemCopy.setStack(QString{});
        notebookItemCopy.setDirty(true);
        localIdIndex.replace(it, notebookItemCopy);

        updateNotebookInLocalStorage(notebookItemCopy);
    }

    auto * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING(
            "model::NotebookModel",
            "Notebook item to be removed from the stack doesn't have a parent, "
            "setting it to all notebooks root item");

        checkAndCreateModelRootItems();

        const int newRow = rowForNewItem(*m_allNotebooksRootItem, *modelItem);
        beginInsertRows(indexForItem(m_allNotebooksRootItem), newRow, newRow);
        m_allNotebooksRootItem->insertChild(newRow, modelItem);
        endInsertRows();

        return indexForItem(modelItem);
    }

    if (parentItem->type() != INotebookModelItem::Type::Stack) {
        QNDEBUG(
            "model::NotebookModel",
            "The notebook item doesn't belong to any stack, nothing to do");
        return index;
    }

    checkAndCreateModelRootItems();
    const QString & linkedNotebookGuid = notebookItem->linkedNotebookGuid();

    auto * newParentItem =
        (linkedNotebookGuid.isEmpty()
             ? m_allNotebooksRootItem
             : &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid)));

    removeModelItemFromParent(*modelItem);

    const QModelIndex parentItemIndex = indexForItem(newParentItem);

    const int newRow = rowForNewItem(*newParentItem, *modelItem);
    beginInsertRows(parentItemIndex, newRow, newRow);
    newParentItem->insertChild(newRow, modelItem);
    endInsertRows();

    QNTRACE(
        "model::NotebookModel",
        "Model item after removing it from the stack: " << *modelItem);

    Q_EMIT notifyNotebookStackChanged(index);
    return indexForItem(modelItem);
}

QStringList NotebookModel::stacks(const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::stacks: linked notebook guid = " << linkedNotebookGuid);

    QStringList result;

    const StackItems * stackItems = nullptr;
    if (linkedNotebookGuid.isEmpty()) {
        stackItems = &m_stackItems;
    }
    else {
        const auto stackItemsIt =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

        if (stackItemsIt == m_stackItemsByLinkedNotebookGuid.end()) {
            QNTRACE(
                "model::NotebookModel",
                "Found no stacks for this linked notebook guid");
            return result;
        }

        stackItems = &(stackItemsIt.value());
    }

    result.reserve(stackItems->size());

    for (const auto it: qevercloud::toRange(std::as_const(*stackItems))) {
        result.push_back(it.value().name());
    }

    QNTRACE(
        "model::NotebookModel",
        "Collected stacks: " << result.join(QStringLiteral(", ")));

    return result;
}

const QHash<QString, QString> & NotebookModel::linkedNotebookOwnerNamesByGuid()
    const noexcept
{
    return m_linkedNotebookUsernamesByGuids;
}

QModelIndex NotebookModel::createNotebook(
    const QString & notebookName, const QString & notebookStack,
    ErrorString & errorDescription)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::createNotebook: notebook name = "
            << notebookName << ", notebook stack = " << notebookStack);

    if (notebookName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Notebook name is empty"));
        return {};
    }

    const auto notebookNameSize = notebookName.size();
    if (notebookNameSize < qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN) {
        errorDescription.setBase(
            QT_TR_NOOP("Notebook name size is below "
                       "the minimal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN);

        return {};
    }

    if (notebookNameSize > qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX) {
        errorDescription.setBase(
            QT_TR_NOOP("Notebook name size is above "
                       "the maximal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX);

        return {};
    }

    QModelIndex existingItemIndex = indexForNotebookName(notebookName);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Notebook with such name already exists"));

        return {};
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const int numExistingNotebooks = static_cast<int>(localIdIndex.size());
    if (Q_UNLIKELY(numExistingNotebooks + 1 >= m_account.notebookCountMax())) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new notebook: "
                       "the account can contain a limited "
                       "number of notebooks"));

        errorDescription.details() =
            QString::number(m_account.notebookCountMax());

        return {};
    }

    checkAndCreateModelRootItems();
    auto * parentItem = m_allNotebooksRootItem;

    Q_EMIT aboutToAddNotebook();

    if (!notebookStack.isEmpty()) {
        parentItem = &(findOrCreateStackItem(
            notebookStack, m_stackItems, m_allNotebooksRootItem));

        QNDEBUG(
            "model::NotebookModel",
            "Will put the new notebook under parent "
                << "stack item: " << *parentItem);
    }

    const auto parentIndex = indexForItem(parentItem);

    // Will insert the notebook to the end of the parent item's children
    const int row = parentItem->childrenCount();

    NotebookItem item;
    item.setLocalId(UidGenerator::Generate());
    m_notebookItemsNotYetInLocalStorageIds.insert(item.localId());

    item.setName(notebookName);
    item.setDirty(true);
    item.setStack(notebookStack);
    item.setSynchronizable(m_account.type() != Account::Type::Local);
    item.setDefault(numExistingNotebooks == 0);
    item.setUpdatable(true);
    item.setNameIsUpdatable(true);

    const auto insertionResult = localIdIndex.insert(item);

    auto * addedNotebookItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    addedNotebookItem->setParent(parentItem);
    endInsertRows();

    updateNotebookInLocalStorage(*addedNotebookItem);

    auto addedNotebookIndex = indexForLocalId(item.localId());

    if (m_sortedColumn != Column::Name) {
        QNDEBUG(
            "model::NotebookModel",
            "Notebook model is not sorted by name, skipping sorting");
        Q_EMIT addedNotebook(addedNotebookIndex);
        return addedNotebookIndex;
    }

    Q_EMIT layoutAboutToBeChanged();
    for (auto & item: localIdIndex) {
        updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
    }
    Q_EMIT layoutChanged();

    // Need to update the index as the item's row could have changed as a result
    // of sorting
    addedNotebookIndex = indexForLocalId(item.localId());

    Q_EMIT addedNotebook(addedNotebookIndex);
    return addedNotebookIndex;
}

QString NotebookModel::columnName(const NotebookModel::Column column) const
{
    switch (column) {
    case Column::Name:
        return tr("Name");
    case Column::Synchronizable:
        return tr("Synchronizable");
    case Column::Dirty:
        return tr("Changed");
    case Column::Default:
        return tr("Default");
    case Column::Published:
        return tr("Published");
    case Column::FromLinkedNotebook:
        return tr("External");
    case Column::NoteCount:
        return tr("Note count");
    default:
        return {};
    }
}

bool NotebookModel::allNotebooksListed() const noexcept
{
    return m_allNotebooksListed && m_allLinkedNotebooksListed;
}

void NotebookModel::favoriteNotebook(const QModelIndex & index)
{
    QNDEBUG(
        "model::NotebookModel",
        "NotebookModel::favoriteNotebook: index: "
            << "is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setNotebookFavorited(index, true);
}

void NotebookModel::unfavoriteNotebook(const QModelIndex & index)
{
    QNDEBUG(
        "model::NotebookModel",
        "NotebookModel::unfavoriteNotebook: index: "
            << "is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setNotebookFavorited(index, false);
}

QString NotebookModel::localIdForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::localIdForItemName: name = "
            << itemName << ", linked notebook guid = " << linkedNotebookGuid);

    const auto index = indexForNotebookName(itemName, linkedNotebookGuid);
    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        QNTRACE(
            "model::NotebookModel",
            "No notebook model item found for index "
                << "found for this notebook name");
        return {};
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!notebookItem)) {
        QNTRACE(
            "model::NotebookModel", "Found notebook model item has wrong type");
        return {};
    }

    return notebookItem->localId();
}

QModelIndex NotebookModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (it == localIdIndex.end()) {
        QNTRACE(
            "model::NotebookModel",
            "Found no notebook model item for local id " << localId);
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString NotebookModel::itemNameForLocalId(const QString & localId) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::itemNameForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model::NotebookModel", "No notebook item with such local id");
        return {};
    }

    return it->name();
}

AbstractItemModel::ItemInfo NotebookModel::itemInfoForLocalId(
    const QString & localId) const
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::itemInfoForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model::NotebookModel", "No notebook item with such local id");
        return {};
    }

    AbstractItemModel::ItemInfo info;
    info.m_localId = it->localId();
    info.m_name = it->name();
    info.m_linkedNotebookGuid = it->linkedNotebookGuid();

    info.m_linkedNotebookUsername =
        linkedNotebookUsername(info.m_linkedNotebookGuid);

    return info;
}

QStringList NotebookModel::itemNames(const QString & linkedNotebookGuid) const
{
    QStringList result;
    const auto & nameIndex = m_data.get<ByNameUpper>();
    result.reserve(static_cast<int>(nameIndex.size()));
    for (auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it) {
        const auto & item = *it;
        const QString & name = item.name();

        if (linkedNotebookGuid.isNull()) {
            // Prevent the occurrence of identical names within the returned
            // result
            if (Q_LIKELY(it != nameIndex.begin())) {
                if (std::prev(it)->name() == name) {
                    continue;
                }
            }

            result << name;
        }
        else if (
            linkedNotebookGuid.isEmpty() && item.linkedNotebookGuid().isEmpty())
        {
            result << name;
        }
        else if (linkedNotebookGuid == item.linkedNotebookGuid()) {
            result << name;
        }
    }

    return result;
}

QList<AbstractItemModel::LinkedNotebookInfo>
NotebookModel::linkedNotebooksInfo() const
{
    QList<LinkedNotebookInfo> infos;
    infos.reserve(m_linkedNotebookItems.size());

    for (const auto it:
         qevercloud::toRange(std::as_const(m_linkedNotebookItems))) {
        infos.push_back(LinkedNotebookInfo{it.key(), it.value().username()});
    }

    return infos;
}

QString NotebookModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    if (const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
        it != m_linkedNotebookItems.end())
    {
        return it.value().username();
    }

    return {};
}

QModelIndex NotebookModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_allNotebooksRootItem)) {
        return {};
    }

    return indexForItem(m_allNotebooksRootItem);
}

QString NotebookModel::localIdForItemIndex(const QModelIndex & index) const
{
    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        return {};
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        return notebookItem->localId();
    }

    return {};
}

QString NotebookModel::linkedNotebookGuidForItemIndex(
    const QModelIndex & index) const
{
    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        return {};
    }

    auto * linkedNotebookItem = modelItem->cast<LinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        return linkedNotebookItem->linkedNotebookGuid();
    }

    return {};
}

void NotebookModel::start()
{
    QNDEBUG("model::NotebookModel", "NotebookModel::start");

    if (m_isStarted) {
        QNDEBUG("model::NotebookModel", "Already started");
        return;
    }

    m_isStarted = true;

    connectToLocalStorageEvents();
    requestNotebooksList();
    requestLinkedNotebooksList();
}

void NotebookModel::stop(const StopMode stopMode)
{
    QNDEBUG("model::NotebookModel", "NotebookModel::stop: " << stopMode);

    if (!m_isStarted) {
        QNDEBUG("model::NotebookModel", "Already stopped");
        return;
    }

    m_isStarted = false;
    disconnectFromLocalStorageEvents();
    clearModel();
}

Qt::ItemFlags NotebookModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((index.column() == static_cast<int>(Column::Dirty)) ||
        (index.column() == static_cast<int>(Column::FromLinkedNotebook)))
    {
        return indexFlags;
    }

    auto * modelItem = itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find notebook model item for the given index: row = "
                << index.row() << ", column = " << index.column());
        return indexFlags;
    }

    auto * stackItem = modelItem->cast<StackItem>();
    if (stackItem) {
        return flagsForStackItem(*stackItem, index.column(), indexFlags);
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        return flagsForNotebookItem(*notebookItem, index.column(), indexFlags);
    }

    return adjustFlagsForColumn(index.column(), indexFlags);
}

QVariant NotebookModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= gNotebookModelColumnCount)) {
        return {};
    }

    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        return {};
    }

    if (modelItem == m_invisibleRootItem) {
        return {};
    }

    Column column;
    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
        column = Column::Name;
        break;
    case Column::Synchronizable:
        column = Column::Synchronizable;
        break;
    case Column::Dirty:
        column = Column::Dirty;
        break;
    case Column::Default:
        column = Column::Default;
        break;
    case Column::Published:
        column = Column::Published;
        break;
    case Column::FromLinkedNotebook:
        column = Column::FromLinkedNotebook;
        break;
    case Column::NoteCount:
        column = Column::NoteCount;
        break;
    default:
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(*modelItem, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*modelItem, column);
    default:
        return {};
    }
}

QVariant NotebookModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation != Qt::Horizontal) {
        return {};
    }

    return columnName(static_cast<Column>(section));
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    auto * parentItem = itemForIndex(parent);
    return (parentItem ? parentItem->childrenCount() : 0);
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    return gNotebookModelColumnCount;
}

QModelIndex NotebookModel::index(
    const int row, const int column, const QModelIndex & parent) const
{
    if ((row < 0) || (column < 0) || (column >= gNotebookModelColumnCount) ||
        (parent.isValid() &&
         (parent.column() != static_cast<int>(Column::Name))))
    {
        return {};
    }

    auto * parentItem = itemForIndex(parent);
    if (!parentItem) {
        return {};
    }

    auto * modelItem = parentItem->childAtRow(row);
    if (!modelItem) {
        return {};
    }

    auto id = idForItem(*modelItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, column, id);
}

QModelIndex NotebookModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto * childItem = itemForIndex(index);
    if (!childItem) {
        return {};
    }

    auto * parentItem = childItem->parent();
    if (!parentItem) {
        return {};
    }

    if (parentItem == m_invisibleRootItem) {
        return {};
    }

    if (parentItem == m_allNotebooksRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allNotebooksRootItemIndexId);
    }

    auto * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        return {};
    }

    const int row = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Internal inconsistency detected in "
                << "NotebookModel: parent of the item can't find the item "
                << "within its children: item = " << *parentItem
                << "\nParent item: " << *grandParentItem);
        return {};
    }

    auto id = idForItem(*parentItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

bool NotebookModel::setHeaderData(
    [[maybe_unused]] int section, [[maybe_unused]] Qt::Orientation orientation,
    [[maybe_unused]] const QVariant & value, [[maybe_unused]] int role)
{
    return false;
}

bool NotebookModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::setData: row = "
            << modelIndex.row() << ", column = " << modelIndex.column()
            << ", internal id = " << modelIndex.internalId()
            << ", value = " << value << ", role = " << role);

    if (role != Qt::EditRole) {
        QNDEBUG("model::NotebookModel", "Role is not EditRole, skipping");
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG("model::NotebookModel", "Index is not valid");
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::Dirty)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"dirty\" flag is set automatically when "
                       "the notebook is changed"));
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::Published)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"published\" flag can't be set manually"));
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::FromLinkedNotebook)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"from linked notebook\" flag can't be set "
                       "manually"));
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::NoteCount)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"note count\" column can't be set "
                       "manually"));
        return false;
    }

    auto * modelItem = itemForIndex(modelIndex);
    if (!modelItem) {
        REPORT_ERROR(
            QT_TR_NOOP("No notebook model item was found for the given "
                       "model index"));
        return false;
    }

    if (Q_UNLIKELY(modelItem == m_invisibleRootItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the invisible root item "
                       "within the notebook model"));
        return false;
    }

    if (Q_UNLIKELY(
            modelItem->type() == INotebookModelItem::Type::LinkedNotebook)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the linked notebook root item"));
        return false;
    }

    if (Q_UNLIKELY(
            modelItem->type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        REPORT_ERROR(QT_TR_NOOP("Can't set data for all notebooks root item"));
        return false;
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        return setNotebookData(*notebookItem, modelIndex, value);
    }

    auto * stackItem = modelItem->cast<StackItem>();
    if (stackItem) {
        return setStackData(*stackItem, modelIndex, value);
    }

    QNWARNING(
        "model::NotebookModel",
        "Cannot set data to notebook model item, "
            << "failed to determine model item type");
    return false;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::insertRows: row = "
            << row << ", count = " << count
            << ", parent: is valid = " << (parent.isValid() ? "true" : "false")
            << ", row = " << parent.row() << ", column = " << parent.column());

    auto * parentItem =
        (parent.isValid() ? itemForIndex(parent) : m_invisibleRootItem);

    if (!parentItem) {
        QNDEBUG("model::NotebookModel", "No model item for given model index");
        return false;
    }

    auto * stackItem = parentItem->cast<StackItem>();
    if (Q_UNLIKELY(!stackItem)) {
        QNDEBUG(
            "model::NotebookModel",
            "Only the insertion of notebooks under "
                << "notebook stacks is supported");
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const int numExistingNotebooks = static_cast<int>(localIdIndex.size());
    if (Q_UNLIKELY(
            numExistingNotebooks + count >= m_account.notebookCountMax())) {
        ErrorString error{
            QT_TR_NOOP("Can't create a new notebook: the account can contain "
                       "a limited number of notebooks")};

        error.details() = QString::number(m_account.notebookCountMax());
        QNINFO("model::NotebookModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    std::vector<NotebookDataByLocalId::iterator> addedItems;
    addedItems.reserve(static_cast<std::size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Adding notebook item
        NotebookItem item;
        item.setLocalId(UidGenerator::Generate());

        m_notebookItemsNotYetInLocalStorageIds.insert(item.localId());

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(stackItem->name());
        item.setSynchronizable(m_account.type() != Account::Type::Local);
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);

        const auto insertionResult = localIdIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        auto * addedNotebookItem =
            const_cast<NotebookItem *>(&(*insertionResult.first));

        addedNotebookItem->setParent(parentItem);
    }
    endInsertRows();

    for (auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it)
    {
        const NotebookItem & item = *(*it);
        updateNotebookInLocalStorage(item);
    }

    if (m_sortedColumn == Column::Name) {
        Q_EMIT layoutAboutToBeChanged();
        for (auto & item: localIdIndex) {
            updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
        }
        Q_EMIT layoutChanged();
    }

    QNDEBUG("model::NotebookModel", "Successfully inserted the row(s)");
    return true;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::removeRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    RemoveRowsScopeGuard removeRowsScopeGuard(*this);
    Q_UNUSED(removeRowsScopeGuard)

    checkAndCreateModelRootItems();

    auto * parentItem =
        (parent.isValid() ? itemForIndex(parent) : m_invisibleRootItem);

    if (!parentItem) {
        QNDEBUG("model::NotebookModel", "No item corresponding to parent index");
        return false;
    }

    if (parentItem == m_invisibleRootItem) {
        QNDEBUG(
            "model::NotebookModel",
            "Can't remove direct child of invisible "
                << "root item (all notebooks root item)");
        return false;
    }

    if (Q_UNLIKELY(
            (parentItem != m_allNotebooksRootItem) &&
            (parentItem->type() != INotebookModelItem::Type::Stack)))
    {
        QNDEBUG(
            "model::NotebookModel",
            "Can't remove row(s) from parent item not "
                << "being a stack item or all notebooks root item: "
                << *parentItem);
        return false;
    }

    QStringList notebookLocalIdsToRemove;
    QList<std::pair<QString, QString>> stacksToRemoveWithLinkedNotebookGuids;

    // just a semi-random guess
    stacksToRemoveWithLinkedNotebookGuids.reserve(count / 2);

    QString linkedNotebookGuid;
    if ((parentItem->type() == INotebookModelItem::Type::LinkedNotebook)) {
        auto * linkedNotebookItem =
            parentItem->cast<LinkedNotebookRootItem>();

        if (linkedNotebookItem) {
            linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
        }
    }

    // First simply collect all the local ids and stacks of items to be removed
    // while checking for each of them whether they can be safely removed
    for (int i = 0; i < count; ++i) {
        auto * childItem = parentItem->childAtRow(row + i);
        if (!childItem) {
            QNWARNING(
                "model::NotebookModel",
                "Detected null pointer to child "
                    << "notebook model item on attempt to remove row "
                    << (row + i) << " from parent item: " << *parentItem);
            continue;
        }

        QNTRACE(
            "model::NotebookModel",
            "Removing item at "
                << childItem << ": " << *childItem << " at row " << (row + i)
                << " from parent item at " << parentItem << ": "
                << *parentItem);

        auto * notebookItem = childItem->cast<NotebookItem>();
        if (notebookItem) {
            if (!canRemoveNotebookItem(*notebookItem)) {
                return false;
            }

            notebookLocalIdsToRemove << notebookItem->localId();
            QNTRACE(
                "model::NotebookModel",
                "Marked notebook local id "
                    << notebookItem->localId()
                    << " as the one scheduled for removal");
        }

        auto * stackItem = childItem->cast<StackItem>();
        if (!stackItem) {
            continue;
        }

        QNTRACE(
            "model::NotebookModel",
            "Removing notebook stack: first remove all "
                << "its child notebooks and then itself");

        auto notebookModelItemsWithinStack = childItem->children();
        Q_ASSERT(
            notebookModelItemsWithinStack.size() <=
            std::numeric_limits<int>::max());
        for (int j = 0,
                 size = static_cast<int>(notebookModelItemsWithinStack.size());
             j < size; ++j)
        {
            auto * notebookModelItem = notebookModelItemsWithinStack[j];
            if (Q_UNLIKELY(!notebookModelItem)) {
                QNWARNING(
                    "model::NotebookModel",
                    "Detected null pointer to notebook model item within the "
                        << "children of the stack item being removed: "
                        << *childItem);
                continue;
            }

            auto * childNotebookItem =
                notebookModelItem->cast<NotebookItem>();

            if (Q_UNLIKELY(!childNotebookItem)) {
                QNWARNING(
                    "model::NotebookModel",
                    "Detected notebook model item "
                        << "within the stack item which is not "
                        << "a notebook by type; stack item: " << *childItem
                        << "\nIts child with "
                        << "wrong type: " << *notebookModelItem);
                continue;
            }

            QNTRACE(
                "model::NotebookModel",
                "Removing notebook item under stack at "
                    << childNotebookItem << ": " << *childNotebookItem);

            if (!canRemoveNotebookItem(*childNotebookItem)) {
                return false;
            }

            notebookLocalIdsToRemove << childNotebookItem->localId();

            QNTRACE(
                "model::NotebookModel",
                "Marked notebook local id "
                    << childNotebookItem->localId()
                    << " as the one scheduled for removal");
        }

        stacksToRemoveWithLinkedNotebookGuids.push_back(
            std::pair<QString, QString>(
                stackItem->name(), linkedNotebookGuid));

        QNTRACE(
            "model::NotebookModel",
            "Marked notebook stack " << stackItem->name()
                                     << " as the one scheduled for removal");
    }

    // Now we are sure all the items collected for the deletion can actually be
    // deleted

    auto & localIdIndex = m_data.get<ByLocalId>();

    Q_ASSERT(
        notebookLocalIdsToRemove.size() <= std::numeric_limits<int>::max());
    for (int i = 0, size = static_cast<int>(notebookLocalIdsToRemove.size());
         i < size; ++i)
    {
        const QString & localId = notebookLocalIdsToRemove[i];
        QNTRACE(
            "model::NotebookModel",
            "Processing notebook local id " << localId
                                             << " scheduled for removal");

        auto notebookItemIt = localIdIndex.find(localId);
        if (Q_UNLIKELY(notebookItemIt == localIdIndex.end())) {
            QNWARNING(
                "model::NotebookModel",
                "Internal error detected: can't find "
                    << "the notebook model item corresponding to the notebook "
                    << "item being removed: local id = " << localId);
            continue;
        }

        auto * modelItem = const_cast<NotebookItem *>(&(*notebookItemIt));
        QNTRACE(
            "model::NotebookModel",
            "Model item corresponding to the notebook: " << *modelItem);

        auto * parentItem = modelItem->parent();
        QNTRACE(
            "model::NotebookModel",
            "Notebook's parent item at "
                << parentItem << ": "
                << (parentItem ? parentItem->toString()
                                : QStringLiteral("<null>")));

        removeModelItemFromParent(*modelItem, RemoveEmptyParentStack::No);

        QNTRACE(
            "model::NotebookModel",
            "Model item corresponding to the notebook "
                << "after parent removal: " << *modelItem
                << "\nParent item after removing the child "
                << "notebook item from it: " << *parentItem);

        Q_UNUSED(localIdIndex.erase(notebookItemIt))

        QNTRACE(
            "model::NotebookModel",
            "Erased the notebook item corresponding to "
                << "local id " << localId);

        const auto indexIt = m_indexIdToLocalIdBimap.right.find(localId);
        if (indexIt != m_indexIdToLocalIdBimap.right.end()) {
            m_indexIdToLocalIdBimap.right.erase(indexIt);
        }

        expungeNotebookFromLocalStorage(localId);

        if (!parentItem) {
            continue;
        }

        auto * parentStackItem = parentItem->cast<StackItem>();
        if (!parentStackItem) {
            continue;
        }

        if (parentStackItem->childrenCount() != 0) {
            continue;
        }

        QNDEBUG(
            "model::NotebookModel",
            "The last child was removed from the stack item, need to remove it "
                << "as well: stack = " << parentStackItem->name());

        QString linkedNotebookGuid;
        auto * grandParentItem = parentItem->parent();
        if (grandParentItem) {
            auto * linkedNotebookItem =
                grandParentItem->cast<LinkedNotebookRootItem>();

            if (linkedNotebookItem) {
                linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
            }
        }

        stacksToRemoveWithLinkedNotebookGuids.push_back(
            std::pair<QString, QString>(
                parentStackItem->name(), linkedNotebookGuid));

        QNTRACE(
            "model::NotebookModel",
            "Marked notebook stack " << parentStackItem->name()
                                     << " as the one scheduled for removal");
    }

    QNTRACE(
        "model::NotebookModel",
        "Finished removing notebook items, switching "
            << "to removing notebook stack items (if any)");

    Q_ASSERT(
        stacksToRemoveWithLinkedNotebookGuids.size() <=
        std::numeric_limits<int>::max());
    for (int i = 0,
             size =
                 static_cast<int>(stacksToRemoveWithLinkedNotebookGuids.size());
         i < size; ++i)
    {
        const auto & pair = stacksToRemoveWithLinkedNotebookGuids[i];
        const QString & stack = pair.first;
        const QString & linkedNotebookGuid = pair.second;

        QNTRACE(
            "model::NotebookModel",
            "Processing notebook stack scheduled for removal: " << stack
                << ", linked notebook guid = " << linkedNotebookGuid);

        auto & stackItems =
            (linkedNotebookGuid.isEmpty()
                 ? m_stackItems
                 : m_stackItemsByLinkedNotebookGuid[linkedNotebookGuid]);

        const auto stackItemIt = stackItems.find(stack);
        if (Q_UNLIKELY(stackItemIt == stackItems.end())) {
            QNWARNING(
                "model::NotebookModel",
                "Internal error detected: can't find "
                    << "the notebook stack item being removed "
                    << "from the notebook model: stack = " << stack
                    << ", linked notebook guid = " << linkedNotebookGuid);
            continue;
        }

        stackItems.erase(stackItemIt);
        QNTRACE(
            "model::NotebookModel",
            "Erased the notebook stack item corresponding to stack " << stack
                << ", linked notebook guid = " << linkedNotebookGuid);
    }

    QNTRACE(
        "model::NotebookModel",
        "Successfully removed row(s) from the notebook model");
    return true;
}

void NotebookModel::sort(const int column, const Qt::SortOrder order)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (column != static_cast<int>(Column::Name)) {
        QNDEBUG(
            "model::NotebookModel",
            "Only sorting by name is implemented at this time");
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(
            "model::NotebookModel",
            "The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    auto & localIdIndex = m_data.get<ByLocalId>();
    for (auto & item: localIdIndex) {
        updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
    }

    for (const auto it: qevercloud::toRange(m_stackItems)) {
        updateItemRowWithRespectToSorting(const_cast<StackItem &>(it.value()));
    }

    for (const auto it: qevercloud::toRange(m_linkedNotebookItems)) {
        updateItemRowWithRespectToSorting(
            const_cast<LinkedNotebookRootItem &>(it.value()));
    }

    for (const auto it: qevercloud::toRange(m_stackItemsByLinkedNotebookGuid)) {
        auto & stackItems = it.value();
        for (const auto sit: qevercloud::toRange(stackItems)) {
            updateItemRowWithRespectToSorting(
                const_cast<StackItem &>(sit.value()));
        }
    }

    updatePersistentModelIndices();
    Q_EMIT layoutChanged();
}

QStringList NotebookModel::mimeTypes() const
{
    QStringList list;
    list << QString::fromUtf8(gMimeType.data(), gMimeType.size());
    return list;
}

QMimeData * NotebookModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    auto * item = itemForIndex(indexes.at(0));
    if (!item) {
        return nullptr;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *item;

    auto * mimeData = new QMimeData;
    mimeData->setData(
        QString::fromUtf8(gMimeType.data(), gMimeType.size()),
        qCompress(encodedItem, 9));

    return mimeData;
}

bool NotebookModel::dropMimeData(
    const QMimeData * mimeData, const Qt::DropAction action, const int row,
    const int column, const QModelIndex & parentIndex)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::dropMimeData: action = "
            << action << ", row = " << row << ", column = " << column
            << ", parent index: is valid = "
            << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row()
            << ", parent column = " << (parentIndex.column())
            << ", internal id = " << parentIndex.internalId()
            << ", mime data formats: "
            << (mimeData ? mimeData->formats().join(QStringLiteral("; "))
                         : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!mimeData ||
        !mimeData->hasFormat(
            QString::fromUtf8(gMimeType.data(), gMimeType.size())))
    {
        return false;
    }

    checkAndCreateModelRootItems();

    auto * newParentItem = itemForIndex(parentIndex);
    if (!newParentItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error, can't drop notebook: no new parent "
                       "item was found"));
        return false;
    }

    if (newParentItem == m_invisibleRootItem) {
        QNDEBUG(
            "model::NotebookModel",
            "Can't drop notebook model items directly onto invisible root "
            "item");
        return false;
    }

    if ((newParentItem != m_allNotebooksRootItem) &&
        (newParentItem->type() != INotebookModelItem::Type::Stack))
    {
        ErrorString error{
            QT_TR_NOOP("Can't drop the notebook onto another notebook")};
        QNINFO("model::NotebookModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    QByteArray data = qUncompress(
        mimeData->data(QString::fromUtf8(gMimeType.data(), gMimeType.size())));
    QDataStream in(&data, QIODevice::ReadOnly);

    qint32 type = 0;
    in >> type;

    if (type != static_cast<qint32>(INotebookModelItem::Type::Notebook)) {
        QNDEBUG(
            "model::NotebookModel",
            "Cannot drop items of unsupported types: " << type);
        return false;
    }

    NotebookItem notebookItem;
    in >> notebookItem;

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(notebookItem.localId());
    if (it == localIdIndex.end()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find the notebook being "
                       "dropped in the notebook model"));
        return false;
    }

    notebookItem = *it;

    QString parentLinkedNotebookGuid;

    auto * parentLinkedNotebookItem =
        newParentItem->cast<LinkedNotebookRootItem>();

    if (parentLinkedNotebookItem) {
        parentLinkedNotebookGuid =
            parentLinkedNotebookItem->linkedNotebookGuid();
    }
    else if (newParentItem->type() == INotebookModelItem::Type::Stack) {
        parentLinkedNotebookItem =
            newParentItem->parent()->cast<LinkedNotebookRootItem>();

        if (parentLinkedNotebookItem) {
            parentLinkedNotebookGuid =
                parentLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    if (notebookItem.linkedNotebookGuid() != parentLinkedNotebookGuid) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move notebooks between different linked "
                       "notebooks or between user's notebooks "
                       "and those from linked notebooks"));
        return false;
    }

    auto * originalParentItem = it->parent();
    int originalRow = -1;
    if (originalParentItem) {
        // Need to manually remove the item from its original parent
        originalRow = originalParentItem->rowForChild(&(*it));
    }

    if (originalRow >= 0) {
        const QModelIndex originalParentIndex =
            indexForItem(originalParentItem);

        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(originalParentItem->takeChild(originalRow))
        endRemoveRows();
    }

    localIdIndex.erase(it);

    beginInsertRows(parentIndex, row, row);

    auto * newParentStackItem = newParentItem->cast<StackItem>();

    notebookItem.setStack(
        (newParentItem == m_allNotebooksRootItem || !newParentStackItem)
            ? QString()
            : newParentStackItem->name());

    notebookItem.setDirty(true);

    const auto insertionResult = localIdIndex.insert(notebookItem);
    const auto notebookItemIt = insertionResult.first;

    auto * modelItem = const_cast<NotebookItem *>(&(*notebookItemIt));

    newParentItem->insertChild(row, modelItem);

    endInsertRows();

    updateItemRowWithRespectToSorting(*modelItem);
    updateNotebookInLocalStorage(notebookItem);
    return true;
}

void NotebookModel::connectToLocalStorageEvents()
{
    QNDEBUG(
        "model::NotebookModel", "NotebookModel::connectToLocalStorageEvents");

    if (m_connectedToLocalStorage) {
        QNDEBUG("model::NotebookModel", "Already connected to local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notebookPut,
        this,
        [this](const qevercloud::Notebook & notebook) {
            const auto status = onNotebookAddedOrUpdated(notebook);
            if (status == NotebookPutStatus::Added) {
                requestNoteCountForNotebook(notebook.localId());
            }
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        [this](const QString & localId) {
            QNDEBUG(
                "model::NotebookModel",
                "Notebook expunged: local id = " << localId);

            auto & localIdIndex = m_data.get<ByLocalId>();
            const auto itemIt = localIdIndex.find(localId);
            if (itemIt == localIdIndex.end()) {
                QNDEBUG(
                    "model::NotebookModel",
                    "Expunged notebook item was not present in the model");
                return;
            }

            Q_EMIT aboutToRemoveNotebooks();
            removeNotebookItemImpl(itemIt);
            Q_EMIT removedNotebooks();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notePut,
        this,
        [this](const qevercloud::Note & note) {
            if (note.deleted()) {
                return;
            }
            // The note could have been:
            // 1. Added - in which case the corresponding notebook's note count
            //    should increase
            // 2. Updated within the same notebook - in which case the
            //    corresponding notebook's note count should stay the same
            // 3. Updated with moving to another notebook - in which case note's
            //    new notebook's note count should be incremented and note's
            //    previous notebook's note count should be decremented.
            // We don't known which of the three cases occurred.
            // Due to the lack of information here the best we can do is to
            // request the actual fair note count for the note's current
            // notebook.
            requestNoteCountForNotebook(note.notebookLocalId());
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::noteExpunged,
        this,
        [this]([[maybe_unused]] const QString & noteLocalId) {
            // As we don't know which notebook the removed note belonged to,
            // the best we can do is to update note counts for all notebooks.
            // TODO: think about whether libquentier could provide the
            // information about which notebook the note was expunged from
            // without much overhead.
            requestNoteCountForAllNotebooks();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::linkedNotebookPut,
        this,
        [this](const qevercloud::LinkedNotebook & linkedNotebook) {
            onLinkedNotebookAddedOrUpdated(linkedNotebook);
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::linkedNotebookExpunged,
        this,
        [this](const QString & linkedNotebookGuid) {
            onLinkedNotebookExpunged(linkedNotebookGuid);
        });

    m_connectedToLocalStorage = true;
}

void NotebookModel::disconnectFromLocalStorageEvents()
{
    QNDEBUG(
        "model::NotebookModel",
        "NotebookModel::disconnectFromLocalStorageEvents");

    if (!m_connectedToLocalStorage) {
        QNDEBUG(
            "model::NotebookModel",
            "Already disconnected from local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();
    Q_ASSERT(notifier);
    notifier->disconnect(this);

    m_connectedToLocalStorage = false;
}

void NotebookModel::requestNotebooksList()
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::requestNotebooksList: offset = "
            << m_listNotebooksOffset);

    local_storage::ILocalStorage::ListNotebooksOptions options;
    options.m_limit = 40;
    options.m_offset = m_listNotebooksOffset;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    options.m_order =
        local_storage::ILocalStorage::ListNotebooksOrder::NoOrder;

    QNDEBUG(
        "model::NotebookModel",
        "Requesting a list of notebooks: offset = " << m_listNotebooksOffset);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto listNotebooksFuture = m_localStorage->listNotebooks(options);

    auto listNotebooksThenFuture = threading::then(
        std::move(listNotebooksFuture), this,
        [this, canceler](const QList<qevercloud::Notebook> & notebooks) {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "model::NotebookModel",
                "Received " << notebooks.size()
                            << " notebooks from local storage");

            for (const auto & notebook: std::as_const(notebooks)) {
                onNotebookAddedOrUpdated(notebook);
                // TODO: maybe it'd be better to request note counts for all
                // notebooks once they are all listed?
                requestNoteCountForNotebook(notebook.localId());
            }

            if (notebooks.isEmpty()) {
                QNDEBUG(
                    "model::NotebookModel",
                    "Received all notebooks from local storage");

                m_allNotebooksListed = true;

                if (m_allLinkedNotebooksListed) {
                    Q_EMIT notifyAllNotebooksListed();
                    Q_EMIT notifyAllItemsListed();
                }

                return;
            }

            m_listNotebooksOffset +=
                static_cast<quint64>(std::max<qint64>(notebooks.size(), 0));

            requestNotebooksList();
        });

    threading::onFailed(
        std::move(listNotebooksThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Failed to list notebooks")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::NotebookModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void NotebookModel::requestNoteCountForNotebook(const QString & notebookLocalId)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::requestNoteCountForNotebook: " << notebookLocalId);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto noteCountFuture = m_localStorage->noteCountPerNotebookLocalId(
        notebookLocalId,
        local_storage::ILocalStorage::NoteCountOption::IncludeNonDeletedNotes);

    auto noteCountThenFuture = threading::then(
        std::move(noteCountFuture), this,
        [this, notebookLocalId, canceler](const quint32 noteCount) {
            if (canceler->isCanceled()) {
                return;
            }

            auto & localIdIndex = m_data.get<ByLocalId>();
            const auto itemIt = localIdIndex.find(notebookLocalId);
            if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
                QNDEBUG(
                    "model::NotebookModel",
                    "Can't find notebook item by local id: "
                        << notebookLocalId);
                return;
            }

            NotebookItem item = *itemIt;
            item.setNoteCount(noteCount);
            updateNoteCountPerNotebookIndex(item, itemIt);
        });

    threading::onFailed(
        std::move(noteCountThenFuture), this,
        [this, notebookLocalId, canceler = std::move(canceler)](
            const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Can't get note count per notebook")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::NotebookModel", error << ": " << notebookLocalId);
            Q_EMIT notifyError(std::move(error));
        });
}

void NotebookModel::requestNoteCountForAllNotebooks()
{
    QNTRACE("model::NotebookModel", "NotebookModel::requestNoteCountForAllNotebooks");

    const auto & localIdIndex = m_data.get<ByLocalId>();
    for (const auto & item: localIdIndex) {
        requestNoteCountForNotebook(item.localId());
    }
}

void NotebookModel::requestLinkedNotebooksList()
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::requestLinkedNotebooksList: "
            << "offset = " << m_listLinkedNotebooksOffset);

    local_storage::ILocalStorage::ListLinkedNotebooksOptions options;
    options.m_limit = 40;
    options.m_offset = m_listLinkedNotebooksOffset;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    options.m_order =
        local_storage::ILocalStorage::ListLinkedNotebooksOrder::NoOrder;

    QNDEBUG(
        "model::NotebookModel",
        "Request a list of linked notebooks: offset = "
            << m_listLinkedNotebooksOffset);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto listLinkedNotebooksFuture =
        m_localStorage->listLinkedNotebooks(options);

    auto listLinkedNotebooksThenFuture = threading::then(
        std::move(listLinkedNotebooksFuture), this,
        [this, canceler](
            const QList<qevercloud::LinkedNotebook> & linkedNotebooks) {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "model::NotebookModel",
                "Received " << linkedNotebooks.size() << " linked notebooks "
                            << "from local storage");
            for (const auto & linkedNotebook: std::as_const(linkedNotebooks)) {
                onLinkedNotebookAddedOrUpdated(linkedNotebook);
            }

            if (linkedNotebooks.isEmpty()) {
                QNDEBUG(
                    "model::NotebookModel",
                    "Received all linked notebooks from local storage");

                m_allLinkedNotebooksListed = true;

                if (m_allNotebooksListed) {
                    Q_EMIT notifyAllNotebooksListed();
                    Q_EMIT notifyAllItemsListed();
                }

                return;
            }

            m_listLinkedNotebooksOffset += static_cast<quint64>(
                std::max<qint64>(linkedNotebooks.size(), 0));

            requestLinkedNotebooksList();
        });

    threading::onFailed(
        std::move(listLinkedNotebooksThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Failed to list linked notebooks")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::NotebookModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

QVariant NotebookModel::dataImpl(
    const INotebookModelItem & item, const Column column) const
{
    if (&item == m_allNotebooksRootItem) {
        if (column == Column::Name) {
            return tr("All notebooks");
        }

        return {};
    }

    const auto * notebookItem = item.cast<NotebookItem>();
    if (notebookItem) {
        return notebookData(*notebookItem, column);
    }

    const auto * stackItem = item.cast<StackItem>();
    if (stackItem) {
        return stackData(*stackItem, column);
    }

    return {};
}

QVariant NotebookModel::dataAccessibleText(
    const INotebookModelItem & item, const Column column) const
{
    if (&item == m_allNotebooksRootItem) {
        if (column == Column::Name) {
            return tr("All notebooks");
        }

        return {};
    }

    const auto * notebookItem = item.cast<NotebookItem>();
    if (notebookItem) {
        return notebookAccessibleData(*notebookItem, column);
    }

    const auto * stackItem = item.cast<StackItem>();
    if (stackItem) {
        return stackAccessibleData(*stackItem, column);
    }

    return {};
}

bool NotebookModel::canUpdateNotebookItem(const NotebookItem & item) const
{
    return item.isUpdatable();
}

void NotebookModel::updateNotebookInLocalStorage(const NotebookItem & item)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::updateNotebookInLocalStorage: local id = "
            << item.localId());

    qevercloud::Notebook notebook;

    auto notYetSavedItemIt =
        m_notebookItemsNotYetInLocalStorageIds.find(item.localId());

    if (notYetSavedItemIt == m_notebookItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG("model::NotebookModel", "Updating notebook");

        const auto * cachedNotebook = m_cache.get(item.localId());
        if (Q_UNLIKELY(!cachedNotebook)) {
            auto canceler = setupCanceler();
            Q_ASSERT(canceler);

            auto findNotebookFuture =
                m_localStorage->findNotebookByLocalId(item.localId());

            auto findNotebookThenFuture = threading::then(
                std::move(findNotebookFuture), this,
                [this, canceler, localId = item.localId()](
                    const std::optional<qevercloud::Notebook> & notebook) {
                    if (canceler->isCanceled()) {
                        return;
                    }

                    if (Q_UNLIKELY(!notebook)) {
                        ErrorString error{QT_TR_NOOP(
                            "Could not find notebook in local storage by "
                            "local id")};
                        error.details() = localId;
                        QNWARNING("model::NotebookModel", error);
                        Q_EMIT notifyError(std::move(error));
                        return;
                    }

                    m_cache.put(localId, *notebook);
                    auto & localIdIndex = m_data.get<ByLocalId>();
                    if (const auto it = localIdIndex.find(localId);
                        it != localIdIndex.end())
                    {
                        updateNotebookInLocalStorage(*it);
                    }
                });

            threading::onFailed(
                std::move(findNotebookThenFuture), this,
                [this, canceler = std::move(canceler), localId = item.localId()](
                    const QException & e) {
                    if (canceler->isCanceled()) {
                        return;
                    }

                    auto message = exceptionMessage(e);
                    QNWARNING(
                        "model::NotebookModel",
                        "Failed to find and update notebook in local storage; "
                            << "local id: " << localId << ", error: "
                            << message);
                    Q_EMIT notifyError(std::move(message));
                });

            return;
        }

        notebook = *cachedNotebook;
    }

    notebook.setLocalId(item.localId());
    notebook.setGuid(
        item.guid().isEmpty() ? std::nullopt : std::make_optional(item.guid()));

    notebook.setLinkedNotebookGuid(
        item.linkedNotebookGuid().isEmpty()
            ? std::nullopt
            : std::make_optional(item.linkedNotebookGuid()));

    notebook.setName(
        item.name().isEmpty() ? std::nullopt : std::make_optional(item.name()));

    notebook.setLocalOnly(!item.isSynchronizable());
    notebook.setLocallyModified(item.isDirty());
    notebook.setDefaultNotebook(item.isDefault());
    notebook.setPublished(item.isPublished());
    notebook.setLocallyFavorited(item.isFavorited());
    notebook.setStack(
        item.stack().isEmpty() ? std::nullopt
                               : std::make_optional(item.stack()));

    // If all item's properties related to the restrictions are "true", there's
    // no real need for the restrictions; otherwise need to set the restrictions
    if (!item.canCreateNotes() || !item.canUpdateNotes() ||
        !item.isUpdatable() || !item.nameIsUpdatable())
    {
        qevercloud::NotebookRestrictions restrictions;
        if (!item.canCreateNotes()) {
            restrictions.setNoCreateNotes(true);
        }

        if (!item.canUpdateNotes()) {
            restrictions.setNoUpdateNotes(true);
        }

        if (!item.isUpdatable()) {
            restrictions.setNoUpdateNotebook(true);
        }

        if (!item.nameIsUpdatable()) {
            restrictions.setNoRenameNotebook(true);
        }

        notebook.setRestrictions(std::move(restrictions));
    }

    // NOTE: deliberately not setting the updatable property from the item
    // as it can't be changed by the model and only serves the utilitary
    // purposes

    if (notYetSavedItemIt != m_notebookItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG(
            "model::NotebookModel",
            "Adding notebook to local storage: " << notebook);

        m_notebookItemsNotYetInLocalStorageIds.erase(notYetSavedItemIt);

        auto canceler = setupCanceler();
        Q_ASSERT(canceler);

        auto putNotebookFuture =
            m_localStorage->putNotebook(std::move(notebook));

        threading::onFailed(
            std::move(putNotebookFuture), this,
            [this, canceler = std::move(canceler), localId = item.localId()](
                const QException & e) {
                if (canceler->isCanceled()) {
                    return;
                }

                auto message = exceptionMessage(e);
                QNWARNING(
                    "model::NotebookModel",
                    "Failed to add notebook to local storage: " << message);
                Q_EMIT notifyError(std::move(message));
                removeNotebookItem(localId);
            });

        return;
    }

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    m_cache.remove(notebook.localId());

    QNDEBUG(
        "model::NotebookModel",
        "Updating notebook in local storage: " << notebook);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putNotebookFuture = m_localStorage->putNotebook(std::move(notebook));

    threading::onFailed(
        std::move(putNotebookFuture), this,
        [this, canceler = std::move(canceler), localId = item.localId()](
            const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            QNWARNING(
                "model::NotebookModel",
                "Failed to update notebook in local storage: " << message);
            Q_EMIT notifyError(std::move(message));

            // Try to restore the notebook to its actual version from
            // the local storage
            restoreNotebookItemFromLocalStorage(localId);
        });
}

void NotebookModel::expungeNotebookFromLocalStorage(const QString & localId)
{
    QNDEBUG(
        "model::NotebookModel",
        "NotebookModel::expungeNotebookFromLocalStorage: local id = "
            << localId);

    m_cache.remove(localId);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto expungeNotebookFuture =
        m_localStorage->expungeNotebookByLocalId(localId);

    threading::onFailed(
        std::move(expungeNotebookFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Failed to expunge notebook")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::NotebookModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

QString NotebookModel::nameForNewNotebook() const
{
    QString baseName = tr("New notebook");
    const auto & nameIndex = m_data.get<ByNameUpper>();

    return newItemName(
        nameIndex, m_lastNewNotebookNameCounter, std::move(baseName));
}

NotebookModel::RemoveRowsScopeGuard::RemoveRowsScopeGuard(
    NotebookModel & model) :
    m_model(model)
{
    m_model.beginRemoveNotebooks();
}

NotebookModel::RemoveRowsScopeGuard::~RemoveRowsScopeGuard()
{
    m_model.endRemoveNotebooks();
}

NotebookModel::NotebookPutStatus NotebookModel::onNotebookAddedOrUpdated(
    const qevercloud::Notebook & notebook)
{
    m_cache.put(notebook.localId(), notebook);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebook.localId());
    const bool newNotebook = (itemIt == localIdIndex.end());
    if (newNotebook) {
        Q_EMIT aboutToAddNotebook();

        onNotebookAdded(notebook);

        const QModelIndex addedNotebookIndex =
            indexForLocalId(notebook.localId());
        Q_EMIT addedNotebook(addedNotebookIndex);
        return NotebookPutStatus::Added;
    }

    const QModelIndex notebookIndexBefore = indexForLocalId(notebook.localId());
    Q_EMIT aboutToUpdateNotebook(notebookIndexBefore);

    onNotebookUpdated(notebook, itemIt);

    const QModelIndex notebookIndexAfter = indexForLocalId(notebook.localId());
    Q_EMIT updatedNotebook(notebookIndexAfter);

    return NotebookPutStatus::Updated;
}

void NotebookModel::onNotebookAdded(const qevercloud::Notebook & notebook)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::onNotebookAdded: notebook local id = "
            << notebook.localId());

    checkAndCreateModelRootItems();

    INotebookModelItem * parentItem = nullptr;
    if (notebook.stack()) {
        auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.linkedNotebookGuid() ? *notebook.linkedNotebookGuid()
                                          : QString{});

        auto * stackItems = stackItemsWithParentPair.first;
        auto * grandParentItem = stackItemsWithParentPair.second;
        const QString & stack = *notebook.stack();

        parentItem =
            &(findOrCreateStackItem(stack, *stackItems, grandParentItem));
    }
    else if (notebook.linkedNotebookGuid()) {
        parentItem = &(findOrCreateLinkedNotebookModelItem(
            *notebook.linkedNotebookGuid()));
    }
    else {
        parentItem = m_allNotebooksRootItem;
    }

    const auto parentIndex = indexForItem(parentItem);

    NotebookItem item;
    notebookToItem(notebook, item);

    const int row = parentItem->childrenCount();

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto insertionResult = localIdIndex.insert(item);
    auto * insertedItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    insertedItem->setParent(parentItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*insertedItem);
}

void NotebookModel::onNotebookUpdated(
    const qevercloud::Notebook & notebook, NotebookDataByLocalId::iterator it)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::onNotebookUpdated: notebook local id = "
            << notebook.localId());

    auto * modelItem = const_cast<NotebookItem *>(&(*it));
    auto * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the parent notebook model item for updated notebook "
                << "item: " << *modelItem);
        return;
    }

    int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the row of the child notebook model item within its "
                << "parent model item: parent item = " << *parentItem
                << "\nChild item = " << *modelItem);
        return;
    }

    bool shouldChangeParent = false;

    QString previousStackName;
    auto * parentStackItem = parentItem->cast<StackItem>();
    if (parentStackItem) {
        previousStackName = parentStackItem->name();
    }

    if (!notebook.stack() && !previousStackName.isEmpty()) {
        // Need to remove the notebook model item from the current stack and
        // set proper root item as the parent
        const QModelIndex parentIndex = indexForItem(parentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(parentItem->takeChild(row))
        endRemoveRows();

        if (!notebook.linkedNotebookGuid()) {
            checkAndCreateModelRootItems();
            parentItem = m_allNotebooksRootItem;
        }
        else {
            parentItem = &(findOrCreateLinkedNotebookModelItem(
                *notebook.linkedNotebookGuid()));
        }

        shouldChangeParent = true;
    }
    else if (notebook.stack() && *notebook.stack() != previousStackName) {
        // Need to remove the notebook from its current parent and insert it
        // under the stack item, either existing or new
        const QModelIndex parentIndex = indexForItem(parentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(parentItem->takeChild(row))
        endRemoveRows();

        auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.linkedNotebookGuid() ? *notebook.linkedNotebookGuid()
                                          : QString{});

        auto * stackItems = stackItemsWithParentPair.first;
        auto * grandParentItem = stackItemsWithParentPair.second;

        parentItem = &(findOrCreateStackItem(
            *notebook.stack(), *stackItems, grandParentItem));

        shouldChangeParent = true;
    }

    bool notebookStackChanged = false;
    if (shouldChangeParent) {
        const QModelIndex parentItemIndex = indexForItem(parentItem);
        const int newRow = parentItem->childrenCount();
        beginInsertRows(parentItemIndex, newRow, newRow);
        modelItem->setParent(parentItem);
        endInsertRows();

        row = parentItem->rowForChild(modelItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model::NotebookModel",
                "Can't find the row for the child notebook item within its "
                    << "parent right after setting the parent item to the "
                    << "child item! Parent item = "
                    << *parentItem << "\nChild item = " << *modelItem);
            return;
        }

        notebookStackChanged = true;
    }

    NotebookItem notebookItemCopy{*it};
    notebookToItem(notebook, notebookItemCopy);

    auto & localIdIndex = m_data.get<ByLocalId>();
    localIdIndex.replace(it, notebookItemCopy);

    const auto modelItemId = idForItem(*modelItem);
    const auto modelIndexFrom = createIndex(row, 0, modelItemId);

    QModelIndex modelIndexTo =
        createIndex(row, gNotebookModelColumnCount - 1, modelItemId);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model::NotebookModel", "Not sorting by name, returning");
        return;
    }

    updateItemRowWithRespectToSorting(*modelItem);

    if (notebookStackChanged) {
        if (!previousStackName.isEmpty() &&
            parentStackItem->childrenCount() == 0)
        {
            auto * grandParentItem = parentStackItem->parent();
            const auto row = grandParentItem->rowForChild(parentStackItem);
            if (row >= 0)
            {
                const auto grandParentItemIndex = indexForItem(grandParentItem);
                beginRemoveRows(grandParentItemIndex, row, row);
                Q_UNUSED(grandParentItem->takeChild(row));
                auto stackItems = stackItemsWithParent(
                    notebook.linkedNotebookGuid().value_or(QString{}));
                stackItems.first->remove(previousStackName);
                endRemoveRows();
            }
        }

        Q_EMIT notifyNotebookStackChanged(modelIndexFrom);
    }
}

void NotebookModel::onLinkedNotebookAddedOrUpdated(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "model::NotebookModel",
            "Can't process the addition or update of a linked notebook without "
                << "guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.username())) {
        QNWARNING(
            "model::NotebookModel",
            "Can't process the addition or update of a linked notebook without "
                << "username: " << linkedNotebook);
        return;
    }

    const auto & linkedNotebookGuid = *linkedNotebook.guid();
    const auto & newUsername = *linkedNotebook.username();

    m_linkedNotebookUsernamesByGuids[linkedNotebookGuid] = newUsername;

    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it != m_linkedNotebookItems.end()) {
        auto & linkedNotebookItem = it.value();

        if (linkedNotebookItem.username() == newUsername) {
            QNDEBUG(
                "model::NotebookModel",
                "The username hasn't changed, nothing to do");
            return;
        }

        linkedNotebookItem.setUsername(newUsername);
        QNDEBUG(
            "model::NotebookModel",
            "Updated the username corresponding to linked notebook guid "
                << linkedNotebookGuid << " to " << newUsername);

        const QModelIndex itemIndex = indexForItem(&linkedNotebookItem);
        Q_EMIT dataChanged(itemIndex, itemIndex);
        return;
    }

    QNDEBUG(
        "model::NotebookModel",
        "Adding new linked notebook item: username "
            << newUsername << " corresponding to linked notebook guid "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    it = m_linkedNotebookItems.insert(
        linkedNotebookGuid,
        LinkedNotebookRootItem{newUsername, linkedNotebookGuid});

    auto & linkedNotebookItem = it.value();

    const QModelIndex parentIndex = indexForItem(m_allNotebooksRootItem);
    const int row = rowForNewItem(*m_allNotebooksRootItem, linkedNotebookItem);

    beginInsertRows(parentIndex, row, row);
    m_allNotebooksRootItem->insertChild(row, &linkedNotebookItem);
    endInsertRows();
}

void NotebookModel::onLinkedNotebookExpunged(const QString & linkedNotebookGuid)
{
    QNDEBUG(
        "model::NotebookModel",
        "NotebookModel::onLinkedNotebookExpunged: linked notebook guid = "
            << linkedNotebookGuid);

    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    const auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);

    QStringList expungedNotebookLocalIds;
    expungedNotebookLocalIds.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedNotebookLocalIds << it->localId();
    }

    for (const auto & expungedNotebookLocalId:
         std::as_const(expungedNotebookLocalIds))
    {
        removeNotebookItem(expungedNotebookLocalId);
    }

    if (const auto linkedNotebookItemIt =
            m_linkedNotebookItems.find(linkedNotebookGuid);
        linkedNotebookItemIt != m_linkedNotebookItems.end())
    {
        auto * modelItem = &(linkedNotebookItemIt.value());
        auto * parentItem = modelItem->parent();
        if (parentItem) {
            const int row = parentItem->rowForChild(modelItem);
            if (row >= 0) {
                const QModelIndex parentItemIndex = indexForItem(parentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(parentItem->takeChild(row))
                endRemoveRows();
            }
        }

        m_linkedNotebookItems.erase(linkedNotebookItemIt);
    }

    if (const auto stackItemsIt =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
        stackItemsIt != m_stackItemsByLinkedNotebookGuid.end())
    {
        m_stackItemsByLinkedNotebookGuid.erase(stackItemsIt);
    }

    if (const auto indexIt =
            m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);
        indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end())
    {
        m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt);
    }
}

void NotebookModel::removeNotebookItemImpl(
    const NotebookDataByLocalId::iterator itemIt)
{
    auto & localIdIndex = m_data.get<ByLocalId>();
    Q_ASSERT(itemIt != localIdIndex.end());

    auto * modelItem = const_cast<NotebookItem *>(&(*itemIt));
    auto * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the parent notebook model item "
                << "for the notebook being removed from the model: local id = "
                << modelItem->localId());
        return;
    }

    const int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the notebook item's row within "
                << "its parent model item: notebook item = " << *modelItem
                << "\nStack item = " << *parentItem);
        return;
    }

    const QModelIndex parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();

    const QString localId = itemIt->localId();
    localIdIndex.erase(itemIt);
    m_cache.remove(localId);

    if (const auto indexIt = m_indexIdToLocalIdBimap.right.find(localId);
        indexIt != m_indexIdToLocalIdBimap.right.end()) {
        m_indexIdToLocalIdBimap.right.erase(indexIt);
    }

    checkAndRemoveEmptyStackItem(*parentItem);
}

bool NotebookModel::notebookItemMatchesByLinkedNotebook(
    const NotebookItem & item, const QString & linkedNotebookGuid) const
{
    if (linkedNotebookGuid.isNull()) {
        return true;
    }

    if (item.linkedNotebookGuid().isEmpty() != linkedNotebookGuid.isEmpty()) {
        return false;
    }

    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    if (item.linkedNotebookGuid() != linkedNotebookGuid) {
        return false;
    }

    return true;
}

Qt::ItemFlags NotebookModel::flagsForNotebookItem(
    const NotebookItem & notebookItem, const int column,
    Qt::ItemFlags flags) const
{
    flags |= Qt::ItemIsDragEnabled;

    if (column == static_cast<int>(Column::Synchronizable) &&
        notebookItem.isSynchronizable())
    {
        return flags;
    }

    if (column == static_cast<int>(Column::Name) &&
        !canUpdateNotebookItem(notebookItem))
    {
        return flags;
    }

    return adjustFlagsForColumn(column, flags);
}

bool NotebookModel::setStackData(
    StackItem & stackItem, const QModelIndex & modelIndex,
    const QVariant & value)
{
    if (modelIndex.column() != static_cast<int>(Column::Name)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't change any column other than name "
                       "for stack items"));
        return false;
    }

    const QString newStack = value.toString();
    const QString previousStack = stackItem.name();
    if (newStack == previousStack) {
        QNDEBUG(
            "model::NotebookModel",
            "Notebook stack hasn't changed, nothing to do");
        return true;
    }

    const auto children = stackItem.children();
    for (const auto * childItem: std::as_const(children)) {
        if (Q_UNLIKELY(!childItem)) {
            QNWARNING(
                "model::NotebookModel",
                "Detected null pointer within children of notebook stack item: "
                    << stackItem);
            continue;
        }

        auto * childNotebookItem = childItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!childNotebookItem)) {
            QNWARNING(
                "model::NotebookModel",
                "Detected non-notebook item within children of notebook stack "
                    << "item: " << stackItem);
            continue;
        }

        if (!canUpdateNotebookItem(*childNotebookItem)) {
            ErrorString error{
                QT_TR_NOOP("Can't update the notebook stack: "
                           "restrictions on at least one "
                           "of stacked notebooks update apply")};
            error.details() = childNotebookItem->name();

            QNINFO(
                "model::NotebookModel",
                error << ", notebook item for which "
                      << "the restrictions apply: " << *childNotebookItem);

            Q_EMIT notifyError(std::move(error));
            return false;
        }
    }

    auto * parentItem = stackItem.parent();
    if (Q_UNLIKELY(!parentItem)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't update notebook stack, can't "
                       "find the parent of the notebook stack item")};

        QNWARNING(
            "model::NotebookModel",
            error << ", previous stack: \"" << previousStack
                  << "\", new stack: \"" << newStack << "\"");

        Q_EMIT notifyError(std::move(error));
        return false;
    }

    QString linkedNotebookGuid;
    if ((parentItem != m_allNotebooksRootItem) &&
        (parentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        auto * linkedNotebookItem =
            parentItem->cast<LinkedNotebookRootItem>();

        if (linkedNotebookItem) {
            linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
        }
    }

    // Change the stack item
    StackItem newStackItem = stackItem;
    newStackItem.setName(newStack);

    // 1) Remove the previous stack item
    int stackItemRow = parentItem->rowForChild(&stackItem);
    if (Q_UNLIKELY(stackItemRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't update the notebook stack item: can't find "
                       "the row of the stack item to be updated within "
                       "the parent item"));
        return false;
    }

    // Remove children from the stack model item about to be removed;
    // set the parent to the stack's parent item for now
    const auto parentItemIndex = indexForItem(parentItem);
    for (auto it = children.constBegin(), end = children.constEnd(); it != end;
         ++it)
    {
        auto * childItem = *it;
        if (Q_UNLIKELY(!childItem)) {
            continue;
        }

        int row = stackItem.rowForChild(childItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model::NotebookModel",
                "Couldn't find row for the child "
                    << "notebook item of the stack item about to be removed; "
                       "stack "
                    << "item: " << stackItem
                    << "\nChild item: " << *childItem);
            continue;
        }

        beginRemoveRows(modelIndex, row, row);
        Q_UNUSED(stackItem.takeChild(row))
        endRemoveRows();

        auto * childNotebookItem = childItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!childNotebookItem)) {
            continue;
        }

        row = rowForNewItem(*parentItem, *childItem);
        beginInsertRows(parentItemIndex, row, row);
        parentItem->insertChild(row, childItem);
        endInsertRows();

        QNTRACE(
            "model::NotebookModel",
            "Temporarily inserted the child notebook "
                << "item from removed stack item at the stack's parent item at "
                   "row "
                << row << ", child item: " << *childItem);
    }

    // As we've just reparented some items to the stack's parent item, need
    // to figure out the stack item's row again
    stackItemRow = parentItem->rowForChild(&stackItem);
    if (Q_UNLIKELY(stackItemRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't update the notebook stack item, fatal "
                       "error: can't find the row of the stack item "
                       "to be updated within the stack's parent item "
                       "after temporarily moving its children "
                       "under the stack's parent item"));
        return false;
    }

    beginRemoveRows(parentItemIndex, stackItemRow, stackItemRow);
    Q_UNUSED(parentItem->takeChild(stackItemRow))

    auto stackItemIt = m_stackItems.find(previousStack);
    if (stackItemIt != m_stackItems.end()) {
        Q_UNUSED(m_stackItems.erase(stackItemIt))
    }

    auto indexIt = m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(
        std::make_pair(previousStack, linkedNotebookGuid));

    if (indexIt != m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
        m_indexIdToStackAndLinkedNotebookGuidBimap.right.erase(indexIt);
    }

    endRemoveRows();

    // 2) Insert the new stack item

    stackItemIt = m_stackItems.insert(newStack, newStackItem);
    stackItemRow = rowForNewItem(*parentItem, *stackItemIt);

    const QModelIndex stackItemIndex =
        index(stackItemRow, static_cast<int>(Column::Name), parentItemIndex);

    beginInsertRows(parentItemIndex, stackItemRow, stackItemRow);
    parentItem->insertChild(stackItemRow, &(*stackItemIt));
    endInsertRows();

    // 3) Move all children of the previous stack items to the new one
    auto & localIdIndex = m_data.get<ByLocalId>();
    for (auto * childItem: std::as_const(children)) {
        if (Q_UNLIKELY(!childItem)) {
            continue;
        }

        auto * childNotebookItem = childItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!childNotebookItem)) {
            continue;
        }

        const auto notebookItemIt =
            localIdIndex.find(childNotebookItem->localId());
        if (notebookItemIt == localIdIndex.end()) {
            QNWARNING(
                "model::NotebookModel",
                "Internal inconsistency detected: can't find iterator for the "
                    << "notebook item for which the stack it being changed: "
                    << "non-found notebook item: " << *childNotebookItem);
            continue;
        }

        int row = parentItem->rowForChild(childItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model::NotebookModel",
                "Internal error: can't find the row of one of the removed "
                    << "stack item's children within the stack's parent item "
                    << "to which they were temporarily moved; stack's parent "
                    << "item: " << *parentItem << "\nChild item: "
                    << *childItem);
            continue;
        }

        beginRemoveRows(parentItemIndex, row, row);
        Q_UNUSED(parentItem->takeChild(row))
        endRemoveRows();

        row = rowForNewItem(*stackItemIt, *childItem);
        beginInsertRows(stackItemIndex, row, row);
        stackItemIt->insertChild(row, childItem);
        endInsertRows();

        // Also update the stack within the notebook item
        NotebookItem notebookItemCopy{*childNotebookItem};
        notebookItemCopy.setStack(newStack);

        const bool wasDirty = notebookItemCopy.isDirty();
        notebookItemCopy.setDirty(true);

        localIdIndex.replace(notebookItemIt, notebookItemCopy);

        QModelIndex notebookItemIndex =
            indexForLocalId(childNotebookItem->localId());

        QNTRACE("model::NotebookModel", "Emitting the data changed signal");
        Q_EMIT dataChanged(notebookItemIndex, notebookItemIndex);

        if (!wasDirty) {
            // Also emit dataChanged for dirty column
            notebookItemIndex = index(
                notebookItemIndex.row(), static_cast<int>(Column::Dirty),
                notebookItemIndex.parent());

            Q_EMIT dataChanged(notebookItemIndex, notebookItemIndex);
        }

        updateNotebookInLocalStorage(notebookItemCopy);
    }

    Q_EMIT notifyNotebookStackRenamed(
        previousStack, newStack, linkedNotebookGuid);

    return true;
}

QVariant NotebookModel::stackData(
    const StackItem & stackItem, const Column column) const
{
    switch (column) {
    case Column::Name:
        return stackItem.name();
    default:
        return {};
    }
}

QVariant NotebookModel::stackAccessibleData(
    const StackItem & stackItem, const Column column) const
{
    QString accessibleText = tr("Notebook stack") + QStringLiteral(": ");

    const QVariant textData = stackData(stackItem, column);
    if (textData.isNull()) {
        return {};
    }

    switch (column) {
    case Column::Name:
        accessibleText +=
            tr("name is") + QStringLiteral(" ") + textData.toString();
        break;
    default:
        return {};
    }

    return QVariant{std::move(accessibleText)};
}

const NotebookModel::StackItems * NotebookModel::findStackItems(
    const QString & linkedNotebookGuid) const
{
    if (linkedNotebookGuid.isEmpty()) {
        return &m_stackItems;
    }

    if (const auto it =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
        it != m_stackItemsByLinkedNotebookGuid.end())
    {
        return &(it.value());
    }

    return nullptr;
}

std::pair<NotebookModel::StackItems *, INotebookModelItem *>
NotebookModel::stackItemsWithParent(const QString & linkedNotebookGuid)
{
    if (linkedNotebookGuid.isEmpty()) {
        checkAndCreateModelRootItems();
        return std::make_pair(&m_stackItems, m_allNotebooksRootItem);
    }

    auto * linkedNotebookRootItem =
        &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));

    auto stackItemsIt =
        m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (Q_UNLIKELY(stackItemsIt == m_stackItemsByLinkedNotebookGuid.end())) {
        stackItemsIt = m_stackItemsByLinkedNotebookGuid.insert(
            linkedNotebookGuid, StackItems());
    }

    return std::pair{&(*stackItemsIt), linkedNotebookRootItem};
}

StackItem & NotebookModel::findOrCreateStackItem(
    const QString & stack, StackItems & stackItems,
    INotebookModelItem * parentItem)
{
    auto it = stackItems.find(stack);
    if (it == stackItems.end()) {
        it = stackItems.insert(stack, StackItem{stack});
        auto & stackItem = it.value();

        const int row = rowForNewItem(*parentItem, stackItem);
        const QModelIndex parentIndex = indexForItem(parentItem);

        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, &stackItem);
        endInsertRows();
    }

    return it.value();
}

Qt::ItemFlags NotebookModel::flagsForStackItem(
    const StackItem & stackItem, const int column, Qt::ItemFlags flags) const
{
    flags |= Qt::ItemIsDropEnabled;

    // Check whether all the notebooks in the stack are eligible for editing
    // of the column in question
    auto children = stackItem.children();
    for (auto * childItem : std::as_const(children)) {
        if (Q_UNLIKELY(!childItem)) {
            QNWARNING(
                "model::NotebookModel",
                "Detected null pointer to notebook model item within the "
                    << "children of another notebook model item");
            continue;
        }

        auto * childNotebookItem = childItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!childNotebookItem)) {
            QNWARNING(
                "model::NotebookModel",
                "Detected nested notebook model item of non-notebook type ("
                    << childItem->type()
                    << "); it is unexpected and most likely incorrect");
            continue;
        }

        if (column == static_cast<int>(Column::Synchronizable) &&
            childNotebookItem->isSynchronizable())
        {
            return flags;
        }

        if (column == static_cast<int>(Column::Name) &&
            !canUpdateNotebookItem(*childNotebookItem))
        {
            return flags;
        }
    }

    if (column == static_cast<int>(Column::Name)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

void NotebookModel::removeNotebookItem(const QString & localId)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::removeNotebookItem: local id = " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model::NotebookModel",
            "Can't find item to remove from the notebook model");
        return;
    }

    removeNotebookItemImpl(itemIt);
}

void NotebookModel::restoreNotebookItemFromLocalStorage(const QString & localId)
{
    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto findNotebookFuture =
        m_localStorage->findNotebookByLocalId(localId);

    auto findNotebookThenFuture = threading::then(
        std::move(findNotebookFuture), this,
        [this, localId, canceler](
            const std::optional<qevercloud::Notebook> & notebook) {
            if (canceler->isCanceled()) {
                return;
            }

            if (Q_UNLIKELY(!notebook)) {
                QNWARNING(
                    "model::NotebookModel",
                    "Could not find notebook by local id in local storage");
                removeNotebookItem(localId);
                return;
            }

            onNotebookAddedOrUpdated(*notebook);
        });

    threading::onFailed(
        std::move(findNotebookThenFuture), this,
        [this, localId, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            QNWARNING(
                "model::NotebookModel",
                "Failed to restore notebook from local storage: "
                    << message);
            Q_EMIT notifyError(std::move(message));
        });
}

void NotebookModel::notebookToItem(
    const qevercloud::Notebook & notebook, NotebookItem & item)
{
    item.setLocalId(notebook.localId());

    if (notebook.guid()) {
        item.setGuid(*notebook.guid());
    }

    if (notebook.stack()) {
        item.setStack(*notebook.stack());
    }

    if (notebook.name()) {
        item.setName(*notebook.name());
    }

    item.setSynchronizable(!notebook.isLocalOnly());
    item.setDirty(notebook.isLocallyModified());
    item.setFavorited(notebook.isLocallyFavorited());

    const bool isDefaultNotebook = notebook.defaultNotebook().value_or(false);
    item.setDefault(isDefaultNotebook);
    if (isDefaultNotebook) {
        switchDefaultNotebookLocalId(notebook.localId());
    }

    item.setPublished(notebook.published().value_or(false));

    if (notebook.restrictions()) {
        const auto & restrictions = *notebook.restrictions();

        item.setUpdatable(
            !restrictions.noUpdateNotebook().value_or(false));

        item.setNameIsUpdatable(
            !restrictions.noRenameNotebook().value_or(false));

        item.setCanCreateNotes(
            !restrictions.noCreateNotes().value_or(false));

        item.setCanUpdateNotes(
            !restrictions.noUpdateNotes().value_or(false));
    }
    else {
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);
        item.setCanCreateNotes(true);
        item.setCanUpdateNotes(true);
    }

    if (notebook.linkedNotebookGuid()) {
        item.setLinkedNotebookGuid(*notebook.linkedNotebookGuid());
    }
}

void NotebookModel::removeModelItemFromParent(
    INotebookModelItem & modelItem,
    RemoveEmptyParentStack removeEmptyParentStack)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::removeModelItemFromParent: "
            << modelItem
            << "\nRemove empty parent stack = " << removeEmptyParentStack);

    auto * parentItem = modelItem.parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNDEBUG("model::NotebookModel", "No parent item, nothing to do");
        return;
    }

    QNTRACE("model::NotebookModel", "Parent item: " << *parentItem);
    const int row = parentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the child notebook model "
                << "item's row within its parent; child item = " << modelItem
                << ", parent item = " << *parentItem);
        return;
    }

    QNTRACE("model::NotebookModel", "Will remove the child at row " << row);

    const QModelIndex parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();

    if (removeEmptyParentStack == RemoveEmptyParentStack::Yes) {
        checkAndRemoveEmptyStackItem(*parentItem);
    }
}

int NotebookModel::rowForNewItem(
    const INotebookModelItem & parentItem,
    const INotebookModelItem & newItem) const
{
    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return parentItem.childrenCount();
    }

    QList<INotebookModelItem *> children = parentItem.children();
    auto it = children.end();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(
            children.begin(), children.end(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(
            children.begin(), children.end(), &newItem, GreaterByName());
    }

    if (it == children.end()) {
        return parentItem.childrenCount();
    }

    return static_cast<int>(std::distance(children.begin(), it));
}

void NotebookModel::updateItemRowWithRespectToSorting(
    INotebookModelItem & modelItem)
{
    QNTRACE(
        "model::NotebookModel", "NotebookModel::updateItemRowWithRespectToSorting");

    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    auto * parentItem = modelItem.parent();
    if (Q_UNLIKELY(!parentItem)) {
        QString linkedNotebookGuid;
        const auto * notebookItem = modelItem.cast<NotebookItem>();
        if (notebookItem) {
            linkedNotebookGuid = notebookItem->linkedNotebookGuid();
        }

        if (!linkedNotebookGuid.isEmpty()) {
            parentItem =
                &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
        }
        else {
            checkAndCreateModelRootItems();
            parentItem = m_allNotebooksRootItem;
        }

        const int row = rowForNewItem(*parentItem, modelItem);

        beginInsertRows(indexForItem(parentItem), row, row);
        parentItem->insertChild(row, &modelItem);
        endInsertRows();
        return;
    }

    const int currentItemRow = parentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't update notebook model item's row: "
                << "can't find its original row within parent: " << modelItem);
        return;
    }

    const QModelIndex parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(parentItem->takeChild(currentItemRow))
    endRemoveRows();

    const int appropriateRow = rowForNewItem(*parentItem, modelItem);

    beginInsertRows(parentIndex, appropriateRow, appropriateRow);
    parentItem->insertChild(appropriateRow, &modelItem);
    endInsertRows();

    QNTRACE(
        "model::NotebookModel",
        "Moved item from row " << currentItemRow << " to row " << appropriateRow
                               << "; item: " << modelItem);
}

void NotebookModel::updatePersistentModelIndices()
{
    QNTRACE("model::NotebookModel", "NotebookModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    const auto indices = persistentIndexList();
    for (const auto & index: std::as_const(indices)) {
        auto * item = itemForId(static_cast<IndexId>(index.internalId()));
        auto replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
}

void NotebookModel::switchDefaultNotebookLocalId(const QString & localId)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::switchDefaultNotebookLocalId: " << localId);

    if (!m_defaultNotebookLocalId.isEmpty()) {
        QNTRACE(
            "model::NotebookModel",
            "There has already been some notebook chosen "
                << "as the default, switching the default flag off for it");

        auto & localIdIndex = m_data.get<ByLocalId>();

        const auto previousDefaultItemIt =
            localIdIndex.find(m_defaultNotebookLocalId);

        if (previousDefaultItemIt != localIdIndex.end()) {
            NotebookItem previousDefaultItemCopy{*previousDefaultItemIt};
            QNTRACE(
                "model::NotebookModel",
                "Previous default notebook item: " << previousDefaultItemCopy);

            previousDefaultItemCopy.setDefault(false);
            const bool wasDirty = previousDefaultItemCopy.isDirty();
            previousDefaultItemCopy.setDirty(true);

            localIdIndex.replace(
                previousDefaultItemIt, previousDefaultItemCopy);

            auto previousDefaultItemIndex =
                indexForLocalId(m_defaultNotebookLocalId);

            Q_EMIT dataChanged(
                previousDefaultItemIndex, previousDefaultItemIndex);

            if (!wasDirty) {
                previousDefaultItemIndex = index(
                    previousDefaultItemIndex.row(),
                    static_cast<int>(Column::Dirty),
                    previousDefaultItemIndex.parent());

                Q_EMIT dataChanged(
                    previousDefaultItemIndex, previousDefaultItemIndex);
            }

            updateNotebookInLocalStorage(previousDefaultItemCopy);
        }
    }

    m_defaultNotebookLocalId = localId;

    QNTRACE(
        "model::NotebookModel", "Set default notebook local id to " << localId);
}

void NotebookModel::checkAndRemoveEmptyStackItem(INotebookModelItem & modelItem)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::checkAndRemoveEmptyStackItem: " << modelItem);

    if (&modelItem == m_invisibleRootItem) {
        QNDEBUG("model::NotebookModel", "Won't remove the invisible root item");
        return;
    }

    if (&modelItem == m_allNotebooksRootItem) {
        QNDEBUG("model::NotebookModel", "Won't remove the all notebooks root item");
        return;
    }

    auto * stackItem = modelItem.cast<StackItem>();
    if (!stackItem) {
        QNDEBUG(
            "model::NotebookModel",
            "The model item is not of stack type, won't remove it");
        return;
    }

    if (modelItem.childrenCount() != 0) {
        QNDEBUG("model::NotebookModel", "The model item still has child items");
        return;
    }

    QNDEBUG(
        "model::NotebookModel",
        "Removed the last child of the previous "
            << "notebook's stack, need to remove the stack item itself");

    const QString previousStack = stackItem->name();

    QString linkedNotebookGuid;
    auto * parentItem = modelItem.parent();
    if (parentItem &&
        (parentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        auto * linkedNotebookItem =
            parentItem->cast<LinkedNotebookRootItem>();

        if (linkedNotebookItem) {
            linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
        }
    }

    removeModelItemFromParent(modelItem, RemoveEmptyParentStack::No);

    StackItems * stackItems = nullptr;

    if (linkedNotebookGuid.isEmpty()) {
        stackItems = &m_stackItems;
    }
    else {
        auto stackItemsIt =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

        if (Q_UNLIKELY(stackItemsIt == m_stackItemsByLinkedNotebookGuid.end()))
        {
            QNWARNING(
                "model::NotebookModel",
                "Can't properly remove the notebook "
                    << "stack item without children: can't find the map of "
                       "stack "
                    << "items for linked notebook guid " << linkedNotebookGuid);
            return;
        }

        stackItems = &(stackItemsIt.value());
    }

    if (const auto it = stackItems->find(previousStack);
        it != stackItems->end()) {
        stackItems->erase(it);
    }
    else {
        QNWARNING(
            "model::NotebookModel",
            "Can't find stack model item to remove, stack " << previousStack);
    }
}

void NotebookModel::setNotebookFavorited(
    const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the model index is invalid"));
        return;
    }

    auto * modelItem = itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "failed to find the model item "
                       "corresponding to the model index"));
        return;
    }

    auto * notebookItem = modelItem->cast<NotebookItem>();
    if (!notebookItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the item attempted to be favorited "
                       "or unfavorited is not a notebook"));
        return;
    }

    if (favorited == notebookItem->isFavorited()) {
        QNDEBUG("model::NotebookModel", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(notebookItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the notebook item was not found within the model"));
        return;
    }

    NotebookItem itemCopy{*notebookItem};
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the notebook as dirty as favorited property is
    // not included into the synchronization protocol

    localIdIndex.replace(it, itemCopy);
    updateNotebookInLocalStorage(itemCopy);
}

void NotebookModel::beginRemoveNotebooks()
{
    Q_EMIT aboutToRemoveNotebooks();
}

void NotebookModel::endRemoveNotebooks()
{
    Q_EMIT removedNotebooks();
}

INotebookModelItem & NotebookModel::findOrCreateLinkedNotebookModelItem(
    const QString & linkedNotebookGuid)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::findOrCreateLinkedNotebookModelItem: "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    if (Q_UNLIKELY(linkedNotebookGuid.isEmpty())) {
        QNWARNING(
            "model::NotebookModel",
            "Detected the request for finding or "
                << "creation of a linked notebook model item for empty "
                << "linked notebook guid");
        return *m_allNotebooksRootItem;
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model::NotebookModel",
            "Found existing linked notebook model item "
                << "for linked notebook guid " << linkedNotebookGuid);
        return linkedNotebookItemIt.value();
    }

    QNDEBUG(
        "model::NotebookModel",
        "Found no existing linked notebook item "
            << "corresponding to linked notebook guid " << linkedNotebookGuid);

    auto linkedNotebookOwnerUsernameIt =
        m_linkedNotebookUsernamesByGuids.find(linkedNotebookGuid);

    if (Q_UNLIKELY(
            linkedNotebookOwnerUsernameIt ==
            m_linkedNotebookUsernamesByGuids.end()))
    {
        QNDEBUG(
            "model::NotebookModel",
            "Found no linked notebook owner's username "
                << "for linked notebook guid " << linkedNotebookGuid);

        linkedNotebookOwnerUsernameIt = m_linkedNotebookUsernamesByGuids.insert(
            linkedNotebookGuid, QString{});
    }

    const QString & linkedNotebookOwnerUsername =
        linkedNotebookOwnerUsernameIt.value();

    LinkedNotebookRootItem item{
        linkedNotebookOwnerUsername, linkedNotebookGuid};

    linkedNotebookItemIt =
        m_linkedNotebookItems.insert(linkedNotebookGuid, item);

    auto * linkedNotebookItem = &(linkedNotebookItemIt.value());

    QNTRACE(
        "model::NotebookModel",
        "Linked notebook root item: " << *linkedNotebookItem);

    const int row = rowForNewItem(*m_allNotebooksRootItem, *linkedNotebookItem);
    beginInsertRows(indexForItem(m_allNotebooksRootItem), row, row);
    m_allNotebooksRootItem->insertChild(row, linkedNotebookItem);
    endInsertRows();

    return *linkedNotebookItem;
}

// WARNING: this method must NOT be called between beginSmth/endSmth
// i.e. beginInsertRows and endInsertRows
void NotebookModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_invisibleRootItem)) {
        m_invisibleRootItem = new InvisibleNotebookRootItem;
        QNDEBUG("model::NotebookModel", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_allNotebooksRootItem)) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_allNotebooksRootItem = new AllNotebooksRootItem;
        m_allNotebooksRootItem->setParent(m_invisibleRootItem);
        endInsertRows();
        QNDEBUG("model::NotebookModel", "Created all notebooks root item");
    }
}

INotebookModelItem * NotebookModel::itemForId(const IndexId id) const
{
    QNTRACE("model::NotebookModel", "NotebookModel::itemForId: " << id);

    if (id == m_allNotebooksRootItemIndexId) {
        return m_allNotebooksRootItem;
    }

    const auto localIdIt = m_indexIdToLocalIdBimap.left.find(id);
    if (localIdIt != m_indexIdToLocalIdBimap.left.end()) {
        const QString & localId = localIdIt->second;
        QNTRACE(
            "model::NotebookModel",
            "Found notebook local id corresponding to "
                << "model index internal id: " << localId);

        const auto & localIdIndex = m_data.get<ByLocalId>();
        if (const auto itemIt = localIdIndex.find(localId);
            itemIt != localIdIndex.end())
        {
            QNTRACE(
                "model::NotebookModel",
                "Found notebook model item corresponding to local id: "
                    << *itemIt);
            return const_cast<NotebookItem *>(&(*itemIt));
        }

        QNTRACE(
            "model::NotebookModel",
            "Found no notebook model item corresponding to local id");

        return nullptr;
    }

    const auto stackIt =
        m_indexIdToStackAndLinkedNotebookGuidBimap.left.find(id);
    if (stackIt != m_indexIdToStackAndLinkedNotebookGuidBimap.left.end()) {
        const QString & stack = stackIt->second.first;
        const QString & linkedNotebookGuid = stackIt->second.second;
        QNTRACE(
            "model::NotebookModel",
            "Found notebook stack corresponding to model index internal id: "
                << stack << ", linked notebook guid = " << linkedNotebookGuid);

        const auto * stackItems = findStackItems(linkedNotebookGuid);
        if (!stackItems) {
            QNWARNING(
                "model::NotebookModel",
                "Found no stack items corresponding to linked notebook guid "
                    << linkedNotebookGuid);
            return nullptr;
        }

        if (auto itemIt = stackItems->find(stack); itemIt != stackItems->end())
        {
            QNTRACE(
                "model::NotebookModel",
                "Found notebook model item corresponding to stack: "
                    << itemIt.value());
            return const_cast<StackItem *>(&(*itemIt));
        }
    }

    const auto linkedNotebookIt =
        m_indexIdToLinkedNotebookGuidBimap.left.find(id);
    if (linkedNotebookIt != m_indexIdToLinkedNotebookGuidBimap.left.end()) {
        const QString & linkedNotebookGuid = linkedNotebookIt->second;
        QNTRACE(
            "model::NotebookModel",
            "Found linked notebook guid corresponding to "
                << "model index internal id: " << linkedNotebookGuid);

        if (auto itemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
            itemIt != m_linkedNotebookItems.end())
        {
            QNTRACE(
                "model::NotebookModel",
                "Found notebook model item corresponding "
                    << "to linked notebook guid: " << itemIt.value());
            return const_cast<LinkedNotebookRootItem *>(&itemIt.value());
        }

        QNTRACE(
            "model::NotebookModel",
            "Found no notebook model item corresponding to linked notebook "
                << "guid");
        return nullptr;
    }

    QNDEBUG(
        "model::NotebookModel",
        "Found no notebook model items corresponding to model index id");
    return nullptr;
}

NotebookModel::IndexId NotebookModel::idForItem(
    const INotebookModelItem & item) const
{
    const auto * notebookItem = item.cast<NotebookItem>();
    if (notebookItem) {
        const auto it =
            m_indexIdToLocalIdBimap.right.find(notebookItem->localId());

        if (it == m_indexIdToLocalIdBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;

            m_indexIdToLocalIdBimap.insert(
                IndexIdToLocalIdBimap::value_type{
                    id, notebookItem->localId()});

            return id;
        }

        return it->second;
    }

    const auto * stackItem = item.cast<StackItem>();
    if (stackItem) {
        QString linkedNotebookGuid;
        auto * parentItem = item.parent();

        if (parentItem &&
            parentItem->type() == INotebookModelItem::Type::LinkedNotebook)
        {
            auto * linkedNotebookItem =
                parentItem->cast<LinkedNotebookRootItem>();
            if (linkedNotebookItem) {
                linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
            }
        }

        std::pair pair{stackItem->name(), linkedNotebookGuid};
        const auto it =
            m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(pair);
        if (it == m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToStackAndLinkedNotebookGuidBimap.insert(
                IndexIdToStackAndLinkedNotebookGuidBimap::value_type(id, pair)))
            return id;
        }

        return it->second;
    }

    const auto * linkedNotebookItem = item.cast<LinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        const auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
            linkedNotebookItem->linkedNotebookGuid());

        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;

            Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.insert(
                IndexIdToLinkedNotebookGuidBimap::value_type(
                    id, linkedNotebookItem->linkedNotebookGuid())))

            return id;
        }

        return it->second;
    }

    if (&item == m_allNotebooksRootItem) {
        return m_allNotebooksRootItemIndexId;
    }

    QNWARNING(
        "model::NotebookModel",
        "Detected attempt to assign id to unidentified notebook model item: "
            << item);
    return 0;
}

Qt::ItemFlags NotebookModel::adjustFlagsForColumn(
    const int column, Qt::ItemFlags flags) const
{
    if (column == static_cast<int>(Column::Name)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool NotebookModel::setNotebookData(
    NotebookItem & notebookItem, const QModelIndex & modelIndex,
    const QVariant & value)
{
    if (!canUpdateNotebookItem(notebookItem)) {
        ErrorString error{
            QT_TR_NOOP("Can't update the notebook, restrictions apply")};

        error.details() = notebookItem.name();
        QNINFO("model::NotebookModel", error << ", notebookItem = " << notebookItem);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::Name) &&
        !notebookItem.nameIsUpdatable())
    {
        ErrorString error{
            QT_TR_NOOP("Can't update the notebook's name, "
                       "restrictions apply")};

        error.details() = notebookItem.name();
        QNINFO(
            "model::NotebookModel",
            error << ", notebookItem = " << notebookItem);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto notebookItemIt = localIdIndex.find(notebookItem.localId());
    if (Q_UNLIKELY(notebookItemIt == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't update the notebook: internal error, can't "
                       "find the notebook to be updated in the model"));
        return false;
    }

    NotebookItem notebookItemCopy{notebookItem};
    bool dirty = notebookItemCopy.isDirty();

    switch (static_cast<Column>(modelIndex.column())) {
    case Column::Name:
    {
        if (!setNotebookName(notebookItemCopy, value.toString().trimmed())) {
            return false;
        }

        dirty = true;
        break;
    }
    case Column::Synchronizable:
    {
        const bool wasSynchronizable = notebookItemCopy.isSynchronizable();
        if (!setNotebookSynchronizable(notebookItemCopy, value.toBool())) {
            return false;
        }

        dirty |= (value.toBool() != wasSynchronizable);
        break;
    }
    case Column::Default:
    {
        if (!setNotebookIsDefault(notebookItemCopy, value.toBool())) {
            return false;
        }

        dirty = true;
        switchDefaultNotebookLocalId(notebookItemCopy.localId());
        break;
    }
    default:
        return false;
    }

    notebookItemCopy.setDirty(dirty);

    const bool sortingByName =
        (modelIndex.column() == static_cast<int>(Column::Name)) &&
        (m_sortedColumn == Column::Name);

    QNTRACE(
        "model::NotebookModel",
        "Sorting by name = " << (sortingByName ? "true" : "false"));

    localIdIndex.replace(notebookItemIt, notebookItemCopy);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    if (sortingByName) {
        updateItemRowWithRespectToSorting(
            const_cast<NotebookItem &>(*notebookItemIt));
    }

    updateNotebookInLocalStorage(notebookItemCopy);
    return true;
}

bool NotebookModel::setNotebookName(
    NotebookItem & notebookItem, const QString & newName)
{
    QNTRACE(
        "model::NotebookModel",
        "New suggested name: " << newName
                               << ", previous name = " << notebookItem.name());

    bool changed = (newName != notebookItem.name());
    if (!changed) {
        QNTRACE("model::NotebookModel", "Notebook name has not changed");
        return true;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    auto nameIt = nameIndex.find(newName.toUpper());
    if (nameIt != nameIndex.end()) {
        ErrorString error{
            QT_TR_NOOP("Can't rename the notebook: no two notebooks within the "
                       "account are allowed to have the same name in "
                       "case-insensitive manner")};
        QNINFO(
            "model::NotebookModel",
            error << ", suggested new name = " << newName);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    ErrorString errorDescription;
    if (!validateNotebookName(newName, &errorDescription)) {
        ErrorString error{QT_TR_NOOP("Can't rename the notebook")};
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNINFO(
            "model::NotebookModel",
            error << "; suggested new name = " << newName);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    notebookItem.setName(newName);
    return true;
}

bool NotebookModel::setNotebookSynchronizable(
    NotebookItem & notebookItem, const bool synchronizable)
{
    if (m_account.type() == Account::Type::Local) {
        ErrorString error{
            QT_TR_NOOP("Can't make the notebook synchronizable "
                       "within the local account")};

        QNINFO("model::NotebookModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    if (notebookItem.isSynchronizable() && !synchronizable) {
        ErrorString error{
            QT_TR_NOOP("Can't make the already synchronizable "
                       "notebook not synchronizable")};

        QNINFO(
            "model::NotebookModel",
            error << ", already synchronizable notebook "
                  << "item: " << notebookItem);

        Q_EMIT notifyError(std::move(error));
        return false;
    }

    notebookItem.setSynchronizable(synchronizable);
    return true;
}

bool NotebookModel::setNotebookIsDefault(
    NotebookItem & notebookItem, const bool isDefault)
{
    if (notebookItem.isDefault() == isDefault) {
        QNDEBUG(
            "model::NotebookModel",
            "The default state of the notebook hasn't changed, nothing to do");
        return true;
    }

    if (notebookItem.isDefault() && !isDefault) {
        ErrorString error{
            QT_TR_NOOP("In order to stop the notebook from being "
                       "the default one please choose another "
                       "default notebook")};
        QNINFO("model::NotebookModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    notebookItem.setDefault(true);
    return true;
}

QVariant NotebookModel::notebookData(
    const NotebookItem & notebookItem, const Column column) const
{
    switch (column) {
    case Column::Name:
        return notebookItem.name();
    case Column::Synchronizable:
        return notebookItem.isSynchronizable();
    case Column::Dirty:
        return notebookItem.isDirty();
    case Column::Default:
        return notebookItem.isDefault();
    case Column::Published:
        return notebookItem.isPublished();
    case Column::FromLinkedNotebook:
        return !notebookItem.linkedNotebookGuid().isEmpty();
    case Column::NoteCount:
        return notebookItem.noteCount();
    default:
        return {};
    }
}

QVariant NotebookModel::notebookAccessibleData(
    const NotebookItem & notebookItem, const Column column) const
{
    const QVariant textData = notebookData(notebookItem, column);
    if (textData.isNull()) {
        return {};
    }

    QString accessibleText = tr("Notebook") + QStringLiteral(": ");
    switch (column) {
    case Column::Name:
        accessibleText +=
            tr("name is") + QStringLiteral(" ") + textData.toString();
        break;
    case Column::Synchronizable:
        accessibleText +=
            (textData.toBool() ? tr("synchronizable")
                               : tr("not synchronizable"));
        break;
    case Column::Dirty:
        accessibleText +=
            (textData.toBool() ? tr("default") : tr("not default"));
        break;
    case Column::Published:
        accessibleText +=
            (textData.toBool() ? tr("published") : tr("not published"));
        break;
    case Column::FromLinkedNotebook:
        accessibleText +=
            (textData.toBool() ? tr("from linked notebook")
                               : tr("from own account"));
        break;
    case Column::NoteCount:
        accessibleText += tr("number of notes per notebook") +
            QStringLiteral(": ") + QString::number(notebookItem.noteCount());
        break;
    default:
        return {};
    }

    return QVariant{std::move(accessibleText)};
}

bool NotebookModel::canRemoveNotebookItem(const NotebookItem & notebookItem)
{
    if (!notebookItem.guid().isEmpty()) {
        ErrorString error{
            QT_TR_NOOP("One of notebooks being removed along with the stack "
                       "containing it has non-empty guid, it can't be "
                       "removed")};

        QNINFO("model::NotebookModel", error << ", notebook: " << notebookItem);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    if (!notebookItem.linkedNotebookGuid().isEmpty()) {
        ErrorString error{
            QT_TR_NOOP("One of notebooks being removed along with the stack "
                       "containing it is the linked notebook from another "
                       "account, it can't be removed")};

        QNINFO("model::NotebookModel", error << ", notebook: " << notebookItem);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    return true;
}

bool NotebookModel::updateNoteCountPerNotebookIndex(
    const NotebookItem & item, const NotebookDataByLocalId::iterator it)
{
    QNTRACE(
        "model::NotebookModel",
        "NotebookModel::updateNoteCountPerNotebookIndex: " << item);

    auto * modelItem = const_cast<NotebookItem *>(&(*it));
    auto * parentItem = modelItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the parent notebook model item "
                << "for the notebook item into which the note was inserted: "
                << *modelItem);
        return false;
    }

    const int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::NotebookModel",
            "Can't find the row of the child notebook "
                << "model item within its parent model item: parent item = "
                << *parentItem << "\nChild item = " << *modelItem);
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    localIdIndex.replace(it, item);

    const auto modelItemId = idForItem(*modelItem);

    QNTRACE(
        "model::NotebookModel",
        "Emitting num notes per notebook update dataChanged signal: row = "
            << row << ", model item: " << *modelItem);

    const auto modelIndexFrom =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    const auto modelIndexTo =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);
    return true;
}

void NotebookModel::clearModel()
{
    QNDEBUG("model::NotebookModel", "NotebookModel::clearModel");

    beginResetModel();

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    m_data.clear();

    delete m_allNotebooksRootItem;
    m_allNotebooksRootItem = nullptr;

    delete m_invisibleRootItem;
    m_invisibleRootItem = nullptr;

    m_allNotebooksRootItemIndexId = 1;
    m_defaultNotebookLocalId.clear();

    m_stackItems.clear();
    m_stackItemsByLinkedNotebookGuid.clear();

    m_linkedNotebookItems.clear();
    m_indexIdToLocalIdBimap.clear();
    m_indexIdToStackAndLinkedNotebookGuidBimap.clear();
    m_indexIdToLinkedNotebookGuidBimap.clear();

    m_listNotebooksOffset = 0;
    m_notebookItemsNotYetInLocalStorageIds.clear();
    m_linkedNotebookUsernamesByGuids.clear();
    m_listLinkedNotebooksOffset = 0;

    m_allNotebooksListed = false;
    m_allLinkedNotebooksListed = false;

    endResetModel();
}

utility::cancelers::ICancelerPtr NotebookModel::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

bool NotebookModel::LessByName::operator()(
    const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

#define ITEM_PTR_LESS(lhs, rhs)                                                \
    if (!lhs) {                                                                \
        return true;                                                           \
    }                                                                          \
    else if (!rhs) {                                                           \
        return false;                                                          \
    }                                                                          \
    else {                                                                     \
        return this->operator()(*lhs, *rhs);                                   \
    }                                                                          \
    // ITEM_PTR_LESS

bool NotebookModel::LessByName::operator()(
    const NotebookItem * lhs,
    const NotebookItem * rhs) const {ITEM_PTR_LESS(lhs, rhs)}

QString modelItemName(const INotebookModelItem & item)
{
    const bool isNotebookItem =
        (item.type() == INotebookModelItem::Type::Notebook);

    const bool isLinkedNotebookItem = !isNotebookItem &&
        (item.type() == INotebookModelItem::Type::LinkedNotebook);

    const bool isStackItem = !isNotebookItem && !isLinkedNotebookItem &&
        (item.type() == INotebookModelItem::Type::Stack);

    const NotebookItem * notebookItem = nullptr;
    if (isNotebookItem) {
        notebookItem = dynamic_cast<const NotebookItem *>(&item);
    }

    const LinkedNotebookRootItem * linkedNotebookItem = nullptr;
    if (isLinkedNotebookItem) {
        linkedNotebookItem =
            dynamic_cast<const LinkedNotebookRootItem *>(&item);
    }

    const StackItem * stackItem = nullptr;
    if (!isNotebookItem && !isLinkedNotebookItem) {
        stackItem = dynamic_cast<const StackItem *>(&item);
    }

    if (isNotebookItem && notebookItem) {
        return notebookItem->nameUpper();
    }

    if (isStackItem && stackItem) {
        return stackItem->name().toUpper();
    }

    if (isLinkedNotebookItem && linkedNotebookItem) {
        return linkedNotebookItem->username();
    }

    return {};
}

bool NotebookModel::LessByName::operator()(
    const INotebookModelItem & lhs, const INotebookModelItem & rhs) const
{
    if (lhs.type() == INotebookModelItem::Type::AllNotebooksRoot &&
        rhs.type() != INotebookModelItem::Type::AllNotebooksRoot)
    {
        return false;
    }

    if (lhs.type() != INotebookModelItem::Type::AllNotebooksRoot &&
        rhs.type() == INotebookModelItem::Type::AllNotebooksRoot)
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if (lhs.type() == INotebookModelItem::Type::LinkedNotebook &&
        rhs.type() != INotebookModelItem::Type::LinkedNotebook)
    {
        return false;
    }

    if (lhs.type() != INotebookModelItem::Type::LinkedNotebook &&
        rhs.type() == INotebookModelItem::Type::LinkedNotebook)
    {
        return true;
    }

    const QString lhsName = modelItemName(lhs);
    const QString rhsName = modelItemName(rhs);
    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool NotebookModel::LessByName::operator()(
    const INotebookModelItem * pLhs, const INotebookModelItem * pRhs) const
{
    ITEM_PTR_LESS(pLhs, pRhs)
}

bool NotebookModel::LessByName::operator()(
    const StackItem & lhs, const StackItem & rhs) const
{
    return (lhs.name().toUpper().localeAwareCompare(rhs.name().toUpper()) <= 0);
}

bool NotebookModel::LessByName::operator()(
    const StackItem * lhs, const StackItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::LessByName::operator()(
    const LinkedNotebookRootItem & lhs,
    const LinkedNotebookRootItem & rhs) const
{
    return (
        lhs.username().toUpper().localeAwareCompare(rhs.username().toUpper()) <=
        0);
}

bool NotebookModel::LessByName::operator()(
    const LinkedNotebookRootItem * lhs,
    const LinkedNotebookRootItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(
    const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

#define ITEM_PTR_GREATER(lhs, rhs)                                             \
    if (!lhs) {                                                                \
        return false;                                                          \
    }                                                                          \
    else if (!rhs) {                                                           \
        return true;                                                           \
    }                                                                          \
    else {                                                                     \
        return this->operator()(*lhs, *rhs);                                   \
    }                                                                          \
    // ITEM_PTR_GREATER

bool NotebookModel::GreaterByName::operator()(
    const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(
    const StackItem & lhs, const StackItem & rhs) const
{
    return (lhs.name().toUpper().localeAwareCompare(rhs.name().toUpper()) > 0);
}

bool NotebookModel::GreaterByName::operator()(
    const StackItem * lhs, const StackItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(
    const LinkedNotebookRootItem & lhs,
    const LinkedNotebookRootItem & rhs) const
{
    return (
        lhs.username().toUpper().localeAwareCompare(rhs.username().toUpper()) >
        0);
}

bool NotebookModel::GreaterByName::operator()(
    const LinkedNotebookRootItem * lhs,
    const LinkedNotebookRootItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(
    const INotebookModelItem & lhs, const INotebookModelItem & rhs) const
{
    if (lhs.type() == INotebookModelItem::Type::AllNotebooksRoot &&
        rhs.type() != INotebookModelItem::Type::AllNotebooksRoot)
    {
        return false;
    }

    if (lhs.type() != INotebookModelItem::Type::AllNotebooksRoot &&
        rhs.type() == INotebookModelItem::Type::AllNotebooksRoot)
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if (lhs.type() == INotebookModelItem::Type::LinkedNotebook &&
        rhs.type() != INotebookModelItem::Type::LinkedNotebook)
    {
        return false;
    }

    if (lhs.type() != INotebookModelItem::Type::LinkedNotebook &&
        rhs.type() == INotebookModelItem::Type::LinkedNotebook)
    {
        return true;
    }

    const QString lhsName = modelItemName(lhs);
    const QString rhsName = modelItemName(rhs);
    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool NotebookModel::GreaterByName::operator()(
    const INotebookModelItem * pLhs,
    const INotebookModelItem * pRhs) const {ITEM_PTR_GREATER(pLhs, pRhs)}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const NotebookModel::Column column)
{
    using Column = NotebookModel::Column;

    switch (column) {
    case Column::Name:
        dbg << "Name";
        break;
    case Column::Synchronizable:
        dbg << "Synchronizable";
        break;
    case Column::Dirty:
        dbg << "Dirty";
        break;
    case Column::Default:
        dbg << "Default";
        break;
    case Column::Published:
        dbg << "Published";
        break;
    case Column::FromLinkedNotebook:
        dbg << "From linked notebook";
        break;
    case Column::NoteCount:
        dbg << "Note count";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(column) << ")";
        break;
    }

    return dbg;
}

QDebug & operator<<(QDebug & dbg, const NotebookModel::Filter filter)
{
    using Filter = NotebookModel::Filter;

    switch (filter) {
    case Filter::NoFilter:
        dbg << "No filter";
        break;
    case Filter::CanCreateNotes:
        dbg << "Can create notes";
        break;
    case Filter::CannotCreateNotes:
        dbg << "Cannot create notes";
        break;
    case Filter::CanUpdateNotes:
        dbg << "Can update notes";
        break;
    case Filter::CannotUpdateNotes:
        dbg << "Cannot update notes";
        break;
    case Filter::CanUpdateNotebook:
        dbg << "Can update notebook";
        break;
    case Filter::CannotUpdateNotebook:
        dbg << "Cannot update notebook";
        break;
    case Filter::CanRenameNotebook:
        dbg << "Can rename notebook";
        break;
    case Filter::CannotRenameNotebook:
        dbg << "Cannot rename notebook";
        break;
    }

    return dbg;
}

QDebug & operator<<(QDebug & dbg, const NotebookModel::Filters filters)
{
    using Filter = NotebookModel::Filter;

    dbg << "Enabled NotebookModel::Filters:\n";

    auto checkFlag = [&](const Filter filter, const char * text) {
        if (filters.testFlag(filter)) {
            dbg << "  " << text << "\n";
        }
    };

    checkFlag(Filter::CanCreateNotes, "can create notes");
    checkFlag(Filter::CannotCreateNotes, "cannot create notes");
    checkFlag(Filter::CanUpdateNotes, "can update notes");
    checkFlag(Filter::CannotUpdateNotes, "cannot update notes");
    checkFlag(Filter::CanUpdateNotebook, "can update notebook");
    checkFlag(Filter::CannotUpdateNotebook, "cannot update notebook");
    checkFlag(Filter::CanRenameNotebook, "can rename notebook");
    checkFlag(Filter::CannotRenameNotebook, "cannot rename notebook");

    return dbg;
}

QDebug & operator<<(
    QDebug & dbg,
    const NotebookModel::RemoveEmptyParentStack removeEmptyParentStack)
{
    using RemoveEmptyParentStack = NotebookModel::RemoveEmptyParentStack;

    switch (removeEmptyParentStack) {
    case RemoveEmptyParentStack::Yes:
        dbg << "Yes";
        break;
    case RemoveEmptyParentStack::No:
        dbg << "No";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(removeEmptyParentStack)
            << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
