/*
 * Copyright 2016-2021 Dmitry Ivanov
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
#include "InvisibleRootItem.h"

#include <lib/model/common/NewItemNameGenerator.hpp>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/UidGenerator.h>

#include <QDataStream>
#include <QMimeData>

namespace quentier {

// Limit for the queries to the local storage
#define NOTEBOOK_LIST_LIMIT        (40)
#define LINKED_NOTEBOOK_LIST_LIMIT (40)

#define NUM_NOTEBOOK_MODEL_COLUMNS (8)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    QNWARNING("model:notebook", errorDescription << "" __VA_ARGS__);           \
    Q_EMIT notifyError(errorDescription) // REPORT_ERROR

NotebookModel::NotebookModel(
    const Account & account,
    LocalStorageManagerAsync & localStorageManagerAsync, NotebookCache & cache,
    QObject * parent) :
    AbstractItemModel(account, parent),
    m_cache(cache)
{
    createConnections(localStorageManagerAsync);

    requestNotebooksList();
    requestLinkedNotebooksList();
}

NotebookModel::~NotebookModel()
{
    delete m_pAllNotebooksRootItem;
    delete m_pInvisibleRootItem;
}

void NotebookModel::setAccount(Account account)
{
    QNTRACE("model:notebook", "NotebookModel::setAccount: " << account);
    m_account = std::move(account);
}

INotebookModelItem * NotebookModel::itemForIndex(
    const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_pInvisibleRootItem;
    }

    return itemForId(static_cast<IndexId>(index.internalId()));
}

QModelIndex NotebookModel::indexForItem(const INotebookModelItem * pItem) const
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::indexForItem: "
            << (pItem ? pItem->toString() : QStringLiteral("<null>")));

    if (!pItem) {
        return {};
    }

    if (pItem == m_pInvisibleRootItem) {
        return {};
    }

    if (pItem == m_pAllNotebooksRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allNotebooksRootItemIndexId);
    }

    const auto * pParentItem = pItem->parent();
    if (!pParentItem) {
        QNWARNING(
            "model:notebook",
            "Notebook model item has no parent, "
                << "returning invalid index for it: " << *pItem);
        return {};
    }

    QNTRACE("model:notebook", "Parent item: " << *pParentItem);
    const int row = pParentItem->rowForChild(pItem);
    QNTRACE("model:notebook", "Child row: " << row);

    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Internal error: can't get the row of "
                << "the child item in parent in NotebookModel, child item: "
                << *pItem << "\nParent item: " << *pParentItem);
        return {};
    }

    const auto id = idForItem(*pItem);
    if (Q_UNLIKELY(id == 0)) {
        QNWARNING(
            "model:notebook",
            "The notebook model item has the internal "
                << "id of 0: " << *pItem);
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

QModelIndex NotebookModel::indexForNotebookName(
    const QString & notebookName, const QString & linkedNotebookGuid) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();
    auto range = nameIndex.equal_range(notebookName.toUpper());
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
    const auto * pStackItems = stackItems(linkedNotebookGuid);
    if (Q_UNLIKELY(!pStackItems)) {
        return {};
    }

    const auto it = pStackItems->find(stack);
    if (it == pStackItems->end()) {
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
            "model:notebook",
            "Found no model item for linked notebook "
                << "guid " << linkedNotebookGuid);
        return {};
    }

    const LinkedNotebookRootItem & item = it.value();
    return indexForItem(&item);
}

QStringList NotebookModel::notebookNames(
    const Filters filters, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::notebookNames: filters = "
            << filters << ", linked notebook guid = " << linkedNotebookGuid
            << " (null = " << (linkedNotebookGuid.isNull() ? "true" : "false")
            << ", empty = " << (linkedNotebookGuid.isEmpty() ? "true" : "false")
            << ")");

    if (filters == Filters(Filter::NoFilter)) {
        QNTRACE("model:notebook", "No filter, returning all notebook names");
        return itemNames(linkedNotebookGuid);
    }

    QStringList result;

    if ((filters & Filter::CanCreateNotes) &&
        (filters & Filter::CannotCreateNotes)) {
        QNTRACE(
            "model:notebook",
            "Both can create notes and cannot create notes filters are "
                << "included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotes) &&
        (filters & Filter::CannotUpdateNotes)) {
        QNTRACE(
            "model:notebook",
            "Both can update notes and cannot update notes filters are "
                << "included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotebook) &&
        (filters & Filter::CannotUpdateNotebook))
    {
        QNTRACE(
            "model:notebook",
            "Both can update notebooks and cannot update notebooks filters are "
                << "included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanRenameNotebook) &&
        (filters & Filter::CannotRenameNotebook))
    {
        QNTRACE(
            "model:notebook",
            "Both can rename notebook and cannot rename notebook filters are "
                << "included, the result is empty");
        return result;
    }

    const auto & nameIndex = m_data.get<ByNameUpper>();
    result.reserve(static_cast<int>(nameIndex.size()));

    for (const auto & item: nameIndex) {
        if (!notebookItemMatchesByLinkedNotebook(item, linkedNotebookGuid)) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item with different linked notebook guid: "
                    << item);
            continue;
        }

        const bool canCreateNotes = item.canCreateNotes();
        if ((filters & Filter::CanCreateNotes) && !canCreateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes cannot be created: "
                    << item);
            continue;
        }

        if ((filters & Filter::CannotCreateNotes) && canCreateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes can be created: "
                    << item);
            continue;
        }

        const bool canUpdateNotes = item.canUpdateNotes();
        if ((filters & Filter::CanUpdateNotes) && !canUpdateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes cannot be updated: "
                    << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotes) && canUpdateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes can be updated: "
                    << item);
            continue;
        }

        const bool canUpdateNotebook = item.isUpdatable();
        if ((filters & Filter::CanUpdateNotebook) && !canUpdateNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which cannot be updated: " << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotebook) && canUpdateNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which can be updated: " << item);
            continue;
        }

        const bool canRenameNotebook = item.nameIsUpdatable();
        if ((filters & Filter::CanRenameNotebook) && !canRenameNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which cannot be renamed: " << item);
            continue;
        }

        if ((filters & Filter::CannotRenameNotebook) && canRenameNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which can be renamed: " << item);
            continue;
        }

        result << item.name();
    }

    return result;
}

QModelIndex NotebookModel::defaultNotebookIndex() const
{
    QNTRACE("model:notebook", "NotebookModel::defaultNotebookIndex");

    if (m_defaultNotebookLocalId.isEmpty()) {
        QNDEBUG("model:notebook", "No default notebook local id");
        return {};
    }

    return indexForLocalId(m_defaultNotebookLocalId);
}

QModelIndex NotebookModel::lastUsedNotebookIndex() const
{
    QNTRACE("model:notebook", "NotebookModel::lastUsedNotebookIndex");

    if (m_lastUsedNotebookLocalId.isEmpty()) {
        QNDEBUG("model:notebook", "No last used notebook local id");
        return {};
    }

    return indexForLocalId(m_lastUsedNotebookLocalId);
}

QModelIndex NotebookModel::moveToStack(
    const QModelIndex & index, const QString & stack)
{
    QNDEBUG("model:notebook", "NotebookModel::moveToStack: stack = " << stack);

    if (Q_UNLIKELY(stack.isEmpty())) {
        return removeFromStack(index);
    }

    auto * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to move "
                       "a notebook to another stack but the respective "
                       "model index has no internal id corresponding "
                       "to the notebook model item"));
        return {};
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (!pNotebookItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move the non-notebook item into a stack"));
        return {};
    }

    QNTRACE(
        "model:notebook",
        "Notebook item to be moved to another stack: " << *pNotebookItem);

    if (pNotebookItem->stack() == stack) {
        QNDEBUG(
            "model:notebook",
            "The stack of the item hasn't changed, "
                << "nothing to do");
        return index;
    }

    removeModelItemFromParent(*pModelItem);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(pNotebookItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find the notebook "
                       "item to be moved to another stack"));

        return {};
    }

    auto stackItemsWithParentPair =
        stackItemsWithParent(pNotebookItem->linkedNotebookGuid());

    auto & stackItems = *stackItemsWithParentPair.first;
    auto * pStackParentItem = stackItemsWithParentPair.second;

    auto & stackItem =
        findOrCreateStackItem(stack, stackItems, pStackParentItem);

    NotebookItem notebookItemCopy{*pNotebookItem};
    notebookItemCopy.setStack(stack);
    notebookItemCopy.setDirty(true);

    localIdIndex.replace(it, notebookItemCopy);
    updateNotebookInLocalStorage(notebookItemCopy);

    auto * pNewParentItem = &stackItem;
    const auto parentIndex = indexForItem(pNewParentItem);

    const int newRow = rowForNewItem(*pNewParentItem, *pModelItem);
    beginInsertRows(parentIndex, newRow, newRow);
    pNewParentItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QNTRACE(
        "model:notebook",
        "Model item after moving to the stack: " << *pModelItem);

    Q_EMIT notifyNotebookStackChanged(index);
    return indexForItem(pModelItem);
}

QModelIndex NotebookModel::removeFromStack(const QModelIndex & index)
{
    QNDEBUG("model:notebook", "NotebookModel::removeFromStack");

    auto * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find a notebook to be "
                       "removed from the stack, no item corresponding to id"));
        return {};
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't remove the non-notebook model item from "
                       "the stack"));
        return {};
    }

    if (pNotebookItem->stack().isEmpty()) {
        QNWARNING(
            "model:notebook",
            "The notebook doesn't appear to have "
                << "a stack but will continue the attempt "
                << "to remove it from the stack anyway");
    }
    else {
        auto & localIdIndex = m_data.get<ByLocalId>();
        const auto it = localIdIndex.find(pNotebookItem->localId());
        if (Q_UNLIKELY(it == localIdIndex.end())) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't find the notebook item to be removed "
                           "from the stack by the local id"));
            return {};
        }

        NotebookItem notebookItemCopy{*pNotebookItem};
        notebookItemCopy.setStack(QString());
        notebookItemCopy.setDirty(true);
        localIdIndex.replace(it, notebookItemCopy);

        updateNotebookInLocalStorage(notebookItemCopy);
    }

    auto * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(
            "model:notebook",
            "Notebook item to be removed from "
                << "the stack doesn't have a parent, setting it to all "
                   "notebooks "
                << "root item");

        checkAndCreateModelRootItems();

        const int newRow = rowForNewItem(*m_pAllNotebooksRootItem, *pModelItem);
        beginInsertRows(indexForItem(m_pAllNotebooksRootItem), newRow, newRow);
        m_pAllNotebooksRootItem->insertChild(newRow, pModelItem);
        endInsertRows();

        return indexForItem(pModelItem);
    }

    if (pParentItem->type() != INotebookModelItem::Type::Stack) {
        QNDEBUG(
            "model:notebook",
            "The notebook item doesn't belong to any "
                << "stack, nothing to do");
        return index;
    }

    checkAndCreateModelRootItems();
    const QString & linkedNotebookGuid = pNotebookItem->linkedNotebookGuid();

    auto * pNewParentItem =
        (linkedNotebookGuid.isEmpty()
             ? m_pAllNotebooksRootItem
             : (&(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid))));

    removeModelItemFromParent(*pModelItem);

    const auto parentItemIndex = indexForItem(pNewParentItem);

    const int newRow = rowForNewItem(*pNewParentItem, *pModelItem);
    beginInsertRows(parentItemIndex, newRow, newRow);
    pNewParentItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QNTRACE(
        "model:notebook",
        "Model item after removing it from the stack: " << *pModelItem);

    Q_EMIT notifyNotebookStackChanged(index);
    return indexForItem(pModelItem);
}

QStringList NotebookModel::stacks(const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::stacks: linked notebook guid = " << linkedNotebookGuid);

    QStringList result;

    const StackItems * pStackItems = nullptr;
    if (linkedNotebookGuid.isEmpty()) {
        pStackItems = &m_stackItems;
    }
    else {
        const auto stackItemsIt =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

        if (stackItemsIt == m_stackItemsByLinkedNotebookGuid.end()) {
            QNTRACE(
                "model:notebook",
                "Found no stacks for this linked "
                    << "notebook guid");
            return result;
        }

        pStackItems = &(stackItemsIt.value());
    }

    result.reserve(pStackItems->size());

    for (auto it = pStackItems->constBegin(), end = pStackItems->constEnd();
         it != end; ++it)
    {
        result.push_back(it.value().name());
    }

    QNTRACE(
        "model:notebook",
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
        "model:notebook",
        "NotebookModel::createNotebook: notebook name = "
            << notebookName << ", notebook stack = " << notebookStack);

    if (notebookName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Notebook name is empty"));
        return {};
    }

    const int notebookNameSize = notebookName.size();
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
    auto * pParentItem = m_pAllNotebooksRootItem;

    Q_EMIT aboutToAddNotebook();

    if (!notebookStack.isEmpty()) {
        pParentItem = &(findOrCreateStackItem(
            notebookStack, m_stackItems, m_pAllNotebooksRootItem));

        QNDEBUG(
            "model:notebook",
            "Will put the new notebook under parent "
                << "stack item: " << *pParentItem);
    }

    const auto parentIndex = indexForItem(pParentItem);

    // Will insert the notebook to the end of the parent item's children
    const int row = pParentItem->childrenCount();

    NotebookItem item;
    item.setLocalId(UidGenerator::Generate());
    Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.insert(item.localId()))

    item.setName(notebookName);
    item.setDirty(true);
    item.setStack(notebookStack);
    item.setSynchronizable(m_account.type() != Account::Type::Local);
    item.setDefault(numExistingNotebooks == 0);
    item.setLastUsed(numExistingNotebooks == 0);
    item.setUpdatable(true);
    item.setNameIsUpdatable(true);

    const auto insertionResult = localIdIndex.insert(item);

    auto * pAddedNotebookItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    pAddedNotebookItem->setParent(pParentItem);
    endInsertRows();

    updateNotebookInLocalStorage(*pAddedNotebookItem);

    auto addedNotebookIndex = indexForLocalId(item.localId());

    if (m_sortedColumn != Column::Name) {
        QNDEBUG(
            "model:notebook",
            "Notebook model is not sorted by name, "
                << "skipping sorting");
        Q_EMIT addedNotebook(addedNotebookIndex);
        return addedNotebookIndex;
    }

    Q_EMIT layoutAboutToBeChanged();
    for (const auto & item: localIdIndex) {
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
    case Column::LastUsed:
        return tr("Last used");
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
        "model:notebook",
        "NotebookModel::favoriteNotebook: index: "
            << "is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setNotebookFavorited(index, true);
}

void NotebookModel::unfavoriteNotebook(const QModelIndex & index)
{
    QNDEBUG(
        "model:notebook",
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
        "model:notebook",
        "NotebookModel::localIdForItemName: name = "
            << itemName << ", linked notebook guid = " << linkedNotebookGuid);

    const auto index = indexForNotebookName(itemName, linkedNotebookGuid);
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        QNTRACE(
            "model:notebook",
            "No notebook model item found for index "
                << "found for this notebook name");
        return {};
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (Q_UNLIKELY(!pNotebookItem)) {
        QNTRACE("model:notebook", "Found notebook model item has wrong type");
        return {};
    }

    return pNotebookItem->localId();
}

QModelIndex NotebookModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (it == localIdIndex.end()) {
        QNTRACE(
            "model:notebook",
            "Found no notebook model item for local id " << localId);
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString NotebookModel::itemNameForLocalId(const QString & localId) const
{
    QNTRACE(
        "model:notebook", "NotebookModel::itemNameForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model:notebook", "No notebook item with such local id");
        return {};
    }

    return it->name();
}

AbstractItemModel::ItemInfo NotebookModel::itemInfoForLocalId(
    const QString & localId) const
{
    QNTRACE(
        "model:notebook", "NotebookModel::itemInfoForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model:notebook", "No notebook item with such local id");
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
                auto prevIt = it;
                --prevIt;
                if (prevIt->name() == name) {
                    continue;
                }
            }

            result << name;
            continue;
        }

        if (linkedNotebookGuid.isEmpty() && item.linkedNotebookGuid().isEmpty())
        {
            result << name;
            continue;
        }

        if (linkedNotebookGuid == item.linkedNotebookGuid()) {
            result << name;
        }
    }

    return result;
}

QVector<AbstractItemModel::LinkedNotebookInfo>
NotebookModel::linkedNotebooksInfo() const
{
    QVector<LinkedNotebookInfo> infos;
    infos.reserve(m_linkedNotebookItems.size());

    for (const auto & it: qevercloud::toRange(m_linkedNotebookItems)) {
        infos.push_back(LinkedNotebookInfo{it.key(), it.value().username()});
    }

    return infos;
}

QString NotebookModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it != m_linkedNotebookItems.end()) {
        const auto & item = it.value();
        return item.username();
    }

    return {};
}

QModelIndex NotebookModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_pAllNotebooksRootItem)) {
        return {};
    }

    return indexForItem(m_pAllNotebooksRootItem);
}

QString NotebookModel::localIdForItemIndex(const QModelIndex & index) const
{
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        return pNotebookItem->localId();
    }

    return {};
}

QString NotebookModel::linkedNotebookGuidForItemIndex(
    const QModelIndex & index) const
{
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    auto * pLinkedNotebookItem = pModelItem->cast<LinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        return pLinkedNotebookItem->linkedNotebookGuid();
    }

    return {};
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

    auto * pModelItem = itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        QNWARNING(
            "model:notebook",
            "Can't find notebook model item for "
                << "a given index: row = " << index.row()
                << ", column = " << index.column());
        return indexFlags;
    }

    auto * pStackItem = pModelItem->cast<StackItem>();
    if (pStackItem) {
        return flagsForStackItem(*pStackItem, index.column(), indexFlags);
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        return flagsForNotebookItem(*pNotebookItem, index.column(), indexFlags);
    }

    return adjustFlagsForColumn(index.column(), indexFlags);
}

QVariant NotebookModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_NOTEBOOK_MODEL_COLUMNS)) {
        return {};
    }

    const auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    if (pModelItem == m_pInvisibleRootItem) {
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
    case Column::LastUsed:
        column = Column::LastUsed;
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
        return dataImpl(*pModelItem, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*pModelItem, column);
    default:
        return {};
    }
}

QVariant NotebookModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    return columnName(static_cast<Column>(section));
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    auto * pParentItem = itemForIndex(parent);
    return (pParentItem ? pParentItem->childrenCount() : 0);
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    return NUM_NOTEBOOK_MODEL_COLUMNS;
}

QModelIndex NotebookModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if ((row < 0) || (column < 0) || (column >= NUM_NOTEBOOK_MODEL_COLUMNS) ||
        (parent.isValid() &&
         (parent.column() != static_cast<int>(Column::Name))))
    {
        return {};
    }

    auto * pParentItem = itemForIndex(parent);
    if (!pParentItem) {
        return {};
    }

    auto * pModelItem = pParentItem->childAtRow(row);
    if (!pModelItem) {
        return {};
    }

    const IndexId id = idForItem(*pModelItem);
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

    auto * pChildItem = itemForIndex(index);
    if (!pChildItem) {
        return {};
    }

    auto * pParentItem = pChildItem->parent();
    if (!pParentItem) {
        return {};
    }

    if (pParentItem == m_pInvisibleRootItem) {
        return {};
    }

    if (pParentItem == m_pAllNotebooksRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allNotebooksRootItemIndexId);
    }

    auto * pGrandParentItem = pParentItem->parent();
    if (!pGrandParentItem) {
        return {};
    }

    const int row = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Internal inconsistency detected in "
                << "NotebookModel: parent of the item can't find the item "
                   "within "
                << "its children: item = " << *pParentItem
                << "\nParent item: " << *pGrandParentItem);
        return {};
    }

    const IndexId id = idForItem(*pParentItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

bool NotebookModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::setData: row = "
            << modelIndex.row() << ", column = " << modelIndex.column()
            << ", internal id = " << modelIndex.internalId()
            << ", value = " << value << ", role = " << role);

    if (role != Qt::EditRole) {
        QNDEBUG("model:notebook", "Role is not EditRole, skipping");
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG("model:notebook", "Index is not valid");
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

    auto * pModelItem = itemForIndex(modelIndex);
    if (!pModelItem) {
        REPORT_ERROR(
            QT_TR_NOOP("No notebook model item was found for the given "
                       "model index"));
        return false;
    }

    if (Q_UNLIKELY(pModelItem == m_pInvisibleRootItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the invisible root item "
                       "within the notebook model"));
        return false;
    }

    if (Q_UNLIKELY(
            pModelItem->type() == INotebookModelItem::Type::LinkedNotebook)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the linked notebook root item"));
        return false;
    }

    if (Q_UNLIKELY(
            pModelItem->type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        REPORT_ERROR(QT_TR_NOOP("Can't set data for all notebooks root item"));
        return false;
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        return setNotebookData(*pNotebookItem, modelIndex, value);
    }

    auto * pStackItem = pModelItem->cast<StackItem>();
    if (pStackItem) {
        return setStackData(*pStackItem, modelIndex, value);
    }

    QNWARNING(
        "model:notebook",
        "Cannot set data to notebook model item, "
            << "failed to determine model item type");
    return false;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::insertRows: row = "
            << row << ", count = " << count
            << ", parent: is valid = " << (parent.isValid() ? "true" : "false")
            << ", row = " << parent.row() << ", column = " << parent.column());

    auto * pParentItem =
        (parent.isValid() ? itemForIndex(parent) : m_pInvisibleRootItem);

    if (!pParentItem) {
        QNDEBUG("model:notebook", "No model item for given model index");
        return false;
    }

    auto * pStackItem = pParentItem->cast<StackItem>();
    if (Q_UNLIKELY(!pStackItem)) {
        QNDEBUG(
            "model:notebook",
            "Only the insertion of notebooks under "
                << "notebook stacks is supported");
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const int numExistingNotebooks = static_cast<int>(localIdIndex.size());
    if (Q_UNLIKELY(
            numExistingNotebooks + count >= m_account.notebookCountMax())) {
        ErrorString error(
            QT_TR_NOOP("Can't create a new notebook: the account can contain "
                       "a limited number of notebooks"));

        error.details() = QString::number(m_account.notebookCountMax());
        QNINFO("model:notebook", error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<NotebookDataByLocalId::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Adding notebook item
        NotebookItem item;
        item.setLocalId(UidGenerator::Generate());

        Q_UNUSED(
            m_notebookItemsNotYetInLocalStorageUids.insert(item.localId()))

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(pStackItem->name());
        item.setSynchronizable(m_account.type() != Account::Type::Local);
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);

        const auto insertionResult = localIdIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        INotebookModelItem * pAddedNotebookItem =
            const_cast<NotebookItem *>(&(*insertionResult.first));

        pAddedNotebookItem->setParent(pParentItem);
    }
    endInsertRows();

    for (const auto & itemIt: qAsConst(addedItems)) {
        updateNotebookInLocalStorage(*itemIt);
    }

    if (m_sortedColumn == Column::Name) {
        Q_EMIT layoutAboutToBeChanged();
        for (const auto & item: localIdIndex) {
            updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
        }
        Q_EMIT layoutChanged();
    }

    QNDEBUG("model:notebook", "Successfully inserted the row(s)");
    return true;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::removeRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    RemoveRowsScopeGuard removeRowsScopeGuard{*this};
    Q_UNUSED(removeRowsScopeGuard)

    checkAndCreateModelRootItems();

    auto * pParentItem =
        (parent.isValid() ? itemForIndex(parent) : m_pInvisibleRootItem);

    if (!pParentItem) {
        QNDEBUG("model:notebook", "No item corresponding to parent index");
        return false;
    }

    if (pParentItem == m_pInvisibleRootItem) {
        QNDEBUG(
            "model:notebook",
            "Can't remove direct child of invisible "
                << "root item (all notebooks root item)");
        return false;
    }

    if (Q_UNLIKELY(
            (pParentItem != m_pAllNotebooksRootItem) &&
            (pParentItem->type() != INotebookModelItem::Type::Stack)))
    {
        QNDEBUG(
            "model:notebook",
            "Can't remove row(s) from parent item not "
                << "being a stack item or all notebooks root item: "
                << *pParentItem);
        return false;
    }

    QStringList notebookLocalIdsToRemove;
    QVector<std::pair<QString, QString>> stacksToRemoveWithLinkedNotebookGuids;

    // just a semi-random guess
    stacksToRemoveWithLinkedNotebookGuids.reserve(count / 2);

    QString linkedNotebookGuid;
    if ((pParentItem->type() == INotebookModelItem::Type::LinkedNotebook)) {
        auto * pLinkedNotebookItem =
            pParentItem->cast<LinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    // First simply collect all the local ids and stacks of items to be removed
    // while checking for each of them whether they can be safely removed
    for (int i = 0; i < count; ++i) {
        auto * pChildItem = pParentItem->childAtRow(row + i);
        if (!pChildItem) {
            QNWARNING(
                "model:notebook",
                "Detected null pointer to child "
                    << "notebook model item on attempt to remove row "
                    << (row + i) << " from parent item: " << *pParentItem);
            continue;
        }

        QNTRACE(
            "model:notebook",
            "Removing item at "
                << pChildItem << ": " << *pChildItem << " at row " << (row + i)
                << " from parent item at " << pParentItem << ": "
                << *pParentItem);

        auto * pNotebookItem = pChildItem->cast<NotebookItem>();
        if (pNotebookItem) {
            if (!canRemoveNotebookItem(*pNotebookItem)) {
                return false;
            }

            notebookLocalIdsToRemove << pNotebookItem->localId();
            QNTRACE(
                "model:notebook",
                "Marked notebook local id "
                    << pNotebookItem->localId()
                    << " as the one scheduled for removal");
        }

        auto * pStackItem = pChildItem->cast<StackItem>();
        if (!pStackItem) {
            continue;
        }

        QNTRACE(
            "model:notebook",
            "Removing notebook stack: first remove all "
                << "its child notebooks and then itself");

        auto notebookModelItemsWithinStack = pChildItem->children();
        for (const auto * pNotebookModelItem:
             qAsConst(notebookModelItemsWithinStack))
        {
            if (Q_UNLIKELY(!pNotebookModelItem)) {
                QNWARNING(
                    "model:notebook",
                    "Detected null pointer to notebook "
                        << "model item within the children "
                        << "of the stack item being removed: " << *pChildItem);
                continue;
            }

            const auto * pChildNotebookItem =
                pNotebookModelItem->cast<NotebookItem>();

            if (Q_UNLIKELY(!pChildNotebookItem)) {
                QNWARNING(
                    "model:notebook",
                    "Detected notebook model item "
                        << "within the stack item which is not "
                        << "a notebook by type; stack item: " << *pChildItem
                        << "\nIts child with "
                        << "wrong type: " << *pNotebookModelItem);
                continue;
            }

            QNTRACE(
                "model:notebook",
                "Removing notebook item under stack at "
                    << pChildNotebookItem << ": " << *pChildNotebookItem);

            if (!canRemoveNotebookItem(*pChildNotebookItem)) {
                return false;
            }

            notebookLocalIdsToRemove << pChildNotebookItem->localId();

            QNTRACE(
                "model:notebook",
                "Marked notebook local id "
                    << pChildNotebookItem->localId()
                    << " as the one scheduled for removal");
        }

        stacksToRemoveWithLinkedNotebookGuids.push_back(
            std::pair<QString, QString>(
                pStackItem->name(), linkedNotebookGuid));

        QNTRACE(
            "model:notebook",
            "Marked notebook stack " << pStackItem->name()
                                     << " as the one scheduled for removal");
    }

    // Now we are sure all the items collected for the deletion can actually be
    // deleted

    auto & localIdIndex = m_data.get<ByLocalId>();

    for (int i = 0, size = notebookLocalIdsToRemove.size(); i < size; ++i) {
        const QString & localId = notebookLocalIdsToRemove[i];
        QNTRACE(
            "model:notebook",
            "Processing notebook local id " << localId
                                             << " scheduled for removal");

        auto notebookItemIt = localIdIndex.find(localId);
        if (Q_UNLIKELY(notebookItemIt == localIdIndex.end())) {
            QNWARNING(
                "model:notebook",
                "Internal error detected: can't find "
                    << "the notebook model item corresponding to the notebook "
                    << "item being removed: local id = " << localId);
            continue;
        }

        auto * pModelItem = const_cast<NotebookItem *>(&(*notebookItemIt));
        QNTRACE(
            "model:notebook",
            "Model item corresponding to the notebook: " << *pModelItem);

        auto * pParentItem = pModelItem->parent();
        QNTRACE(
            "model:notebook",
            "Notebook's parent item at "
                << pParentItem << ": "
                << (pParentItem ? pParentItem->toString()
                                : QStringLiteral("<null>")));

        removeModelItemFromParent(*pModelItem, RemoveEmptyParentStack::No);

        QNTRACE(
            "model:notebook",
            "Model item corresponding to the notebook "
                << "after parent removal: " << *pModelItem
                << "\nParent item after removing the child "
                << "notebook item from it: " << *pParentItem);

        Q_UNUSED(localIdIndex.erase(notebookItemIt))

        QNTRACE(
            "model:notebook",
            "Erased the notebook item corresponding to "
                << "local id " << localId);

        const auto indexIt = m_indexIdToLocalIdBimap.right.find(localId);
        if (indexIt != m_indexIdToLocalIdBimap.right.end()) {
            m_indexIdToLocalIdBimap.right.erase(indexIt);
        }

        expungeNotebookFromLocalStorage(localId);

        if (!pParentItem) {
            continue;
        }

        auto * pParentStackItem = pParentItem->cast<StackItem>();
        if (!pParentStackItem) {
            continue;
        }

        if (pParentStackItem->childrenCount() != 0) {
            continue;
        }

        QNDEBUG(
            "model:notebook",
            "The last child was removed from "
                << "the stack item, need to remove it as well: stack = "
                << pParentStackItem->name());

        QString linkedNotebookGuid;
        auto * pGrandParentItem = pParentItem->parent();
        if (pGrandParentItem) {
            auto * pLinkedNotebookItem =
                pGrandParentItem->cast<LinkedNotebookRootItem>();

            if (pLinkedNotebookItem) {
                linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();
            }
        }

        stacksToRemoveWithLinkedNotebookGuids.push_back(
            std::pair<QString, QString>(
                pParentStackItem->name(), linkedNotebookGuid));

        QNTRACE(
            "model:notebook",
            "Marked notebook stack " << pParentStackItem->name()
                                     << " as the one scheduled for removal");
    }

    QNTRACE(
        "model:notebook",
        "Finished removing notebook items, switching "
            << "to removing notebook stack items (if any)");

    for (const auto & pair: stacksToRemoveWithLinkedNotebookGuids)
    {
        const QString & stack = pair.first;
        const QString & linkedNotebookGuid = pair.second;

        QNTRACE(
            "model:notebook",
            "Processing notebook stack scheduled for "
                << "removal: " << stack
                << ", linked notebook guid = " << linkedNotebookGuid);

        auto & stackItems =
            (linkedNotebookGuid.isEmpty()
                 ? m_stackItems
                 : m_stackItemsByLinkedNotebookGuid[linkedNotebookGuid]);

        const auto stackItemIt = stackItems.find(stack);
        if (Q_UNLIKELY(stackItemIt == stackItems.end())) {
            QNWARNING(
                "model:notebook",
                "Internal error detected: can't find "
                    << "the notebook stack item being removed "
                    << "from the notebook model: stack = " << stack
                    << ", linked notebook guid = " << linkedNotebookGuid);
            continue;
        }

        Q_UNUSED(stackItems.erase(stackItemIt))
        QNTRACE(
            "model:notebook",
            "Erased the notebook stack item corresponding to stack " << stack
                << ", linked notebook guid = " << linkedNotebookGuid);
    }

    QNTRACE(
        "model:notebook",
        "Successfully removed row(s) from the notebook model");
    return true;
}

void NotebookModel::sort(int column, Qt::SortOrder order)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (column != static_cast<int>(Column::Name)) {
        QNDEBUG(
            "model:notebook",
            "Only sorting by name is implemented at "
                << "this time");
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(
            "model:notebook",
            "The sort order already established, "
                << "nothing to do");
        return;
    }

    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    auto & localIdIndex = m_data.get<ByLocalId>();
    for (const auto & item: localIdIndex) {
        updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
    }

    for (auto it = m_stackItems.begin(), end = m_stackItems.end(); it != end;
         ++it) {
        updateItemRowWithRespectToSorting(const_cast<StackItem &>(it.value()));
    }

    for (auto it = m_linkedNotebookItems.begin(),
              end = m_linkedNotebookItems.end();
         it != end; ++it)
    {
        updateItemRowWithRespectToSorting(
            const_cast<LinkedNotebookRootItem &>(it.value()));
    }

    for (auto it = m_stackItemsByLinkedNotebookGuid.begin(),
              end = m_stackItemsByLinkedNotebookGuid.end();
         it != end; ++it)
    {
        StackItems & stackItems = it.value();
        for (auto sit = stackItems.begin(), send = stackItems.end();
             sit != send; ++sit) {
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
    list << NOTEBOOK_MODEL_MIME_TYPE;
    return list;
}

QMimeData * NotebookModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    auto * pItem = itemForIndex(indexes.at(0));
    if (!pItem) {
        return nullptr;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *pItem;

    auto * pMimeData = new QMimeData;
    pMimeData->setData(
        NOTEBOOK_MODEL_MIME_TYPE,
        qCompress(encodedItem, NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION));

    return pMimeData;
}

bool NotebookModel::dropMimeData(
    const QMimeData * pMimeData, Qt::DropAction action, int row, int column,
    const QModelIndex & parentIndex)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::dropMimeData: action = "
            << action << ", row = " << row << ", column = " << column
            << ", parent index: is valid = "
            << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row()
            << ", parent column = " << (parentIndex.column())
            << ", internal id = " << parentIndex.internalId()
            << ", mime data formats: "
            << (pMimeData ? pMimeData->formats().join(QStringLiteral("; "))
                          : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!pMimeData || !pMimeData->hasFormat(NOTEBOOK_MODEL_MIME_TYPE)) {
        return false;
    }

    checkAndCreateModelRootItems();

    auto * pNewParentItem = itemForIndex(parentIndex);
    if (!pNewParentItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error, can't drop notebook: no new parent "
                       "item was found"));
        return false;
    }

    if (pNewParentItem == m_pInvisibleRootItem) {
        QNDEBUG(
            "model:notebook",
            "Can't drop notebook model items directly "
                << "onto invisible root item");
        return false;
    }

    if ((pNewParentItem != m_pAllNotebooksRootItem) &&
        (pNewParentItem->type() != INotebookModelItem::Type::Stack))
    {
        ErrorString error(
            QT_TR_NOOP("Can't drop the notebook onto another notebook"));
        QNINFO("model:notebook", error);
        Q_EMIT notifyError(error);
        return false;
    }

    QByteArray data = qUncompress(pMimeData->data(NOTEBOOK_MODEL_MIME_TYPE));
    QDataStream in(&data, QIODevice::ReadOnly);

    qint32 type = 0;
    in >> type;

    if (type != static_cast<qint32>(INotebookModelItem::Type::Notebook)) {
        QNDEBUG(
            "model:notebook",
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

    auto * pParentLinkedNotebookItem =
        pNewParentItem->cast<LinkedNotebookRootItem>();

    if (pParentLinkedNotebookItem) {
        parentLinkedNotebookGuid =
            pParentLinkedNotebookItem->linkedNotebookGuid();
    }
    else if (pNewParentItem->type() == INotebookModelItem::Type::Stack) {
        pParentLinkedNotebookItem =
            pNewParentItem->parent()->cast<LinkedNotebookRootItem>();

        if (pParentLinkedNotebookItem) {
            parentLinkedNotebookGuid =
                pParentLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    if (notebookItem.linkedNotebookGuid() != parentLinkedNotebookGuid) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move notebooks between different linked "
                       "notebooks or between user's notebooks "
                       "and those from linked notebooks"));
        return false;
    }

    auto * pOriginalParentItem = it->parent();
    int originalRow = -1;
    if (pOriginalParentItem) {
        // Need to manually remove the item from its original parent
        originalRow = pOriginalParentItem->rowForChild(&(*it));
    }

    if (originalRow >= 0) {
        QModelIndex originalParentIndex = indexForItem(pOriginalParentItem);
        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(pOriginalParentItem->takeChild(originalRow))
        endRemoveRows();
    }

    Q_UNUSED(localIdIndex.erase(it))

    beginInsertRows(parentIndex, row, row);

    auto * pNewParentStackItem = pNewParentItem->cast<StackItem>();

    notebookItem.setStack(
        (pNewParentItem == m_pAllNotebooksRootItem || !pNewParentStackItem)
            ? QString()
            : pNewParentStackItem->name());

    notebookItem.setDirty(true);

    const auto insertionResult = localIdIndex.insert(notebookItem);
    const auto notebookItemIt = insertionResult.first;

    auto * pModelItem = const_cast<NotebookItem *>(&(*notebookItemIt));

    pNewParentItem->insertChild(row, pModelItem);

    endInsertRows();

    updateItemRowWithRespectToSorting(*pModelItem);
    updateNotebookInLocalStorage(notebookItem);
    return true;
}

void NotebookModel::onAddNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    const auto it = m_addNotebookRequestIds.find(requestId);
    if (it != m_addNotebookRequestIds.end()) {
        Q_UNUSED(m_addNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onAddNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_addNotebookRequestIds.find(requestId);
    if (it == m_addNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onAddNotebookFailed: notebook = " << notebook
            << "\nError description = " << errorDescription << ", request id = "
            << requestId);

    Q_UNUSED(m_addNotebookRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);

    removeItemByLocalId(notebook.localId());
}

void NotebookModel::onUpdateNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onUpdateNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    const auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void NotebookModel::onUpdateNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onUpdateNotebookFailed: notebook = " << notebook
            << "\nError description = " << errorDescription << ", request id = "
            << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:notebook",
        "Emitting the request to find the notebook: "
            << "local id = " << notebook.localId()
            << ", request id = " << requestId);

    Q_EMIT findNotebook(notebook, requestId);
}

void NotebookModel::onFindNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    const auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
         ? m_findNotebookToPerformUpdateRequestIds.find(requestId)
         : m_findNotebookToPerformUpdateRequestIds.end());

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onFindNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))

        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(notebook.localId(), notebook);

        auto & localIdIndex = m_data.get<ByLocalId>();
        const auto it = localIdIndex.find(notebook.localId());
        if (it != localIdIndex.end()) {
            updateNotebookInLocalStorage(*it);
        }
    }
}

void NotebookModel::onFindNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()
         ? m_findNotebookToPerformUpdateRequestIds.find(requestId)
         : m_findNotebookToPerformUpdateRequestIds.end());

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onFindNotebookFailed: notebook = " << notebook
            << "\nError description = " << errorDescription << ", request id = "
            << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void NotebookModel::onListNotebooksComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, // NOLINT
    QList<qevercloud::Notebook> foundNotebooks, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onListNotebooksComplete: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", num found notebooks = " << foundNotebooks.size()
            << ", request id = " << requestId);

    for (const auto & notebook: qAsConst(foundNotebooks)) {
        onNotebookAddedOrUpdated(notebook);
        requestNoteCountForNotebook(notebook);
    }

    m_listNotebooksRequestId = QUuid();

    if (!foundNotebooks.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "The number of found notebooks is not empty, "
                << "requesting more notebooks from the local storage");
        m_listNotebooksOffset += static_cast<size_t>(foundNotebooks.size());
        requestNotebooksList();
        return;
    }

    m_allNotebooksListed = true;

    if (m_allLinkedNotebooksListed) {
        Q_EMIT notifyAllNotebooksListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void NotebookModel::onListNotebooksFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onListNotebooksFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listNotebooksRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void NotebookModel::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    const auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it != m_expungeNotebookRequestIds.end()) {
        Q_UNUSED(m_expungeNotebookRequestIds.erase(it))
        return;
    }

    Q_EMIT aboutToRemoveNotebooks();
    removeItemByLocalId(notebook.localId());
    Q_EMIT removedNotebooks();
}

void NotebookModel::onExpungeNotebookFailed(
    qevercloud::Notebook notebook, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it == m_expungeNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onExpungeNotebookFailed: notebook = " << notebook
            << "\nError description = " << errorDescription << ", request id = "
            << requestId);

    Q_UNUSED(m_expungeNotebookRequestIds.erase(it))

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onGetNoteCountPerNotebookComplete(
    int noteCount, qevercloud::Notebook notebook, // NOLINT
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it = m_noteCountPerNotebookRequestIds.find(requestId);
    if (it == m_noteCountPerNotebookRequestIds.end()) {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onGetNoteCountPerNotebookComplete: "
            << "note count = " << noteCount << ", notebook = " << notebook
            << "\nRequest id = " << requestId);

    Q_UNUSED(m_noteCountPerNotebookRequestIds.erase(it))

    const QString & notebookLocalId = notebook.localId();

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebookLocalId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Can't find the notebook item by local id: " << notebookLocalId);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNoteCount(noteCount);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onGetNoteCountPerNotebookFailed(
    ErrorString errorDescription, qevercloud::Notebook notebook, // NOLINT
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    const auto it = m_noteCountPerNotebookRequestIds.find(requestId);
    if (it == m_noteCountPerNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onGetNoteCountPerNotebookFailed: "
            << "error description = " << errorDescription
            << ", notebook: " << notebook << "\nRequest id = " << requestId);

    Q_UNUSED(m_noteCountPerNotebookRequestIds.erase(it))

    // Not much can be done here - will just attempt ot "remove" the count from
    // the item

    const QString & notebookLocalId = notebook.localId();

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebookLocalId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Can't find the notebook item by local id: " << notebookLocalId);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNoteCount(-1);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onAddNoteComplete(
    qevercloud::Note note, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddNoteComplete: note = " << note << ", request id = "
                                                    << requestId);

    if (Q_UNLIKELY(note.deleted())) {
        return;
    }

    const auto notebookLocalId = note.notebookLocalId();
    if (!notebookLocalId.isEmpty() &&
        incrementNoteCountForNotebook(notebookLocalId))
    {
        return;
    }

    qevercloud::Notebook notebook;
    if (!notebookLocalId.isEmpty()) {
        notebook.setLocalId(notebookLocalId);
    }
    else if (note.notebookGuid()) {
        notebook.setGuid(*note.notebookGuid());
    }
    else {
        QNDEBUG(
            "model:notebook",
            "Added note has no notebook local id and no notebook guid, "
                << "re-requesting the note count for all notebooks");
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onNoteMovedToAnotherNotebook(
    QString noteLocalId, QString previousNotebookLocalId, // NOLINT
    QString newNotebookLocalId) // NOLINT
{
    QNDEBUG(
        "model:notebook",
        "NotebookModel::onNoteMovedToAnotherNotebook: "
            << "note local id = " << noteLocalId
            << ", previous notebook local id = " << previousNotebookLocalId
            << ", new notebook local id = " << newNotebookLocalId);

    Q_UNUSED(decrementNoteCountForNotebook(previousNotebookLocalId))
    Q_UNUSED(incrementNoteCountForNotebook(newNotebookLocalId))
}

void NotebookModel::onExpungeNoteComplete(
    qevercloud::Note note, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeNoteComplete: note = "
            << note << "\nRequest id = " << requestId);

    const auto notebookLocalId = note.notebookLocalId();

    // NOTE: it's not sufficient to decrement note count for notebook
    // as this note might have had deletion timestamp set so it did not
    // actually contribute to the displayed note count for this notebook

    qevercloud::Notebook notebook;
    if (!notebookLocalId.isEmpty()) {
        notebook.setLocalId(notebookLocalId);
    }
    else if (note.notebookGuid()) {
        notebook.setGuid(*note.notebookGuid());
    }
    else {
        QNDEBUG(
            "model:notebook",
            "Expunged note has no notebook local id and no notebook guid, "
                << "re-requesting the note count for all notebooks");
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onAddLinkedNotebookComplete(
    qevercloud::LinkedNotebook linkedNotebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void NotebookModel::onUpdateLinkedNotebookComplete(
    qevercloud::LinkedNotebook linkedNotebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onUpdateLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void NotebookModel::onExpungeLinkedNotebookComplete(
    qevercloud::LinkedNotebook linkedNotebook, QUuid requestId) // NOLINT
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "model:notebook",
            "Received linked notebook expunged event "
                << "but the linked notebook has no guid: " << linkedNotebook
                << ", request id = " << requestId);
        return;
    }

    const QString & linkedNotebookGuid = *linkedNotebook.guid();
    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);

    QStringList expungedNotebookLocalIds;
    expungedNotebookLocalIds.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedNotebookLocalIds << it->localId();
    }

    for (auto it = expungedNotebookLocalIds.constBegin(),
              end = expungedNotebookLocalIds.constEnd();
         it != end; ++it)
    {
        removeItemByLocalId(*it);
    }

    const auto linkedNotebookItemIt =
        m_linkedNotebookItems.find(linkedNotebookGuid);

    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        auto * pModelItem = &(linkedNotebookItemIt.value());
        auto * pParentItem = pModelItem->parent();
        if (pParentItem) {
            int row = pParentItem->rowForChild(pModelItem);
            if (row >= 0) {
                QModelIndex parentItemIndex = indexForItem(pParentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(pParentItem->takeChild(row))
                endRemoveRows();
            }
        }

        Q_UNUSED(m_linkedNotebookItems.erase(linkedNotebookItemIt))
    }

    const auto stackItemsIt =
        m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (stackItemsIt != m_stackItemsByLinkedNotebookGuid.end()) {
        Q_UNUSED(m_stackItemsByLinkedNotebookGuid.erase(stackItemsIt))
    }

    const auto indexIt =
        m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);

    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }
}

void NotebookModel::onListAllLinkedNotebooksComplete(
    size_t limit, size_t offset,
    LocalStorageManager::ListLinkedNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QList<qevercloud::LinkedNotebook> foundLinkedNotebooks, // NOLINT
    QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onListAllLinkedNotebooksComplete: "
            << "limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", order direction = " << orderDirection
            << ", request id = " << requestId);

    for (auto it = foundLinkedNotebooks.constBegin(),
              end = foundLinkedNotebooks.constEnd();
         it != end; ++it)
    {
        onLinkedNotebookAddedOrUpdated(*it);
    }

    m_listLinkedNotebooksRequestId = QUuid();

    if (!foundLinkedNotebooks.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "The number of found linked notebooks is not "
                << "empty, requesting more linked notebooks from the local "
                << "storage");

        m_listLinkedNotebooksOffset +=
            static_cast<size_t>(foundLinkedNotebooks.size());

        requestLinkedNotebooksList();
        return;
    }

    m_allLinkedNotebooksListed = true;

    if (m_allNotebooksListed) {
        Q_EMIT notifyAllNotebooksListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void NotebookModel::onListAllLinkedNotebooksFailed(
    size_t limit, size_t offset,
    LocalStorageManager::ListLinkedNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId) // NOLINT
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onListAllLinkedNotebooksFailed: "
            << "limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", order direction = " << orderDirection
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listLinkedNotebooksRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void NotebookModel::createConnections( // NOLINT
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNTRACE("model:notebook", "NotebookModel::createConnections");

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(
        this, &NotebookModel::addNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddNotebookRequest);

    QObject::connect(
        this, &NotebookModel::updateNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateNotebookRequest);

    QObject::connect(
        this, &NotebookModel::findNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNotebookRequest);

    QObject::connect(
        this, &NotebookModel::listNotebooks, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListNotebooksRequest);

    QObject::connect(
        this, &NotebookModel::expungeNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onExpungeNotebookRequest);

    QObject::connect(
        this, &NotebookModel::requestNoteCountPerNotebook,
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::onGetNoteCountPerNotebookRequest);

    QObject::connect(
        this, &NotebookModel::listAllLinkedNotebooks, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListAllLinkedNotebooksRequest);

    // localStorageManagerAsync's signals to local slots
    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addNotebookComplete, this,
        &NotebookModel::onAddNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addNotebookFailed,
        this, &NotebookModel::onAddNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete, this,
        &NotebookModel::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookFailed, this,
        &NotebookModel::onUpdateNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookComplete, this,
        &NotebookModel::onFindNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed, this,
        &NotebookModel::onFindNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksComplete, this,
        &NotebookModel::onListNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listNotebooksFailed, this,
        &NotebookModel::onListNotebooksFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &NotebookModel::onExpungeNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookFailed, this,
        &NotebookModel::onExpungeNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &NotebookModel::onAddNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::noteMovedToAnotherNotebook, this,
        &NotebookModel::onNoteMovedToAnotherNotebook);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete, this,
        &NotebookModel::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerNotebookComplete, this,
        &NotebookModel::onGetNoteCountPerNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerNotebookFailed, this,
        &NotebookModel::onGetNoteCountPerNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addLinkedNotebookComplete, this,
        &NotebookModel::onAddLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateLinkedNotebookComplete, this,
        &NotebookModel::onUpdateLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeLinkedNotebookComplete, this,
        &NotebookModel::onExpungeLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listAllLinkedNotebooksComplete, this,
        &NotebookModel::onListAllLinkedNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listAllLinkedNotebooksFailed, this,
        &NotebookModel::onListAllLinkedNotebooksFailed);
}

void NotebookModel::requestNotebooksList()
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::requestNotebooksList: offset = "
            << m_listNotebooksOffset);

    const LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    const auto order = LocalStorageManager::ListNotebooksOrder::NoOrder;
    const auto direction = LocalStorageManager::OrderDirection::Ascending;
    m_listNotebooksRequestId = QUuid::createUuid();

    QNTRACE(
        "model:notebook",
        "Emitting the request to list notebooks: "
            << "offset = " << m_listNotebooksOffset
            << ", request id = " << m_listNotebooksRequestId);

    Q_EMIT listNotebooks(
        flags, NOTEBOOK_LIST_LIMIT, m_listNotebooksOffset, order, direction, {},
        m_listNotebooksRequestId);
}

void NotebookModel::requestNoteCountForNotebook(
    const qevercloud::Notebook & notebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::requestNoteCountForNotebook: " << notebook);

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerNotebookRequestIds.insert(requestId))
    QNTRACE(
        "model:notebook",
        "Emitting request to get the note count per "
            << "notebook: request id = " << requestId);

    const LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountPerNotebook(notebook, options, requestId);
}

void NotebookModel::requestNoteCountForAllNotebooks()
{
    QNTRACE("model:notebook", "NotebookModel::requestNoteCountForAllNotebooks");

    const auto & localIdIndex = m_data.get<ByLocalId>();
    for (const auto & item: localIdIndex) {
        qevercloud::Notebook notebook;
        notebook.setLocalId(item.localId());
        requestNoteCountForNotebook(notebook);
    }
}

void NotebookModel::requestLinkedNotebooksList()
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::requestLinkedNotebooksList: "
            << "offset = " << m_listLinkedNotebooksOffset);

    const auto order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    const auto direction = LocalStorageManager::OrderDirection::Ascending;
    m_listLinkedNotebooksRequestId = QUuid::createUuid();

    QNTRACE(
        "model:notebook",
        "Emitting the request to list linked notebooks: "
            << "offset = " << m_listLinkedNotebooksOffset
            << ", request id = " << m_listLinkedNotebooksRequestId);

    Q_EMIT listAllLinkedNotebooks(
        LINKED_NOTEBOOK_LIST_LIMIT, m_listLinkedNotebooksOffset, order,
        direction, m_listLinkedNotebooksRequestId);
}

QVariant NotebookModel::dataImpl(
    const INotebookModelItem & item, const Column column) const
{
    if (&item == m_pAllNotebooksRootItem) {
        if (column == Column::Name) {
            return tr("All notebooks");
        }

        return {};
    }

    const auto * pNotebookItem = item.cast<NotebookItem>();
    if (pNotebookItem) {
        return notebookData(*pNotebookItem, column);
    }

    const auto * pStackItem = item.cast<StackItem>();
    if (pStackItem) {
        return stackData(*pStackItem, column);
    }

    return {};
}

QVariant NotebookModel::dataAccessibleText(
    const INotebookModelItem & item, const Column column) const
{
    if (&item == m_pAllNotebooksRootItem) {
        if (column == Column::Name) {
            return tr("All notebooks");
        }

        return {};
    }

    const auto * pNotebookItem = item.cast<NotebookItem>();
    if (pNotebookItem) {
        return notebookAccessibleData(*pNotebookItem, column);
    }

    const auto * pStackItem = item.cast<StackItem>();
    if (pStackItem) {
        return stackAccessibleData(*pStackItem, column);
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
        "model:notebook",
        "NotebookModel::updateNotebookInLocalStorage: "
            << "local id = " << item.localId());

    qevercloud::Notebook notebook;

    const auto notYetSavedItemIt =
        m_notebookItemsNotYetInLocalStorageUids.find(item.localId());

    if (notYetSavedItemIt == m_notebookItemsNotYetInLocalStorageUids.end()) {
        QNDEBUG("model:notebook", "Updating the notebook");

        const auto * pCachedNotebook = m_cache.get(item.localId());
        if (Q_UNLIKELY(!pCachedNotebook)) {
            const QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))

            qevercloud::Notebook dummy;
            dummy.setLocalId(item.localId());
            Q_EMIT findNotebook(dummy, requestId);

            QNTRACE(
                "model:notebook",
                "Emitted the request to find "
                    << "the notebook: local id = " << item.localId()
                    << ", request id = " << requestId);
            return;
        }

        notebook = *pCachedNotebook;
    }

    notebook.setLocalId(item.localId());
    notebook.setGuid(item.guid());
    notebook.setLinkedNotebookGuid(item.linkedNotebookGuid());
    notebook.setName(item.name());
    notebook.setLocalOnly(!item.isSynchronizable());
    notebook.setLocallyModified(item.isDirty());
    notebook.setDefaultNotebook(item.isDefault());
    notebook.setPublished(item.isPublished());
    notebook.setLocallyFavorited(item.isFavorited());
    notebook.setStack(item.stack());

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
    }

    m_cache.put(item.localId(), notebook);

    // NOTE: deliberately not setting the updatable property from the item
    // as it can't be changed by the model and only serves the utilitary
    // purposes

    const auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_notebookItemsNotYetInLocalStorageUids.end()) {
        Q_UNUSED(m_addNotebookRequestIds.insert(requestId))

        QNTRACE(
            "model:notebook",
            "Emitting the request to add the notebook "
                << "to the local storage: id = " << requestId
                << ", notebook = " << notebook);

        Q_EMIT addNotebook(notebook, requestId);

        Q_UNUSED(
            m_notebookItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else {
        Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

        // While the notebook is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(notebook.localId()))

        QNTRACE(
            "model:notebook",
            "Emitting the request to update notebook in "
                << "the local storage: id = " << requestId
                << ", notebook = " << notebook);

        Q_EMIT updateNotebook(notebook, requestId);
    }
}

void NotebookModel::expungeNotebookFromLocalStorage(const QString & localId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::expungeNotebookFromLocalStorage: "
            << "local id = " << localId);

    qevercloud::Notebook dummyNotebook;
    dummyNotebook.setLocalId(localId);

    Q_UNUSED(m_cache.remove(localId))

    const auto requestId = QUuid::createUuid();
    Q_UNUSED(m_expungeNotebookRequestIds.insert(requestId))

    QNDEBUG(
        "model:notebook",
        "Emitting the request to expunge the notebook "
            << "from the local storage: request id = " << requestId
            << ", local id = " << localId);

    Q_EMIT expungeNotebook(dummyNotebook, requestId);
}

QString NotebookModel::nameForNewNotebook() const
{
    const QString baseName = tr("New notebook");
    const auto & nameIndex = m_data.get<ByNameUpper>();

    return newItemName(nameIndex, m_lastNewNotebookNameCounter, baseName);
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

void NotebookModel::onNotebookAddedOrUpdated(
    const qevercloud::Notebook & notebook)
{
    m_cache.put(notebook.localId(), notebook);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(notebook.localId());
    const bool newNotebook = (itemIt == localIdIndex.end());
    if (newNotebook) {
        Q_EMIT aboutToAddNotebook();

        onNotebookAdded(notebook);

        const auto addedNotebookIndex = indexForLocalId(notebook.localId());
        Q_EMIT addedNotebook(addedNotebookIndex);
    }
    else {
        const auto notebookIndexBefore = indexForLocalId(notebook.localId());
        Q_EMIT aboutToUpdateNotebook(notebookIndexBefore);

        onNotebookUpdated(notebook, itemIt);

        const auto notebookIndexAfter = indexForLocalId(notebook.localId());
        Q_EMIT updatedNotebook(notebookIndexAfter);
    }
}

void NotebookModel::onNotebookAdded(const qevercloud::Notebook & notebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onNotebookAdded: notebook "
            << "local id = " << notebook.localId());

    checkAndCreateModelRootItems();

    INotebookModelItem * pParentItem = nullptr;
    if (notebook.stack()) {
        auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.linkedNotebookGuid() ? *notebook.linkedNotebookGuid()
                                          : QString{});

        auto * pStackItems = stackItemsWithParentPair.first;
        auto * pGrandParentItem = stackItemsWithParentPair.second;
        const QString & stack = *notebook.stack();

        pParentItem =
            &(findOrCreateStackItem(stack, *pStackItems, pGrandParentItem));
    }
    else if (notebook.linkedNotebookGuid()) {
        pParentItem = &(findOrCreateLinkedNotebookModelItem(
            *notebook.linkedNotebookGuid()));
    }
    else {
        pParentItem = m_pAllNotebooksRootItem;
    }

    const auto parentIndex = indexForItem(pParentItem);

    NotebookItem item;
    notebookToItem(notebook, item);

    const int row = pParentItem->childrenCount();

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto insertionResult = localIdIndex.insert(item);
    auto * pInsertedItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    pInsertedItem->setParent(pParentItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*pInsertedItem);
}

void NotebookModel::onNotebookUpdated(
    const qevercloud::Notebook & notebook, NotebookDataByLocalId::iterator it)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onNotebookUpdated: notebook "
            << "local id = " << notebook.localId());

    auto * pModelItem = const_cast<NotebookItem *>(&(*it));
    auto * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(
            "model:notebook",
            "Can't find the parent notebook model item "
                << "for updated notebook item: " << *pModelItem);
        return;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the row of the child notebook "
                << "model item within its parent model item: parent item = "
                << *pParentItem << "\nChild item = " << *pModelItem);
        return;
    }

    bool shouldChangeParent = false;

    QString previousStackName;
    auto * pParentStackItem = pParentItem->cast<StackItem>();
    if (pParentStackItem) {
        previousStackName = pParentStackItem->name();
    }

    if (!notebook.stack() && !previousStackName.isEmpty()) {
        // Need to remove the notebook model item from the current stack and
        // set proper root item as the parent
        const auto parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        if (!notebook.linkedNotebookGuid()) {
            checkAndCreateModelRootItems();
            pParentItem = m_pAllNotebooksRootItem;
        }
        else {
            pParentItem = &(findOrCreateLinkedNotebookModelItem(
                *notebook.linkedNotebookGuid()));
        }

        shouldChangeParent = true;
    }
    else if (notebook.stack() && (*notebook.stack() != previousStackName)) {
        // Need to remove the notebook from its current parent and insert it
        // under the stack item, either existing or new
        QModelIndex parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        const auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.linkedNotebookGuid() ? *notebook.linkedNotebookGuid()
                                          : QString{});

        auto * pStackItems = stackItemsWithParentPair.first;
        auto * pGrandParentItem = stackItemsWithParentPair.second;

        pParentItem = &(findOrCreateStackItem(
            *notebook.stack(), *pStackItems, pGrandParentItem));

        shouldChangeParent = true;
    }

    bool notebookStackChanged = false;
    if (shouldChangeParent) {
        const auto parentItemIndex = indexForItem(pParentItem);
        const int newRow = pParentItem->childrenCount();
        beginInsertRows(parentItemIndex, newRow, newRow);
        pModelItem->setParent(pParentItem);
        endInsertRows();

        row = pParentItem->rowForChild(pModelItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model:notebook",
                "Can't find the row for the child "
                    << "notebook item within its parent right after setting "
                    << "the parent item to the child item! Parent item = "
                    << *pParentItem << "\nChild item = " << *pModelItem);
            return;
        }

        notebookStackChanged = true;
    }

    NotebookItem notebookItemCopy{*it};
    notebookToItem(notebook, notebookItemCopy);

    auto & localIdIndex = m_data.get<ByLocalId>();
    localIdIndex.replace(it, notebookItemCopy);

    const auto modelItemId = idForItem(*pModelItem);

    const auto modelIndexFrom = createIndex(row, 0, modelItemId);

    const QModelIndex modelIndexTo =
        createIndex(row, NUM_NOTEBOOK_MODEL_COLUMNS - 1, modelItemId);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model:notebook", "Not sorting by name, returning");
        return;
    }

    updateItemRowWithRespectToSorting(*pModelItem);

    if (notebookStackChanged) {
        Q_EMIT notifyNotebookStackChanged(modelIndexFrom);
    }
}

void NotebookModel::onLinkedNotebookAddedOrUpdated(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "model:notebook",
            "Can't process the addition or update of "
                << "a linked notebook without guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.username())) {
        QNWARNING(
            "model:notebook",
            "Can't process the addition or update of "
                << "a linked notebook without username: " << linkedNotebook);
        return;
    }

    const QString & linkedNotebookGuid = *linkedNotebook.guid();
    const auto & newUsername = *linkedNotebook.username();

    m_linkedNotebookUsernamesByGuids[linkedNotebookGuid] = newUsername;

    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it != m_linkedNotebookItems.end()) {
        auto & linkedNotebookItem = it.value();

        if (linkedNotebookItem.username() == newUsername) {
            QNDEBUG(
                "model:notebook",
                "The username hasn't changed, nothing to "
                    << "do");
            return;
        }

        linkedNotebookItem.setUsername(newUsername);
        QNDEBUG(
            "model:notebook",
            "Updated the username corresponding to "
                << "linked notebook guid " << linkedNotebookGuid << " to "
                << newUsername);

        const auto itemIndex = indexForItem(&linkedNotebookItem);
        Q_EMIT dataChanged(itemIndex, itemIndex);
        return;
    }

    QNDEBUG(
        "model:notebook",
        "Adding new linked notebook item: username "
            << newUsername << " corresponding to linked notebook guid "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    it = m_linkedNotebookItems.insert(
        linkedNotebookGuid,
        LinkedNotebookRootItem(newUsername, linkedNotebookGuid));

    auto & linkedNotebookItem = it.value();

    const auto parentIndex = indexForItem(m_pAllNotebooksRootItem);
    const int row = rowForNewItem(*m_pAllNotebooksRootItem, linkedNotebookItem);

    beginInsertRows(parentIndex, row, row);
    m_pAllNotebooksRootItem->insertChild(row, &linkedNotebookItem);
    endInsertRows();
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

    if ((column == static_cast<int>(Column::Synchronizable)) &&
        notebookItem.isSynchronizable())
    {
        return flags;
    }

    if ((column == static_cast<int>(Column::Name)) &&
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
            "model:notebook",
            "Notebook stack hasn't changed, nothing to "
                << "do");
        return true;
    }

    const auto children = stackItem.children();
    for (auto it = children.constBegin(), end = children.constEnd(); it != end;
         ++it)
    {
        auto * pChildItem = *it;
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(
                "model:notebook",
                "Detected null pointer within children "
                    << "of notebook stack item: " << stackItem);
            continue;
        }

        auto * pChildNotebookItem = pChildItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pChildNotebookItem)) {
            QNWARNING(
                "model:notebook",
                "Detected non-notebook item within "
                    << "children of notebook stack item: " << stackItem);
            continue;
        }

        if (!canUpdateNotebookItem(*pChildNotebookItem)) {
            ErrorString error(
                QT_TR_NOOP("Can't update the notebook stack: "
                           "restrictions on at least one "
                           "of stacked notebooks update apply"));

            error.details() = pChildNotebookItem->name();

            QNINFO(
                "model:notebook",
                error << ", notebook item for which "
                      << "the restrictions apply: " << *pChildNotebookItem);

            Q_EMIT notifyError(error);
            return false;
        }
    }

    auto * pParentItem = stackItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't update notebook stack, can't "
                       "find the parent of the notebook stack item"));

        QNWARNING(
            "model:notebook",
            error << ", previous stack: \"" << previousStack
                  << "\", new stack: \"" << newStack << "\"");

        Q_EMIT notifyError(error);
        return false;
    }

    QString linkedNotebookGuid;
    if ((pParentItem != m_pAllNotebooksRootItem) &&
        (pParentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        auto * pLinkedNotebookItem =
            pParentItem->cast<LinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    // Change the stack item
    StackItem newStackItem = stackItem;
    newStackItem.setName(newStack);

    // 1) Remove the previous stack item

    int stackItemRow = pParentItem->rowForChild(&stackItem);
    if (Q_UNLIKELY(stackItemRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't update the notebook stack item: can't find "
                       "the row of the stack item to be updated within "
                       "the parent item"));
        return false;
    }

    // Remove children from the stack model item about to be removed;
    // set the parent to the stack's parent item for now
    const auto parentItemIndex = indexForItem(pParentItem);
    for (auto it = children.constBegin(), end = children.constEnd(); it != end;
         ++it)
    {
        auto * pChildItem = *it;
        if (Q_UNLIKELY(!pChildItem)) {
            continue;
        }

        int row = stackItem.rowForChild(pChildItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model:notebook",
                "Couldn't find row for the child "
                    << "notebook item of the stack item about to be removed; "
                       "stack "
                    << "item: " << stackItem
                    << "\nChild item: " << *pChildItem);
            continue;
        }

        beginRemoveRows(modelIndex, row, row);
        Q_UNUSED(stackItem.takeChild(row))
        endRemoveRows();

        auto * pChildNotebookItem = pChildItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pChildNotebookItem)) {
            continue;
        }

        row = rowForNewItem(*pParentItem, *pChildItem);
        beginInsertRows(parentItemIndex, row, row);
        pParentItem->insertChild(row, pChildItem);
        endInsertRows();

        QNTRACE(
            "model:notebook",
            "Temporarily inserted the child notebook "
                << "item from removed stack item at the stack's parent item at "
                   "row "
                << row << ", child item: " << *pChildItem);
    }

    // As we've just reparented some items to the stack's parent item, need
    // to figure out the stack item's row again
    stackItemRow = pParentItem->rowForChild(&stackItem);
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
    Q_UNUSED(pParentItem->takeChild(stackItemRow))

    auto stackItemIt = m_stackItems.find(previousStack);
    if (stackItemIt != m_stackItems.end()) {
        Q_UNUSED(m_stackItems.erase(stackItemIt))
    }

    const auto indexIt = m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(
        std::make_pair(previousStack, linkedNotebookGuid));

    if (indexIt != m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
        m_indexIdToStackAndLinkedNotebookGuidBimap.right.erase(indexIt);
    }

    endRemoveRows();

    // 2) Insert the new stack item

    stackItemIt = m_stackItems.insert(newStack, newStackItem);
    stackItemRow = rowForNewItem(*pParentItem, *stackItemIt);

    const auto stackItemIndex =
        index(stackItemRow, static_cast<int>(Column::Name), parentItemIndex);

    beginInsertRows(parentItemIndex, stackItemRow, stackItemRow);
    pParentItem->insertChild(stackItemRow, &(*stackItemIt));
    endInsertRows();

    // 3) Move all children of the previous stack items to the new one
    auto & localIdIndex = m_data.get<ByLocalId>();

    for (auto it = children.constBegin(), end = children.constEnd(); it != end;
         ++it)
    {
        auto * pChildItem = *it;
        if (Q_UNLIKELY(!pChildItem)) {
            continue;
        }

        auto * pChildNotebookItem = pChildItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pChildNotebookItem)) {
            continue;
        }

        const auto notebookItemIt =
            localIdIndex.find(pChildNotebookItem->localId());
        if (notebookItemIt == localIdIndex.end()) {
            QNWARNING(
                "model:notebook",
                "Internal inconsistency detected: "
                    << "can't find the iterator for the notebook item for "
                       "which "
                    << "the stack it being changed: non-found notebook item: "
                    << *pChildNotebookItem);
            continue;
        }

        int row = pParentItem->rowForChild(pChildItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(
                "model:notebook",
                "Internal error: can't find the row of "
                    << "one of the removed stack item's children within "
                    << "the stack's parent item to which they have been "
                    << "temporarily moved; stack's parent item: "
                    << *pParentItem << "\nChild item: " << *pChildItem);
            continue;
        }

        beginRemoveRows(parentItemIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        row = rowForNewItem(*stackItemIt, *pChildItem);
        beginInsertRows(stackItemIndex, row, row);
        stackItemIt->insertChild(row, pChildItem);
        endInsertRows();

        // Also update the stack within the notebook item
        NotebookItem notebookItemCopy{*pChildNotebookItem};
        notebookItemCopy.setStack(newStack);

        const bool wasDirty = notebookItemCopy.isDirty();
        notebookItemCopy.setDirty(true);

        localIdIndex.replace(notebookItemIt, notebookItemCopy);

        auto notebookItemIndex =
            indexForLocalId(pChildNotebookItem->localId());

        QNTRACE("model:notebook", "Emitting the data changed signal");
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

    return QVariant(accessibleText);
}

const NotebookModel::StackItems * NotebookModel::stackItems(
    const QString & linkedNotebookGuid) const
{
    if (linkedNotebookGuid.isEmpty()) {
        return &m_stackItems;
    }

    const auto it = m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (it != m_stackItemsByLinkedNotebookGuid.end()) {
        return &(it.value());
    }

    return nullptr;
}

std::pair<NotebookModel::StackItems *, INotebookModelItem *>
NotebookModel::stackItemsWithParent(const QString & linkedNotebookGuid)
{
    if (linkedNotebookGuid.isEmpty()) {
        checkAndCreateModelRootItems();
        return std::make_pair(&m_stackItems, m_pAllNotebooksRootItem);
    }

    auto * pLinkedNotebookRootItem =
        &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));

    auto stackItemsIt =
        m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (Q_UNLIKELY(stackItemsIt == m_stackItemsByLinkedNotebookGuid.end())) {
        stackItemsIt = m_stackItemsByLinkedNotebookGuid.insert(
            linkedNotebookGuid, StackItems());
    }

    return std::make_pair(&(*stackItemsIt), pLinkedNotebookRootItem);
}

StackItem & NotebookModel::findOrCreateStackItem(
    const QString & stack, StackItems & stackItems,
    INotebookModelItem * pParentItem)
{
    auto it = stackItems.find(stack);
    if (it == stackItems.end()) {
        it = stackItems.insert(stack, StackItem(stack));
        auto & stackItem = it.value();

        const int row = rowForNewItem(*pParentItem, stackItem);
        const auto parentIndex = indexForItem(pParentItem);

        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, &stackItem);
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
    const auto children = stackItem.children();
    for (auto * pChildItem: children) {
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(
                "model:notebook",
                "Detected null pointer to notebook model item within "
                    << "the children of another notebook model item");
            continue;
        }

        auto * pChildNotebookItem = pChildItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pChildNotebookItem)) {
            QNWARNING(
                "model:notebook",
                "Detected nested notebook model item "
                    << "of non-notebook type (" << pChildItem->type()
                    << "); it is unexpected and most likely incorrect");
            continue;
        }

        if ((column == static_cast<int>(Column::Synchronizable)) &&
            pChildNotebookItem->isSynchronizable())
        {
            return flags;
        }

        if ((column == static_cast<int>(Column::Name)) &&
            !canUpdateNotebookItem(*pChildNotebookItem))
        {
            return flags;
        }
    }

    if (column == static_cast<int>(Column::Name)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

void NotebookModel::removeItemByLocalId(const QString & localId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::removeItemByLocalId: "
            << "local id = " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Can't find item to remove from the notebook "
                << "model");
        return;
    }

    auto * pModelItem = const_cast<NotebookItem *>(&(*itemIt));
    auto * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(
            "model:notebook",
            "Can't find the parent notebook model item "
                << "for the notebook being removed from the model: local id = "
                << pModelItem->localId());
        return;
    }

    const int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the notebook item's row within "
                << "its parent model item: notebook item = " << *pModelItem
                << "\nStack item = " << *pParentItem);
        return;
    }

    const auto parentItemModelIndex = indexForItem(pParentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    Q_UNUSED(localIdIndex.erase(itemIt))
    Q_UNUSED(m_cache.remove(localId))

    const auto indexIt = m_indexIdToLocalIdBimap.right.find(localId);
    if (indexIt != m_indexIdToLocalIdBimap.right.end()) {
        m_indexIdToLocalIdBimap.right.erase(indexIt);
    }

    checkAndRemoveEmptyStackItem(*pParentItem);
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

    const bool isDefaultNotebook =
        notebook.defaultNotebook() && *notebook.defaultNotebook();

    item.setDefault(isDefaultNotebook);
    if (isDefaultNotebook) {
        switchDefaultNotebookLocalId(notebook.localId());
    }

    item.setLastUsed(false);

    if (notebook.published()) {
        item.setPublished(*notebook.published());
    }

    if (notebook.restrictions()) {
        const auto & restrictions = *notebook.restrictions();

        item.setUpdatable(
            !restrictions.noUpdateNotebook() ||
            !*restrictions.noUpdateNotebook());

        item.setNameIsUpdatable(
            !restrictions.noRenameNotebook() ||
            !*restrictions.noRenameNotebook());

        item.setCanCreateNotes(
            !restrictions.noCreateNotes() ||
            !*restrictions.noCreateNotes());

        item.setCanUpdateNotes(
            !restrictions.noUpdateNotes() ||
            !*restrictions.noUpdateNotes());
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
    const RemoveEmptyParentStack option)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::removeModelItemFromParent: "
            << modelItem
            << "\nRemove empty parent stack = " << option);

    auto * pParentItem = modelItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNDEBUG("model:notebook", "No parent item, nothing to do");
        return;
    }

    QNTRACE("model:notebook", "Parent item: " << *pParentItem);
    const int row = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the child notebook model "
                << "item's row within its parent; child item = " << modelItem
                << ", parent item = " << *pParentItem);
        return;
    }

    QNTRACE("model:notebook", "Will remove the child at row " << row);

    const auto parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    if (option == RemoveEmptyParentStack::Yes) {
        checkAndRemoveEmptyStackItem(*pParentItem);
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
        "model:notebook", "NotebookModel::updateItemRowWithRespectToSorting");

    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    auto * pParentItem = modelItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QString linkedNotebookGuid;
        const auto * pNotebookItem = modelItem.cast<NotebookItem>();
        if (pNotebookItem) {
            linkedNotebookGuid = pNotebookItem->linkedNotebookGuid();
        }

        if (!linkedNotebookGuid.isEmpty()) {
            pParentItem =
                &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
        }
        else {
            checkAndCreateModelRootItems();
            pParentItem = m_pAllNotebooksRootItem;
        }

        const int row = rowForNewItem(*pParentItem, modelItem);

        beginInsertRows(indexForItem(pParentItem), row, row);
        pParentItem->insertChild(row, &modelItem);
        endInsertRows();
        return;
    }

    int currentItemRow = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't update notebook model item's row: "
                << "can't find its original row within parent: " << modelItem);
        return;
    }

    const auto parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(pParentItem->takeChild(currentItemRow))
    endRemoveRows();

    const int appropriateRow = rowForNewItem(*pParentItem, modelItem);

    beginInsertRows(parentIndex, appropriateRow, appropriateRow);
    pParentItem->insertChild(appropriateRow, &modelItem);
    endInsertRows();

    QNTRACE(
        "model:notebook",
        "Moved item from row " << currentItemRow << " to row " << appropriateRow
                               << "; item: " << modelItem);
}

void NotebookModel::updatePersistentModelIndices()
{
    QNTRACE("model:notebook", "NotebookModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    const auto indices = persistentIndexList();
    for (const auto & index: qAsConst(indices)) {
        auto * pItem = itemForId(static_cast<IndexId>(index.internalId()));
        auto replacementIndex = indexForItem(pItem);
        changePersistentIndex(index, replacementIndex);
    }
}

bool NotebookModel::incrementNoteCountForNotebook(
    const QString & notebookLocalId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::incrementNoteCountForNotebook: " << notebookLocalId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(notebookLocalId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Wasn't able to find the notebook item by "
                << "the specified local id");
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.noteCount();
    ++noteCount;
    item.setNoteCount(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

bool NotebookModel::decrementNoteCountForNotebook(
    const QString & notebookLocalId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::decrementNoteCountForNotebook: " << notebookLocalId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(notebookLocalId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Wasn't able to find the notebook item by "
                << "the specified local id");
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.noteCount();
    --noteCount;
    noteCount = std::max(noteCount, 0);
    item.setNoteCount(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

void NotebookModel::switchDefaultNotebookLocalId(const QString & localId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::switchDefaultNotebookLocalId: " << localId);

    if (!m_defaultNotebookLocalId.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "There has already been some notebook chosen "
                << "as the default, switching the default flag off for it");

        auto & localIdIndex = m_data.get<ByLocalId>();

        const auto previousDefaultItemIt =
            localIdIndex.find(m_defaultNotebookLocalId);

        if (previousDefaultItemIt != localIdIndex.end()) {
            NotebookItem previousDefaultItemCopy(*previousDefaultItemIt);
            QNTRACE(
                "model:notebook",
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
    QNTRACE("model:notebook", "Set default notebook local id to " << localId);
}

void NotebookModel::switchLastUsedNotebookLocalId(const QString & localId)
{
    QNTRACE(
        "model:notebook", "NotebookModel::setLastUsedNotebook: " << localId);

    if (!m_lastUsedNotebookLocalId.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "There has already been some notebook chosen "
                << "as the last used one, switching the last used flag off for "
                   "it");

        auto & localIdIndex = m_data.get<ByLocalId>();

        const auto previousLastUsedItemIt =
            localIdIndex.find(m_lastUsedNotebookLocalId);

        if (previousLastUsedItemIt != localIdIndex.end()) {
            NotebookItem previousLastUsedItemCopy(*previousLastUsedItemIt);
            QNTRACE(
                "model:notebook",
                "Previous last used notebook item: "
                    << previousLastUsedItemCopy);

            previousLastUsedItemCopy.setLastUsed(false);
            const bool wasDirty = previousLastUsedItemCopy.isDirty();
            previousLastUsedItemCopy.setDirty(true);

            localIdIndex.replace(
                previousLastUsedItemIt, previousLastUsedItemCopy);

            auto previousLastUsedItemIndex =
                indexForLocalId(m_lastUsedNotebookLocalId);

            Q_EMIT dataChanged(
                previousLastUsedItemIndex, previousLastUsedItemIndex);

            if (!wasDirty) {
                previousLastUsedItemIndex = index(
                    previousLastUsedItemIndex.row(),
                    static_cast<int>(Column::Dirty),
                    previousLastUsedItemIndex.parent());

                Q_EMIT dataChanged(
                    previousLastUsedItemIndex, previousLastUsedItemIndex);
            }

            updateNotebookInLocalStorage(previousLastUsedItemCopy);
        }
    }

    m_lastUsedNotebookLocalId = localId;

    QNTRACE(
        "model:notebook", "Set last used notebook local id to " << localId);
}

void NotebookModel::checkAndRemoveEmptyStackItem(INotebookModelItem & modelItem)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::checkAndRemoveEmptyStackItem: " << modelItem);

    if (&modelItem == m_pInvisibleRootItem) {
        QNDEBUG("model:notebook", "Won't remove the invisible root item");
        return;
    }

    if (&modelItem == m_pAllNotebooksRootItem) {
        QNDEBUG("model:notebook", "Won't remove the all notebooks root item");
        return;
    }

    auto * pStackItem = modelItem.cast<StackItem>();
    if (!pStackItem) {
        QNDEBUG(
            "model:notebook",
            "The model item is not of stack type, won't "
                << "remove it");
        return;
    }

    if (modelItem.childrenCount() != 0) {
        QNDEBUG("model:notebook", "The model item still has child items");
        return;
    }

    QNDEBUG(
        "model:notebook",
        "Removed the last child of the previous "
            << "notebook's stack, need to remove the stack item itself");

    const QString previousStack = pStackItem->name();

    QString linkedNotebookGuid;
    auto * pParentItem = modelItem.parent();
    if (pParentItem &&
        (pParentItem->type() == INotebookModelItem::Type::LinkedNotebook))
    {
        auto * pLinkedNotebookItem =
            pParentItem->cast<LinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();
        }
    }

    removeModelItemFromParent(modelItem, RemoveEmptyParentStack::No);

    StackItems * pStackItems = nullptr;

    if (linkedNotebookGuid.isEmpty()) {
        pStackItems = &m_stackItems;
    }
    else {
        const auto stackItemsIt =
            m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

        if (Q_UNLIKELY(stackItemsIt == m_stackItemsByLinkedNotebookGuid.end()))
        {
            QNWARNING(
                "model:notebook",
                "Can't properly remove the notebook "
                    << "stack item without children: can't find the map of "
                       "stack "
                    << "items for linked notebook guid " << linkedNotebookGuid);
            return;
        }

        pStackItems = &(stackItemsIt.value());
    }

    const auto it = pStackItems->find(previousStack);
    if (it != pStackItems->end()) {
        Q_UNUSED(pStackItems->erase(it))
    }
    else {
        QNWARNING(
            "model:notebook",
            "Can't find stack model item to remove, "
                << "stack " << previousStack);
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

    auto * pModelItem = itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "failed to find the model item "
                       "corresponding to the model index"));
        return;
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (!pNotebookItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the item attempted to be favorited "
                       "or unfavorited is not a notebook"));
        return;
    }

    if (favorited == pNotebookItem->isFavorited()) {
        QNDEBUG("model:notebook", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(pNotebookItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the notebook item was not found within the model"));
        return;
    }

    NotebookItem itemCopy{*pNotebookItem};
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
        "model:notebook",
        "NotebookModel::findOrCreateLinkedNotebookModelItem: "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    if (Q_UNLIKELY(linkedNotebookGuid.isEmpty())) {
        QNWARNING(
            "model:notebook",
            "Detected the request for finding of "
                << "creation of a linked notebook model item for empty "
                << "linked notebook guid");
        return *m_pAllNotebooksRootItem;
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model:notebook",
            "Found existing linked notebook model item "
                << "for linked notebook guid " << linkedNotebookGuid);
        return linkedNotebookItemIt.value();
    }

    QNDEBUG(
        "model:notebook",
        "Found no existing linked notebook item "
            << "corresponding to linked notebook guid " << linkedNotebookGuid);

    auto linkedNotebookOwnerUsernameIt =
        m_linkedNotebookUsernamesByGuids.find(linkedNotebookGuid);

    if (Q_UNLIKELY(
            linkedNotebookOwnerUsernameIt ==
            m_linkedNotebookUsernamesByGuids.end()))
    {
        QNDEBUG(
            "model:notebook",
            "Found no linked notebook owner's username "
                << "for linked notebook guid " << linkedNotebookGuid);

        linkedNotebookOwnerUsernameIt = m_linkedNotebookUsernamesByGuids.insert(
            linkedNotebookGuid, QString());
    }

    const QString & linkedNotebookOwnerUsername =
        linkedNotebookOwnerUsernameIt.value();

    LinkedNotebookRootItem linkedNotebookItem(
        linkedNotebookOwnerUsername, linkedNotebookGuid);

    linkedNotebookItemIt =
        m_linkedNotebookItems.insert(linkedNotebookGuid, linkedNotebookItem);

    auto * pLinkedNotebookItem = &(linkedNotebookItemIt.value());

    QNTRACE(
        "model:notebook",
        "Linked notebook root item: " << *pLinkedNotebookItem);

    const int row =
        rowForNewItem(*m_pAllNotebooksRootItem, *pLinkedNotebookItem);

    beginInsertRows(indexForItem(m_pAllNotebooksRootItem), row, row);
    m_pAllNotebooksRootItem->insertChild(row, pLinkedNotebookItem);
    endInsertRows();

    return *pLinkedNotebookItem;
}

// WARNING: this method must NOT be called between beginSmth/endSmth
// i.e. beginInsertRows and endInsertRows
void NotebookModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_pInvisibleRootItem)) {
        m_pInvisibleRootItem = new InvisibleRootItem;
        QNDEBUG("model:notebook", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_pAllNotebooksRootItem)) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_pAllNotebooksRootItem = new AllNotebooksRootItem;
        m_pAllNotebooksRootItem->setParent(m_pInvisibleRootItem);
        endInsertRows();
        QNDEBUG("model:notebook", "Created all notebooks root item");
    }
}

INotebookModelItem * NotebookModel::itemForId(const IndexId id) const
{
    QNTRACE("model:notebook", "NotebookModel::itemForId: " << id);

    if (id == m_allNotebooksRootItemIndexId) {
        return m_pAllNotebooksRootItem;
    }

    const auto localIdIt = m_indexIdToLocalIdBimap.left.find(id);
    if (localIdIt != m_indexIdToLocalIdBimap.left.end()) {
        const QString & localId = localIdIt->second;
        QNTRACE(
            "model:notebook",
            "Found notebook local id corresponding to "
                << "model index internal id: " << localId);

        const auto & localIdIndex = m_data.get<ByLocalId>();
        const auto itemIt = localIdIndex.find(localId);
        if (itemIt != localIdIndex.end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to local id: " << *itemIt);
            return const_cast<NotebookItem *>(&(*itemIt));
        }

        QNTRACE(
            "model:notebook",
            "Found no notebook model item corresponding "
                << "to local id");

        return nullptr;
    }

    const auto stackIt =
        m_indexIdToStackAndLinkedNotebookGuidBimap.left.find(id);

    if (stackIt != m_indexIdToStackAndLinkedNotebookGuidBimap.left.end()) {
        const QString & stack = stackIt->second.first;
        const QString & linkedNotebookGuid = stackIt->second.second;
        QNTRACE(
            "model:notebook",
            "Found notebook stack corresponding to model "
                << "index internal id: " << stack
                << ", linked notebook guid = " << linkedNotebookGuid);

        const auto * pStackItems = stackItems(linkedNotebookGuid);
        if (!pStackItems) {
            QNWARNING(
                "model:notebook",
                "Found no stack items corresponding to "
                    << "linked notebook guid " << linkedNotebookGuid);
            return nullptr;
        }

        const auto itemIt = pStackItems->find(stack);
        if (itemIt != pStackItems->end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to stack: " << itemIt.value());
            return const_cast<StackItem *>(&(*itemIt));
        }
    }

    const auto linkedNotebookIt =
        m_indexIdToLinkedNotebookGuidBimap.left.find(id);

    if (linkedNotebookIt != m_indexIdToLinkedNotebookGuidBimap.left.end()) {
        const QString & linkedNotebookGuid = linkedNotebookIt->second;
        QNTRACE(
            "model:notebook",
            "Found linked notebook guid corresponding to "
                << "model index internal id: " << linkedNotebookGuid);

        const auto itemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
        if (itemIt != m_linkedNotebookItems.end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to linked notebook guid: " << itemIt.value());
            return const_cast<LinkedNotebookRootItem *>(&itemIt.value());
        }

        QNTRACE(
            "model:notebook",
            "Found no notebook model item "
                << "corresponding to linked notebook guid");
        return nullptr;
    }

    QNDEBUG(
        "model:notebook",
        "Found no notebook model items corresponding to model index id");
    return nullptr;
}

NotebookModel::IndexId NotebookModel::idForItem(
    const INotebookModelItem & item) const
{
    const auto * pNotebookItem = item.cast<NotebookItem>();
    if (pNotebookItem) {
        const auto it =
            m_indexIdToLocalIdBimap.right.find(pNotebookItem->localId());

        if (it == m_indexIdToLocalIdBimap.right.end()) {
            const IndexId id = m_lastFreeIndexId++;

            Q_UNUSED(m_indexIdToLocalIdBimap.insert(
                IndexIdToLocalIdBimap::value_type(
                    id, pNotebookItem->localId())))

            return id;
        }

        return it->second;
    }

    const auto * pStackItem = item.cast<StackItem>();
    if (pStackItem) {
        QString linkedNotebookGuid;
        auto * pParentItem = item.parent();

        if (pParentItem &&
            (pParentItem->type() == INotebookModelItem::Type::LinkedNotebook))
        {
            auto * pLinkedNotebookItem =
                pParentItem->cast<LinkedNotebookRootItem>();
            if (pLinkedNotebookItem) {
                linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();
            }
        }

        std::pair<QString, QString> pair(
            pStackItem->name(), linkedNotebookGuid);

        const auto it =
            m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(pair);

        if (it == m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
            const IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToStackAndLinkedNotebookGuidBimap.insert(
                IndexIdToStackAndLinkedNotebookGuidBimap::value_type(id, pair)))
            return id;
        }

        return it->second;
    }

    const auto * pLinkedNotebookItem = item.cast<LinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        const auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
            pLinkedNotebookItem->linkedNotebookGuid());

        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            const IndexId id = m_lastFreeIndexId++;

            Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.insert(
                IndexIdToLinkedNotebookGuidBimap::value_type(
                    id, pLinkedNotebookItem->linkedNotebookGuid())))

            return id;
        }

        return it->second;
    }

    if (&item == m_pAllNotebooksRootItem) {
        return m_allNotebooksRootItemIndexId;
    }

    QNWARNING(
        "model:notebook",
        "Detected attempt to assign id to unidentified "
            << "notebook model item: " << item);
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
        ErrorString error(
            QT_TR_NOOP("Can't update the notebook, restrictions apply"));

        error.details() = notebookItem.name();
        QNINFO("model:notebook", error << ", notebookItem = " << notebookItem);
        Q_EMIT notifyError(error);
        return false;
    }

    if ((modelIndex.column() == static_cast<int>(Column::Name)) &&
        !notebookItem.nameIsUpdatable())
    {
        ErrorString error(
            QT_TR_NOOP("Can't update the notebook's name, "
                       "restrictions apply"));

        error.details() = notebookItem.name();
        QNINFO("model:notebook", error << ", notebookItem = " << notebookItem);
        Q_EMIT notifyError(error);
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

    NotebookItem notebookItemCopy = notebookItem;
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
    case Column::LastUsed:
    {
        if (!setNotebookIsLastUsed(notebookItemCopy, value.toBool())) {
            return false;
        }

        dirty = true;
        switchLastUsedNotebookLocalId(notebookItemCopy.localId());
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
        "model:notebook",
        "Sorting by name = " << (sortingByName ? "true" : "false"));

    if (sortingByName) {
        Q_EMIT layoutAboutToBeChanged();
    }

    localIdIndex.replace(notebookItemIt, notebookItemCopy);

    Q_EMIT dataChanged(modelIndex, modelIndex);

    if (sortingByName) {
        Q_EMIT layoutChanged();
    }

    updateNotebookInLocalStorage(notebookItemCopy);
    return true;
}

bool NotebookModel::setNotebookName(
    NotebookItem & notebookItem, const QString & newName)
{
    QNTRACE(
        "model:notebook",
        "New suggested name: " << newName
                               << ", previous name = " << notebookItem.name());

    const bool changed = (newName != notebookItem.name());
    if (!changed) {
        QNTRACE("model:notebook", "Notebook name has not changed");
        return true;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    const auto nameIt = nameIndex.find(newName.toUpper());
    if (nameIt != nameIndex.end()) {
        ErrorString error(
            QT_TR_NOOP("Can't rename the notebook: no two "
                       "notebooks within the account are allowed "
                       "to have the same name in "
                       "case-insensitive manner"));

        QNINFO("model:notebook", error << ", suggested new name = " << newName);
        Q_EMIT notifyError(error);
        return false;
    }

    ErrorString errorDescription;
    if (!validateNotebookName(newName, &errorDescription)) {
        ErrorString error(QT_TR_NOOP("Can't rename the notebook"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNINFO("model:notebook", error << "; suggested new name = " << newName);
        Q_EMIT notifyError(error);
        return false;
    }

    notebookItem.setName(newName);
    return true;
}

bool NotebookModel::setNotebookSynchronizable(
    NotebookItem & notebookItem, const bool synchronizable)
{
    if (m_account.type() == Account::Type::Local) {
        ErrorString error(
            QT_TR_NOOP("Can't make the notebook synchronizable "
                       "within the local account"));

        QNINFO("model:notebook", error);
        Q_EMIT notifyError(error);
        return false;
    }

    if (notebookItem.isSynchronizable() && !synchronizable) {
        ErrorString error(
            QT_TR_NOOP("Can't make the already synchronizable "
                       "notebook not synchronizable"));

        QNINFO(
            "model:notebook",
            error << ", already synchronizable notebook "
                  << "item: " << notebookItem);

        Q_EMIT notifyError(error);
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
            "model:notebook",
            "The default state of the notebook "
                << "hasn't changed, nothing to do");
        return true;
    }

    if (notebookItem.isDefault() && !isDefault) {
        ErrorString error(
            QT_TR_NOOP("In order to stop the notebook from being "
                       "the default one please choose another "
                       "default notebook"));

        QNINFO("model:notebook", error);
        Q_EMIT notifyError(error);
        return false;
    }

    notebookItem.setDefault(true);
    return true;
}

bool NotebookModel::setNotebookIsLastUsed(
    NotebookItem & notebookItem, const bool isLastUsed)
{
    if (notebookItem.isLastUsed() == isLastUsed) {
        QNDEBUG(
            "model:notebook",
            "The last used state of the notebook "
                << "hasn't changed, nothing to do");
        return true;
    }

    if (notebookItem.isLastUsed() && !isLastUsed) {
        ErrorString error(
            QT_TR_NOOP("The last used flag for the notebook is set "
                       "automatically when some of its notes are "
                       "edited"));

        QNDEBUG("model:notebook", error);
        Q_EMIT notifyError(error);
        return false;
    }

    notebookItem.setLastUsed(true);
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
    case Column::LastUsed:
        return notebookItem.isLastUsed();
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
    case Column::LastUsed:
        accessibleText +=
            (textData.toBool() ? tr("last used") : tr("not last used"));
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

    return QVariant(accessibleText);
}

bool NotebookModel::canRemoveNotebookItem(const NotebookItem & notebookItem)
{
    if (!notebookItem.guid().isEmpty()) {
        ErrorString error(
            QT_TR_NOOP("One of notebooks being removed along with the stack "
                       "containing it has non-empty guid, it can't be "
                       "removed"));

        QNINFO("model:notebook", error << ", notebook: " << notebookItem);
        Q_EMIT notifyError(error);
        return false;
    }

    if (!notebookItem.linkedNotebookGuid().isEmpty()) {
        ErrorString error(
            QT_TR_NOOP("One of notebooks being removed along with the stack "
                       "containing it is the linked notebook from another "
                       "account, it can't be removed"));

        QNINFO("model:notebook", error << ", notebook: " << notebookItem);
        Q_EMIT notifyError(error);
        return false;
    }

    return true;
}

bool NotebookModel::updateNoteCountPerNotebookIndex(
    const NotebookItem & item, const NotebookDataByLocalId::iterator it)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::updateNoteCountPerNotebookIndex: " << item);

    auto * pModelItem = const_cast<NotebookItem *>(&(*it));
    auto * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(
            "model:notebook",
            "Can't find the parent notebook model item "
                << "for the notebook item into which the note was inserted: "
                << *pModelItem);
        return false;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the row of the child notebook "
                << "model item within its parent model item: parent item = "
                << *pParentItem << "\nChild item = " << *pModelItem);
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    localIdIndex.replace(it, item);

    const auto modelItemId = idForItem(*pModelItem);

    QNTRACE(
        "model:notebook",
        "Emitting num notes per notebook update "
            << "dataChanged signal: row = " << row
            << ", model item: " << *pModelItem);

    const auto modelIndexFrom =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    const auto modelIndexTo =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);
    return true;
}

bool NotebookModel::LessByName::operator()(
    const NotebookItem & lhs, const NotebookItem & rhs) const noexcept
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

#define ITEM_PTR_LESS(lhs, rhs)                                                \
    if (!lhs) {                                                                \
        return true;                                                           \
    }                                                                          \
    if (!rhs) {                                                                \
        return false;                                                          \
    }                                                                          \
    return this->operator()(*lhs, *rhs)

bool NotebookModel::LessByName::operator()(
    const NotebookItem * lhs,
    const NotebookItem * rhs) const noexcept
{
    ITEM_PTR_LESS(lhs, rhs);
}

namespace {

[[nodiscard]] QString modelItemName(const INotebookModelItem & item)
{
    const bool isNotebookItem = (item.type() == INotebookModelItem::Type::Notebook);

    const bool isLinkedNotebookItem = !isNotebookItem &&
        (item.type() == INotebookModelItem::Type::LinkedNotebook);

    const bool isStackItem = !isNotebookItem && !isLinkedNotebookItem &&
        (item.type() == INotebookModelItem::Type::Stack);

    const NotebookItem * pNotebookItem = nullptr;
    if (isNotebookItem) {
        pNotebookItem = dynamic_cast<const NotebookItem *>(&item);
    }

    const LinkedNotebookRootItem * pLinkedNotebookItem = nullptr;
    if (isLinkedNotebookItem) {
        pLinkedNotebookItem =
            dynamic_cast<const LinkedNotebookRootItem *>(&item);
    }

    const StackItem * pStackItem = nullptr;
    if (!isNotebookItem && !isLinkedNotebookItem) {
        pStackItem = dynamic_cast<const StackItem *>(&item);
    }

    if (isNotebookItem && pNotebookItem) {
        return pNotebookItem->nameUpper();
    }

    if (isStackItem && pStackItem) {
        return pStackItem->name().toUpper();
    }

    if (isLinkedNotebookItem && pLinkedNotebookItem) {
        return pLinkedNotebookItem->username();
    }

    return {};
}

} // namespace

bool NotebookModel::LessByName::operator()(
    const INotebookModelItem & lhs,
    const INotebookModelItem & rhs) const noexcept
{
    if ((lhs.type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }

    if ((lhs.type() != INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs.type() == INotebookModelItem::Type::LinkedNotebook) &&
        (rhs.type() != INotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }

    if ((lhs.type() != INotebookModelItem::Type::LinkedNotebook) &&
        (rhs.type() == INotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName = modelItemName(lhs);
    QString rhsName = modelItemName(rhs);

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool NotebookModel::LessByName::operator()(
    const INotebookModelItem * pLhs,
    const INotebookModelItem * pRhs) const noexcept
{
    ITEM_PTR_LESS(pLhs, pRhs);
}

bool NotebookModel::LessByName::operator()(
    const StackItem & lhs, const StackItem & rhs) const noexcept
{
    return (lhs.name().toUpper().localeAwareCompare(rhs.name().toUpper()) <= 0);
}

bool NotebookModel::LessByName::operator()(
    const StackItem * lhs, const StackItem * rhs) const noexcept
{
    ITEM_PTR_LESS(lhs, rhs);
}

bool NotebookModel::LessByName::operator()(
    const LinkedNotebookRootItem & lhs,
    const LinkedNotebookRootItem & rhs) const noexcept
{
    return (
        lhs.username().toUpper().localeAwareCompare(rhs.username().toUpper()) <=
        0);
}

bool NotebookModel::LessByName::operator()(
    const LinkedNotebookRootItem * lhs,
    const LinkedNotebookRootItem * rhs) const noexcept
{
    ITEM_PTR_LESS(lhs, rhs);
}

bool NotebookModel::GreaterByName::operator()(
    const NotebookItem & lhs, const NotebookItem & rhs) const noexcept
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

#define ITEM_PTR_GREATER(lhs, rhs)                                             \
    if (!lhs) {                                                                \
        return false;                                                          \
    }                                                                          \
    if (!rhs) {                                                                \
        return true;                                                           \
    }                                                                          \
    return this->operator()(*lhs, *rhs)

bool NotebookModel::GreaterByName::operator()(
    const NotebookItem * lhs, const NotebookItem * rhs) const noexcept
{
    ITEM_PTR_GREATER(lhs, rhs);
}

bool NotebookModel::GreaterByName::operator()(
    const StackItem & lhs, const StackItem & rhs) const noexcept
{
    return (lhs.name().toUpper().localeAwareCompare(rhs.name().toUpper()) > 0);
}

bool NotebookModel::GreaterByName::operator()(
    const StackItem * lhs, const StackItem * rhs) const noexcept
{
    ITEM_PTR_GREATER(lhs, rhs);
}

bool NotebookModel::GreaterByName::operator()(
    const LinkedNotebookRootItem & lhs,
    const LinkedNotebookRootItem & rhs) const noexcept
{
    return (
        lhs.username().toUpper().localeAwareCompare(rhs.username().toUpper()) >
        0);
}

bool NotebookModel::GreaterByName::operator()(
    const LinkedNotebookRootItem * lhs,
    const LinkedNotebookRootItem * rhs) const noexcept
{
    ITEM_PTR_GREATER(lhs, rhs);
}

bool NotebookModel::GreaterByName::operator()(
    const INotebookModelItem & lhs,
    const INotebookModelItem & rhs) const noexcept
{
    if ((lhs.type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }

    if ((lhs.type() != INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() == INotebookModelItem::Type::AllNotebooksRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs.type() == INotebookModelItem::Type::LinkedNotebook) &&
        (rhs.type() != INotebookModelItem::Type::LinkedNotebook))
    {
        return false;
    }

    if ((lhs.type() != INotebookModelItem::Type::LinkedNotebook) &&
        (rhs.type() == INotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName = modelItemName(lhs);
    QString rhsName = modelItemName(rhs);
    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool NotebookModel::GreaterByName::operator()(
    const INotebookModelItem * pLhs,
    const INotebookModelItem * pRhs) const noexcept
{
    ITEM_PTR_GREATER(pLhs, pRhs);
}

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
    case Column::LastUsed:
        dbg << "Last used";
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

    const auto checkFlag = [&](const Filter filter, const char * text) {
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
    const NotebookModel::RemoveEmptyParentStack option)
{
    using RemoveEmptyParentStack = NotebookModel::RemoveEmptyParentStack;

    switch (option) {
    case RemoveEmptyParentStack::Yes:
        dbg << "Yes";
        break;
    case RemoveEmptyParentStack::No:
        dbg << "No";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(option) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
