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

#include "NotebookModel.h"

#include "AllNotebooksRootItem.h"
#include "InvisibleRootItem.h"

#include <lib/model/common/NewItemNameGenerator.hpp>

#include <quentier/logging/QuentierLogger.h>

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
    int row = pParentItem->rowForChild(pItem);
    QNTRACE("model:notebook", "Child row: " << row);

    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Internal error: can't get the row of "
                << "the child item in parent in NotebookModel, child item: "
                << *pItem << "\nParent item: " << *pParentItem);
        return {};
    }

    auto id = idForItem(*pItem);
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
            return indexForLocalUid(item.localUid());
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

    auto it = pStackItems->find(stack);
    if (it == pStackItems->end()) {
        return {};
    }

    const auto & stackItem = it.value();
    return indexForItem(&stackItem);
}

QModelIndex NotebookModel::indexForLinkedNotebookGuid(
    const QString & linkedNotebookGuid) const
{
    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
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
            "Both can create notes and cannot create "
                << "notes filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotes) &&
        (filters & Filter::CannotUpdateNotes)) {
        QNTRACE(
            "model:notebook",
            "Both can update notes and cannot update "
                << "notes filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanUpdateNotebook) &&
        (filters & Filter::CannotUpdateNotebook))
    {
        QNTRACE(
            "model:notebook",
            "Both can update notebooks and cannot update "
                << "notebooks filters are included, the result is empty");
        return result;
    }

    if ((filters & Filter::CanRenameNotebook) &&
        (filters & Filter::CannotRenameNotebook))
    {
        QNTRACE(
            "model:notebook",
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
                "model:notebook",
                "Skipping notebook item with different "
                    << "linked notebook guid: " << item);
            continue;
        }

        bool canCreateNotes = item.canCreateNotes();
        if ((filters & Filter::CanCreateNotes) && !canCreateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes "
                    << "cannot be created: " << item);
            continue;
        }

        if ((filters & Filter::CannotCreateNotes) && canCreateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes "
                    << "can be created: " << item);
            continue;
        }

        bool canUpdateNotes = item.canUpdateNotes();
        if ((filters & Filter::CanUpdateNotes) && !canUpdateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes "
                    << "cannot be updated: " << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotes) && canUpdateNotes) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item for which notes "
                    << "can be updated: " << item);
            continue;
        }

        bool canUpdateNotebook = item.isUpdatable();
        if ((filters & Filter::CanUpdateNotebook) && !canUpdateNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which cannot be "
                    << "updated: " << item);
            continue;
        }

        if ((filters & Filter::CannotUpdateNotebook) && canUpdateNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which can be "
                    << "updated: " << item);
            continue;
        }

        bool canRenameNotebook = item.nameIsUpdatable();
        if ((filters & Filter::CanRenameNotebook) && !canRenameNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which cannot be "
                    << "renamed: " << item);
            continue;
        }

        if ((filters & Filter::CannotRenameNotebook) && canRenameNotebook) {
            QNTRACE(
                "model:notebook",
                "Skipping notebook item which can be "
                    << "renamed: " << item);
            continue;
        }

        result << it->name();
    }

    return result;
}

QModelIndex NotebookModel::defaultNotebookIndex() const
{
    QNTRACE("model:notebook", "NotebookModel::defaultNotebookIndex");

    if (m_defaultNotebookLocalUid.isEmpty()) {
        QNDEBUG("model:notebook", "No default notebook local uid");
        return {};
    }

    return indexForLocalUid(m_defaultNotebookLocalUid);
}

QModelIndex NotebookModel::lastUsedNotebookIndex() const
{
    QNTRACE("model:notebook", "NotebookModel::lastUsedNotebookIndex");

    if (m_lastUsedNotebookLocalUid.isEmpty()) {
        QNDEBUG("model:notebook", "No last used notebook local uid");
        return {};
    }

    return indexForLocalUid(m_lastUsedNotebookLocalUid);
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(pNotebookItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
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

    NotebookItem notebookItemCopy(*pNotebookItem);
    notebookItemCopy.setStack(stack);
    notebookItemCopy.setDirty(true);

    localUidIndex.replace(it, notebookItemCopy);
    updateNotebookInLocalStorage(notebookItemCopy);

    auto * pNewParentItem = &stackItem;
    auto parentIndex = indexForItem(pNewParentItem);

    int newRow = rowForNewItem(*pNewParentItem, *pModelItem);
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
        auto & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(pNotebookItem->localUid());
        if (Q_UNLIKELY(it == localUidIndex.end())) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't find the notebook item to be removed "
                           "from the stack by the local uid"));
            return {};
        }

        NotebookItem notebookItemCopy(*pNotebookItem);
        notebookItemCopy.setStack(QString());
        notebookItemCopy.setDirty(true);
        localUidIndex.replace(it, notebookItemCopy);

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

        int newRow = rowForNewItem(*m_pAllNotebooksRootItem, *pModelItem);
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

    QModelIndex parentItemIndex = indexForItem(pNewParentItem);

    int newRow = rowForNewItem(*pNewParentItem, *pModelItem);
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
        auto stackItemsIt =
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
    const
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

    int notebookNameSize = notebookName.size();
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingNotebooks = static_cast<int>(localUidIndex.size());
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

    auto parentIndex = indexForItem(pParentItem);

    // Will insert the notebook to the end of the parent item's children
    int row = pParentItem->childrenCount();

    NotebookItem item;
    item.setLocalUid(UidGenerator::Generate());
    Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.insert(item.localUid()))

    item.setName(notebookName);
    item.setDirty(true);
    item.setStack(notebookStack);
    item.setSynchronizable(m_account.type() != Account::Type::Local);
    item.setDefault(numExistingNotebooks == 0);
    item.setLastUsed(numExistingNotebooks == 0);
    item.setUpdatable(true);
    item.setNameIsUpdatable(true);

    auto insertionResult = localUidIndex.insert(item);

    auto * pAddedNotebookItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    pAddedNotebookItem->setParent(pParentItem);
    endInsertRows();

    updateNotebookInLocalStorage(*pAddedNotebookItem);

    auto addedNotebookIndex = indexForLocalUid(item.localUid());

    if (m_sortedColumn != Column::Name) {
        QNDEBUG(
            "model:notebook",
            "Notebook model is not sorted by name, "
                << "skipping sorting");
        Q_EMIT addedNotebook(addedNotebookIndex);
        return addedNotebookIndex;
    }

    Q_EMIT layoutAboutToBeChanged();
    for (auto & item: localUidIndex) {
        updateItemRowWithRespectToSorting(const_cast<NotebookItem &>(item));
    }
    Q_EMIT layoutChanged();

    // Need to update the index as the item's row could have changed as a result
    // of sorting
    addedNotebookIndex = indexForLocalUid(item.localUid());

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

bool NotebookModel::allNotebooksListed() const
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

QString NotebookModel::localUidForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::localUidForItemName: name = "
            << itemName << ", linked notebook guid = " << linkedNotebookGuid);

    auto index = indexForNotebookName(itemName, linkedNotebookGuid);
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

    return pNotebookItem->localUid();
}

QModelIndex NotebookModel::indexForLocalUid(const QString & localUid) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (it == localUidIndex.end()) {
        QNTRACE(
            "model:notebook",
            "Found no notebook model item for local uid " << localUid);
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString NotebookModel::itemNameForLocalUid(const QString & localUid) const
{
    QNTRACE(
        "model:notebook", "NotebookModel::itemNameForLocalUid: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:notebook", "No notebook item with such local uid");
        return {};
    }

    return it->name();
}

AbstractItemModel::ItemInfo NotebookModel::itemInfoForLocalUid(
    const QString & localUid) const
{
    QNTRACE(
        "model:notebook", "NotebookModel::itemInfoForLocalUid: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:notebook", "No notebook item with such local uid");
        return {};
    }

    AbstractItemModel::ItemInfo info;
    info.m_localUid = it->localUid();
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

QVector<AbstractItemModel::LinkedNotebookInfo>
NotebookModel::linkedNotebooksInfo() const
{
    QVector<LinkedNotebookInfo> infos;
    infos.reserve(m_linkedNotebookItems.size());

    for (const auto & it: qevercloud::toRange(m_linkedNotebookItems)) {
        infos.push_back(LinkedNotebookInfo(it.key(), it.value().username()));
    }

    return infos;
}

QString NotebookModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
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

QString NotebookModel::localUidForItemIndex(const QModelIndex & index) const
{
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        return pNotebookItem->localUid();
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

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_NOTEBOOK_MODEL_COLUMNS)) {
        return {};
    }

    auto * pModelItem = itemForIndex(index);
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

    IndexId id = idForItem(*pModelItem);
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

    int row = pGrandParentItem->rowForChild(pParentItem);
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

    IndexId id = idForItem(*pParentItem);
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingNotebooks = static_cast<int>(localUidIndex.size());
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

    std::vector<NotebookDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Adding notebook item
        NotebookItem item;
        item.setLocalUid(UidGenerator::Generate());

        Q_UNUSED(
            m_notebookItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(pStackItem->name());
        item.setSynchronizable(m_account.type() != Account::Type::Local);
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        INotebookModelItem * pAddedNotebookItem =
            const_cast<NotebookItem *>(&(*insertionResult.first));

        pAddedNotebookItem->setParent(pParentItem);
    }
    endInsertRows();

    for (auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it)
    {
        const NotebookItem & item = *(*it);
        updateNotebookInLocalStorage(item);
    }

    if (m_sortedColumn == Column::Name) {
        Q_EMIT layoutAboutToBeChanged();
        for (auto & item: localUidIndex) {
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

    RemoveRowsScopeGuard removeRowsScopeGuard(*this);
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

    QStringList notebookLocalUidsToRemove;
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

    // First simply collect all the local uids and stacks of items to be removed
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

            notebookLocalUidsToRemove << pNotebookItem->localUid();
            QNTRACE(
                "model:notebook",
                "Marked notebook local uid "
                    << pNotebookItem->localUid()
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
        for (int j = 0, size = notebookModelItemsWithinStack.size(); j < size;
             ++j) {
            auto * pNotebookModelItem = notebookModelItemsWithinStack[j];
            if (Q_UNLIKELY(!pNotebookModelItem)) {
                QNWARNING(
                    "model:notebook",
                    "Detected null pointer to notebook "
                        << "model item within the children "
                        << "of the stack item being removed: " << *pChildItem);
                continue;
            }

            auto * pChildNotebookItem =
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

            notebookLocalUidsToRemove << pChildNotebookItem->localUid();

            QNTRACE(
                "model:notebook",
                "Marked notebook local uid "
                    << pChildNotebookItem->localUid()
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

    auto & localUidIndex = m_data.get<ByLocalUid>();

    for (int i = 0, size = notebookLocalUidsToRemove.size(); i < size; ++i) {
        const QString & localUid = notebookLocalUidsToRemove[i];
        QNTRACE(
            "model:notebook",
            "Processing notebook local uid " << localUid
                                             << " scheduled for removal");

        auto notebookItemIt = localUidIndex.find(localUid);
        if (Q_UNLIKELY(notebookItemIt == localUidIndex.end())) {
            QNWARNING(
                "model:notebook",
                "Internal error detected: can't find "
                    << "the notebook model item corresponding to the notebook "
                    << "item being removed: local uid = " << localUid);
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

        Q_UNUSED(localUidIndex.erase(notebookItemIt))

        QNTRACE(
            "model:notebook",
            "Erased the notebook item corresponding to "
                << "local uid " << localUid);

        auto indexIt = m_indexIdToLocalUidBimap.right.find(localUid);
        if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
            m_indexIdToLocalUidBimap.right.erase(indexIt);
        }

        expungeNotebookFromLocalStorage(localUid);

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

    for (int i = 0, size = stacksToRemoveWithLinkedNotebookGuids.size();
         i < size; ++i)
    {
        const auto & pair = stacksToRemoveWithLinkedNotebookGuids.at(i);
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

        auto stackItemIt = stackItems.find(stack);
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
            "Erased the notebook stack item "
                << "corresponding "
                << "to stack " << stack << ", linked notebook "
                << "guid = " << linkedNotebookGuid);
    }

    QNTRACE(
        "model:notebook",
        "Successfully removed row(s) from the notebook "
            << "model");
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    for (auto & item: localUidIndex) {
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(notebookItem.localUid());
    if (it == localUidIndex.end()) {
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

    Q_UNUSED(localUidIndex.erase(it))

    beginInsertRows(parentIndex, row, row);

    auto * pNewParentStackItem = pNewParentItem->cast<StackItem>();

    notebookItem.setStack(
        (pNewParentItem == m_pAllNotebooksRootItem || !pNewParentStackItem)
            ? QString()
            : pNewParentStackItem->name());

    notebookItem.setDirty(true);

    auto insertionResult = localUidIndex.insert(notebookItem);
    auto notebookItemIt = insertionResult.first;

    auto * pModelItem = const_cast<NotebookItem *>(&(*notebookItemIt));

    pNewParentItem->insertChild(row, pModelItem);

    endInsertRows();

    updateItemRowWithRespectToSorting(*pModelItem);
    updateNotebookInLocalStorage(notebookItem);
    return true;
}

void NotebookModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    auto it = m_addNotebookRequestIds.find(requestId);
    if (it != m_addNotebookRequestIds.end()) {
        Q_UNUSED(m_addNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onAddNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNotebookRequestIds.find(requestId);
    if (it == m_addNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onAddNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_addNotebookRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);

    removeItemByLocalUid(notebook.localUid());
}

void NotebookModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onUpdateNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void NotebookModel::onUpdateNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onUpdateNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:notebook",
        "Emitting the request to find the notebook: "
            << "local uid = " << notebook.localUid()
            << ", request id = " << requestId);

    Q_EMIT findNotebook(notebook, requestId);
}

void NotebookModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findNotebookToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onFindNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))

        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(notebook.localUid(), notebook);

        auto & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(notebook.localUid());
        if (it != localUidIndex.end()) {
            updateNotebookInLocalStorage(*it);
        }
    }
}

void NotebookModel::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt =
        m_findNotebookToPerformUpdateRequestIds.find(requestId);

    if ((restoreUpdateIt ==
         m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onFindNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

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
    QString linkedNotebookGuid, QList<Notebook> foundNotebooks, QUuid requestId)
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
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
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
    Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it != m_expungeNotebookRequestIds.end()) {
        Q_UNUSED(m_expungeNotebookRequestIds.erase(it))
        return;
    }

    Q_EMIT aboutToRemoveNotebooks();
    removeItemByLocalUid(notebook.localUid());
    Q_EMIT removedNotebooks();
}

void NotebookModel::onExpungeNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it == m_expungeNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:notebook",
        "NotebookModel::onExpungeNotebookFailed: "
            << "notebook = " << notebook << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_expungeNotebookRequestIds.erase(it))

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onGetNoteCountPerNotebookComplete(
    int noteCount, Notebook notebook,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    auto it = m_noteCountPerNotebookRequestIds.find(requestId);
    if (it == m_noteCountPerNotebookRequestIds.end()) {
        return;
    }

    QNTRACE(
        "model:notebook",
        "NotebookModel::onGetNoteCountPerNotebookComplete: "
            << "note count = " << noteCount << ", notebook = " << notebook
            << "\nRequest id = " << requestId);

    Q_UNUSED(m_noteCountPerNotebookRequestIds.erase(it))

    QString notebookLocalUid = notebook.localUid();

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Can't find the notebook item by local uid: " << notebookLocalUid);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNoteCount(noteCount);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onGetNoteCountPerNotebookFailed(
    ErrorString errorDescription, Notebook notebook,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    auto it = m_noteCountPerNotebookRequestIds.find(requestId);
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

    QString notebookLocalUid = notebook.localUid();

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Can't find the notebook item by local uid: " << notebookLocalUid);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNoteCount(-1);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddNoteComplete: note = " << note << ", request id = "
                                                    << requestId);

    if (Q_UNLIKELY(note.hasDeletionTimestamp())) {
        return;
    }

    if (note.hasNotebookLocalUid()) {
        bool res = incrementNoteCountForNotebook(note.notebookLocalUid());
        if (res) {
            return;
        }
    }

    Notebook notebook;
    if (note.hasNotebookLocalUid()) {
        notebook.setLocalUid(note.notebookLocalUid());
    }
    else if (note.hasNotebookGuid()) {
        notebook.setGuid(note.notebookGuid());
    }
    else {
        QNDEBUG(
            "model:notebook",
            "Added note has no notebook local uid and no "
                << "notebook guid, re-requesting the note count for all "
                   "notebooks");
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onNoteMovedToAnotherNotebook(
    QString noteLocalUid, QString previousNotebookLocalUid,
    QString newNotebookLocalUid)
{
    QNDEBUG(
        "model:notebook",
        "NotebookModel::onNoteMovedToAnotherNotebook: "
            << "note local uid = " << noteLocalUid
            << ", previous notebook local uid = " << previousNotebookLocalUid
            << ", new notebook local uid = " << newNotebookLocalUid);

    Q_UNUSED(decrementNoteCountForNotebook(previousNotebookLocalUid))
    Q_UNUSED(incrementNoteCountForNotebook(newNotebookLocalUid))
}

void NotebookModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeNoteComplete: note = "
            << note << "\nRequest id = " << requestId);

    QString notebookLocalUid;
    if (note.hasNotebookLocalUid()) {
        notebookLocalUid = note.notebookLocalUid();
    }

    // NOTE: it's not sufficient to decrement note count for notebook
    // as this note might have had deletion timestamp set so it did not
    // actually contribute to the displayed note count for this notebook

    Notebook notebook;
    if (!notebookLocalUid.isEmpty()) {
        notebook.setLocalUid(notebookLocalUid);
    }
    else if (note.hasNotebookGuid()) {
        notebook.setGuid(note.notebookGuid());
    }
    else {
        QNDEBUG(
            "model:notebook",
            "Expunged note has no notebook local uid and "
                << "no notebook guid, re-requesting the note count for all "
                << "notebooks");
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onAddLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onAddLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void NotebookModel::onUpdateLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onUpdateLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void NotebookModel::onExpungeLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onExpungeLinkedNotebookComplete: "
            << "request id = " << requestId
            << ", linked notebook: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model:notebook",
            "Received linked notebook expunged event "
                << "but the linked notebook has no guid: " << linkedNotebook
                << ", request id = " << requestId);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();
    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);

    QStringList expungedNotebookLocalUids;
    expungedNotebookLocalUids.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedNotebookLocalUids << it->localUid();
    }

    for (auto it = expungedNotebookLocalUids.constBegin(),
              end = expungedNotebookLocalUids.constEnd();
         it != end; ++it)
    {
        removeItemByLocalUid(*it);
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
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

    auto stackItemsIt =
        m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (stackItemsIt != m_stackItemsByLinkedNotebookGuid.end()) {
        Q_UNUSED(m_stackItemsByLinkedNotebookGuid.erase(stackItemsIt))
    }

    auto indexIt =
        m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);

    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }
}

void NotebookModel::onListAllLinkedNotebooksComplete(
    size_t limit, size_t offset,
    LocalStorageManager::ListLinkedNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QList<LinkedNotebook> foundLinkedNotebooks, QUuid requestId)
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
    ErrorString errorDescription, QUuid requestId)
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

void NotebookModel::createConnections(
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

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    auto order = LocalStorageManager::ListNotebooksOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;
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

void NotebookModel::requestNoteCountForNotebook(const Notebook & notebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::requestNoteCountForNotebook: " << notebook);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerNotebookRequestIds.insert(requestId))
    QNTRACE(
        "model:notebook",
        "Emitting request to get the note count per "
            << "notebook: request id = " << requestId);

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountPerNotebook(notebook, options, requestId);
}

void NotebookModel::requestNoteCountForAllNotebooks()
{
    QNTRACE("model:notebook", "NotebookModel::requestNoteCountForAllNotebooks");

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    for (const auto & item: localUidIndex) {
        Notebook notebook;
        notebook.setLocalUid(item.localUid());
        requestNoteCountForNotebook(notebook);
    }
}

void NotebookModel::requestLinkedNotebooksList()
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::requestLinkedNotebooksList: "
            << "offset = " << m_listLinkedNotebooksOffset);

    auto order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;
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
        else {
            return {};
        }
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
        else {
            return {};
        }
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
            << "local uid = " << item.localUid());

    Notebook notebook;

    auto notYetSavedItemIt =
        m_notebookItemsNotYetInLocalStorageUids.find(item.localUid());

    if (notYetSavedItemIt == m_notebookItemsNotYetInLocalStorageUids.end()) {
        QNDEBUG("model:notebook", "Updating the notebook");

        const auto * pCachedNotebook = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNotebook)) {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))

            Notebook dummy;
            dummy.setLocalUid(item.localUid());
            Q_EMIT findNotebook(dummy, requestId);

            QNTRACE(
                "model:notebook",
                "Emitted the request to find "
                    << "the notebook: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            return;
        }

        notebook = *pCachedNotebook;
    }

    notebook.setLocalUid(item.localUid());
    notebook.setGuid(item.guid());
    notebook.setLinkedNotebookGuid(item.linkedNotebookGuid());
    notebook.setName(item.name());
    notebook.setLocal(!item.isSynchronizable());
    notebook.setDirty(item.isDirty());
    notebook.setDefaultNotebook(item.isDefault());
    notebook.setLastUsed(item.isLastUsed());
    notebook.setPublished(item.isPublished());
    notebook.setFavorited(item.isFavorited());
    notebook.setStack(item.stack());

    // If all item's properties related to the restrictions are "true", there's
    // no real need for the restrictions; otherwise need to set the restrictions
    if (!item.canCreateNotes() || !item.canUpdateNotes() ||
        !item.isUpdatable() || !item.nameIsUpdatable())
    {
        notebook.setCanCreateNotes(item.canCreateNotes());
        notebook.setCanUpdateNotes(item.canUpdateNotes());
        notebook.setCanUpdateNotebook(item.isUpdatable());
        notebook.setCanRenameNotebook(item.nameIsUpdatable());
    }

    m_cache.put(item.localUid(), notebook);

    // NOTE: deliberately not setting the updatable property from the item
    // as it can't be changed by the model and only serves the utilitary
    // purposes

    auto requestId = QUuid::createUuid();

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
        Q_UNUSED(m_cache.remove(notebook.localUid()))

        QNTRACE(
            "model:notebook",
            "Emitting the request to update notebook in "
                << "the local storage: id = " << requestId
                << ", notebook = " << notebook);

        Q_EMIT updateNotebook(notebook, requestId);
    }
}

void NotebookModel::expungeNotebookFromLocalStorage(const QString & localUid)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::expungeNotebookFromLocalStorage: "
            << "local uid = " << localUid);

    Notebook dummyNotebook;
    dummyNotebook.setLocalUid(localUid);

    Q_UNUSED(m_cache.remove(localUid))

    auto requestId = QUuid::createUuid();
    Q_UNUSED(m_expungeNotebookRequestIds.insert(requestId))

    QNDEBUG(
        "model:notebook",
        "Emitting the request to expunge the notebook "
            << "from the local storage: request id = " << requestId
            << ", local uid = " << localUid);

    Q_EMIT expungeNotebook(dummyNotebook, requestId);
}

QString NotebookModel::nameForNewNotebook() const
{
    QString baseName = tr("New notebook");
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

void NotebookModel::onNotebookAddedOrUpdated(const Notebook & notebook)
{
    m_cache.put(notebook.localUid(), notebook);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebook.localUid());
    bool newNotebook = (itemIt == localUidIndex.end());
    if (newNotebook) {
        Q_EMIT aboutToAddNotebook();

        onNotebookAdded(notebook);

        QModelIndex addedNotebookIndex = indexForLocalUid(notebook.localUid());
        Q_EMIT addedNotebook(addedNotebookIndex);
    }
    else {
        QModelIndex notebookIndexBefore = indexForLocalUid(notebook.localUid());
        Q_EMIT aboutToUpdateNotebook(notebookIndexBefore);

        onNotebookUpdated(notebook, itemIt);

        QModelIndex notebookIndexAfter = indexForLocalUid(notebook.localUid());
        Q_EMIT updatedNotebook(notebookIndexAfter);
    }
}

void NotebookModel::onNotebookAdded(const Notebook & notebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onNotebookAdded: notebook "
            << "local uid = " << notebook.localUid());

    checkAndCreateModelRootItems();

    INotebookModelItem * pParentItem = nullptr;
    if (notebook.hasStack()) {
        auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid()
                                             : QString());

        auto * pStackItems = stackItemsWithParentPair.first;
        auto * pGrandParentItem = stackItemsWithParentPair.second;
        const QString & stack = notebook.stack();

        pParentItem =
            &(findOrCreateStackItem(stack, *pStackItems, pGrandParentItem));
    }
    else if (notebook.hasLinkedNotebookGuid()) {
        pParentItem = &(
            findOrCreateLinkedNotebookModelItem(notebook.linkedNotebookGuid()));
    }
    else {
        pParentItem = m_pAllNotebooksRootItem;
    }

    auto parentIndex = indexForItem(pParentItem);

    NotebookItem item;
    notebookToItem(notebook, item);

    int row = pParentItem->childrenCount();

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto insertionResult = localUidIndex.insert(item);
    auto * pInsertedItem =
        const_cast<NotebookItem *>(&(*insertionResult.first));

    beginInsertRows(parentIndex, row, row);
    pInsertedItem->setParent(pParentItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*pInsertedItem);
}

void NotebookModel::onNotebookUpdated(
    const Notebook & notebook, NotebookDataByLocalUid::iterator it)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onNotebookUpdated: notebook "
            << "local uid = " << notebook.localUid());

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

    if (!notebook.hasStack() && !previousStackName.isEmpty()) {
        // Need to remove the notebook model item from the current stack and
        // set proper root item as the parent
        QModelIndex parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        if (!notebook.hasLinkedNotebookGuid()) {
            checkAndCreateModelRootItems();
            pParentItem = m_pAllNotebooksRootItem;
        }
        else {
            pParentItem = &(findOrCreateLinkedNotebookModelItem(
                notebook.linkedNotebookGuid()));
        }

        shouldChangeParent = true;
    }
    else if (notebook.hasStack() && (notebook.stack() != previousStackName)) {
        // Need to remove the notebook from its current parent and insert it
        // under the stack item, either existing or new
        QModelIndex parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        auto stackItemsWithParentPair = stackItemsWithParent(
            notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid()
                                             : QString());

        auto * pStackItems = stackItemsWithParentPair.first;
        auto * pGrandParentItem = stackItemsWithParentPair.second;

        pParentItem = &(findOrCreateStackItem(
            notebook.stack(), *pStackItems, pGrandParentItem));

        shouldChangeParent = true;
    }

    bool notebookStackChanged = false;
    if (shouldChangeParent) {
        QModelIndex parentItemIndex = indexForItem(pParentItem);
        int newRow = pParentItem->childrenCount();
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

    NotebookItem notebookItemCopy(*it);
    notebookToItem(notebook, notebookItemCopy);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, notebookItemCopy);

    auto modelItemId = idForItem(*pModelItem);

    auto modelIndexFrom = createIndex(row, 0, modelItemId);

    QModelIndex modelIndexTo =
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
    const LinkedNotebook & linkedNotebook)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model:notebook",
            "Can't process the addition or update of "
                << "a linked notebook without guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.hasUsername())) {
        QNWARNING(
            "model:notebook",
            "Can't process the addition or update of "
                << "a linked notebook without username: " << linkedNotebook);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();
    const auto & newUsername = linkedNotebook.username();

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

        QModelIndex itemIndex = indexForItem(&linkedNotebookItem);
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

    QModelIndex parentIndex = indexForItem(m_pAllNotebooksRootItem);
    int row = rowForNewItem(*m_pAllNotebooksRootItem, linkedNotebookItem);

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

    QString newStack = value.toString();
    QString previousStack = stackItem.name();
    if (newStack == previousStack) {
        QNDEBUG(
            "model:notebook",
            "Notebook stack hasn't changed, nothing to "
                << "do");
        return true;
    }

    auto children = stackItem.children();
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
    auto parentItemIndex = indexForItem(pParentItem);
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

    auto indexIt = m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(
        std::make_pair(previousStack, linkedNotebookGuid));

    if (indexIt != m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
        m_indexIdToStackAndLinkedNotebookGuidBimap.right.erase(indexIt);
    }

    endRemoveRows();

    // 2) Insert the new stack item

    stackItemIt = m_stackItems.insert(newStack, newStackItem);
    stackItemRow = rowForNewItem(*pParentItem, *stackItemIt);

    QModelIndex stackItemIndex =
        index(stackItemRow, static_cast<int>(Column::Name), parentItemIndex);

    beginInsertRows(parentItemIndex, stackItemRow, stackItemRow);
    pParentItem->insertChild(stackItemRow, &(*stackItemIt));
    endInsertRows();

    // 3) Move all children of the previous stack items to the new one
    auto & localUidIndex = m_data.get<ByLocalUid>();

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

        auto notebookItemIt =
            localUidIndex.find(pChildNotebookItem->localUid());
        if (notebookItemIt == localUidIndex.end()) {
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
        NotebookItem notebookItemCopy(*pChildNotebookItem);
        notebookItemCopy.setStack(newStack);

        bool wasDirty = notebookItemCopy.isDirty();
        notebookItemCopy.setDirty(true);

        localUidIndex.replace(notebookItemIt, notebookItemCopy);

        QModelIndex notebookItemIndex =
            indexForLocalUid(pChildNotebookItem->localUid());

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

    QVariant textData = stackData(stackItem, column);
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

    auto it = m_stackItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
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

        int row = rowForNewItem(*pParentItem, stackItem);
        QModelIndex parentIndex = indexForItem(pParentItem);

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
    auto children = stackItem.children();
    for (auto it = children.begin(), end = children.end(); it != end; ++it) {
        auto * pChildItem = *it;
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(
                "model:notebook",
                "Detected null pointer to notebook "
                    << "model item within the children of another notebook "
                       "model "
                    << "item");
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

void NotebookModel::removeItemByLocalUid(const QString & localUid)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::removeItemByLocalUid: "
            << "local uid = " << localUid);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
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
                << "for the notebook being removed from the model: local uid = "
                << pModelItem->localUid());
        return;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the notebook item's row within "
                << "its parent model item: notebook item = " << *pModelItem
                << "\nStack item = " << *pParentItem);
        return;
    }

    QModelIndex parentItemModelIndex = indexForItem(pParentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    Q_UNUSED(localUidIndex.erase(itemIt))
    Q_UNUSED(m_cache.remove(localUid))

    auto indexIt = m_indexIdToLocalUidBimap.right.find(localUid);
    if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
        m_indexIdToLocalUidBimap.right.erase(indexIt);
    }

    checkAndRemoveEmptyStackItem(*pParentItem);
}

void NotebookModel::notebookToItem(
    const Notebook & notebook, NotebookItem & item)
{
    item.setLocalUid(notebook.localUid());

    if (notebook.hasGuid()) {
        item.setGuid(notebook.guid());
    }

    if (notebook.hasStack()) {
        item.setStack(notebook.stack());
    }

    if (notebook.hasName()) {
        item.setName(notebook.name());
    }

    item.setSynchronizable(!notebook.isLocal());
    item.setDirty(notebook.isDirty());
    item.setFavorited(notebook.isFavorited());

    bool isDefaultNotebook = notebook.isDefaultNotebook();
    item.setDefault(isDefaultNotebook);
    if (isDefaultNotebook) {
        switchDefaultNotebookLocalUid(notebook.localUid());
    }

    bool isLastUsedNotebook = notebook.isLastUsed();
    item.setLastUsed(isLastUsedNotebook);
    if (isLastUsedNotebook) {
        switchLastUsedNotebookLocalUid(notebook.localUid());
    }

    if (notebook.hasPublished()) {
        item.setPublished(notebook.isPublished());
    }

    if (notebook.hasRestrictions()) {
        const auto & restrictions = notebook.restrictions();

        item.setUpdatable(
            !restrictions.noUpdateNotebook.isSet() ||
            !restrictions.noUpdateNotebook.ref());

        item.setNameIsUpdatable(
            !restrictions.noRenameNotebook.isSet() ||
            !restrictions.noRenameNotebook.ref());

        item.setCanCreateNotes(
            !restrictions.noCreateNotes.isSet() ||
            !restrictions.noCreateNotes.ref());

        item.setCanUpdateNotes(
            !restrictions.noUpdateNotes.isSet() ||
            !restrictions.noUpdateNotes.ref());
    }
    else {
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);
        item.setCanCreateNotes(true);
        item.setCanUpdateNotes(true);
    }

    if (notebook.hasLinkedNotebookGuid()) {
        item.setLinkedNotebookGuid(notebook.linkedNotebookGuid());
    }
}

void NotebookModel::removeModelItemFromParent(
    INotebookModelItem & modelItem,
    RemoveEmptyParentStack removeEmptyParentStack)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::removeModelItemFromParent: "
            << modelItem
            << "\nRemove empty parent stack = " << removeEmptyParentStack);

    auto * pParentItem = modelItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNDEBUG("model:notebook", "No parent item, nothing to do");
        return;
    }

    QNTRACE("model:notebook", "Parent item: " << *pParentItem);
    int row = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:notebook",
            "Can't find the child notebook model "
                << "item's row within its parent; child item = " << modelItem
                << ", parent item = " << *pParentItem);
        return;
    }

    QNTRACE("model:notebook", "Will remove the child at row " << row);

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    if (removeEmptyParentStack == RemoveEmptyParentStack::Yes) {
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

    int row = static_cast<int>(std::distance(children.begin(), it));
    return row;
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

        int row = rowForNewItem(*pParentItem, modelItem);

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

    auto parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(pParentItem->takeChild(currentItemRow))
    endRemoveRows();

    int appropriateRow = rowForNewItem(*pParentItem, modelItem);

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
    auto indices = persistentIndexList();
    for (const auto & index: qAsConst(indices)) {
        auto * pItem = itemForId(static_cast<IndexId>(index.internalId()));
        auto replacementIndex = indexForItem(pItem);
        changePersistentIndex(index, replacementIndex);
    }
}

bool NotebookModel::incrementNoteCountForNotebook(
    const QString & notebookLocalUid)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::incrementNoteCountForNotebook: " << notebookLocalUid);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Wasn't able to find the notebook item by "
                << "the specified local uid");
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.noteCount();
    ++noteCount;
    item.setNoteCount(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

bool NotebookModel::decrementNoteCountForNotebook(
    const QString & notebookLocalUid)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::decrementNoteCountForNotebook: " << notebookLocalUid);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG(
            "model:notebook",
            "Wasn't able to find the notebook item by "
                << "the specified local uid");
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.noteCount();
    --noteCount;
    noteCount = std::max(noteCount, 0);
    item.setNoteCount(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

void NotebookModel::switchDefaultNotebookLocalUid(const QString & localUid)
{
    QNTRACE(
        "model:notebook",
        "NotebookModel::switchDefaultNotebookLocalUid: " << localUid);

    if (!m_defaultNotebookLocalUid.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "There has already been some notebook chosen "
                << "as the default, switching the default flag off for it");

        auto & localUidIndex = m_data.get<ByLocalUid>();

        auto previousDefaultItemIt =
            localUidIndex.find(m_defaultNotebookLocalUid);

        if (previousDefaultItemIt != localUidIndex.end()) {
            NotebookItem previousDefaultItemCopy(*previousDefaultItemIt);
            QNTRACE(
                "model:notebook",
                "Previous default notebook item: " << previousDefaultItemCopy);

            previousDefaultItemCopy.setDefault(false);
            bool wasDirty = previousDefaultItemCopy.isDirty();
            previousDefaultItemCopy.setDirty(true);

            localUidIndex.replace(
                previousDefaultItemIt, previousDefaultItemCopy);

            auto previousDefaultItemIndex =
                indexForLocalUid(m_defaultNotebookLocalUid);

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

    m_defaultNotebookLocalUid = localUid;
    QNTRACE("model:notebook", "Set default notebook local uid to " << localUid);
}

void NotebookModel::switchLastUsedNotebookLocalUid(const QString & localUid)
{
    QNTRACE(
        "model:notebook", "NotebookModel::setLastUsedNotebook: " << localUid);

    if (!m_lastUsedNotebookLocalUid.isEmpty()) {
        QNTRACE(
            "model:notebook",
            "There has already been some notebook chosen "
                << "as the last used one, switching the last used flag off for "
                   "it");

        auto & localUidIndex = m_data.get<ByLocalUid>();

        auto previousLastUsedItemIt =
            localUidIndex.find(m_lastUsedNotebookLocalUid);

        if (previousLastUsedItemIt != localUidIndex.end()) {
            NotebookItem previousLastUsedItemCopy(*previousLastUsedItemIt);
            QNTRACE(
                "model:notebook",
                "Previous last used notebook item: "
                    << previousLastUsedItemCopy);

            previousLastUsedItemCopy.setLastUsed(false);
            bool wasDirty = previousLastUsedItemCopy.isDirty();
            previousLastUsedItemCopy.setDirty(true);

            localUidIndex.replace(
                previousLastUsedItemIt, previousLastUsedItemCopy);

            auto previousLastUsedItemIndex =
                indexForLocalUid(m_lastUsedNotebookLocalUid);

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

    m_lastUsedNotebookLocalUid = localUid;

    QNTRACE(
        "model:notebook", "Set last used notebook local uid to " << localUid);
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

    QString previousStack = pStackItem->name();

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
        auto stackItemsIt =
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

    auto it = pStackItems->find(previousStack);
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(pNotebookItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the notebook: "
                       "the notebook item was not found within the model"));
        return;
    }

    NotebookItem itemCopy(*pNotebookItem);
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the notebook as dirty as favorited property is
    // not included into the synchronization protocol

    localUidIndex.replace(it, itemCopy);

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

    int row = rowForNewItem(*m_pAllNotebooksRootItem, *pLinkedNotebookItem);
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
        m_pAllNotebooksRootItem = new AllNotebooksRootItem;
        beginInsertRows(QModelIndex(), 0, 0);
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

    auto localUidIt = m_indexIdToLocalUidBimap.left.find(id);
    if (localUidIt != m_indexIdToLocalUidBimap.left.end()) {
        const QString & localUid = localUidIt->second;
        QNTRACE(
            "model:notebook",
            "Found notebook local uid corresponding to "
                << "model index internal id: " << localUid);

        const auto & localUidIndex = m_data.get<ByLocalUid>();
        auto itemIt = localUidIndex.find(localUid);
        if (itemIt != localUidIndex.end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to local uid: " << *itemIt);
            return const_cast<NotebookItem *>(&(*itemIt));
        }

        QNTRACE(
            "model:notebook",
            "Found no notebook model item corresponding "
                << "to local uid");

        return nullptr;
    }

    auto stackIt = m_indexIdToStackAndLinkedNotebookGuidBimap.left.find(id);
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

        auto itemIt = pStackItems->find(stack);
        if (itemIt != pStackItems->end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to stack: " << itemIt.value());
            return const_cast<StackItem *>(&(*itemIt));
        }
    }

    auto linkedNotebookIt = m_indexIdToLinkedNotebookGuidBimap.left.find(id);
    if (linkedNotebookIt != m_indexIdToLinkedNotebookGuidBimap.left.end()) {
        const QString & linkedNotebookGuid = linkedNotebookIt->second;
        QNTRACE(
            "model:notebook",
            "Found linked notebook guid corresponding to "
                << "model index internal id: " << linkedNotebookGuid);

        auto itemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
        if (itemIt != m_linkedNotebookItems.end()) {
            QNTRACE(
                "model:notebook",
                "Found notebook model item corresponding "
                    << "to linked notebook guid: " << itemIt.value());
            return const_cast<LinkedNotebookRootItem *>(&itemIt.value());
        }
        else {
            QNTRACE(
                "model:notebook",
                "Found no notebook model item "
                    << "corresponding to linked notebook guid");
            return nullptr;
        }
    }

    QNDEBUG(
        "model:notebook",
        "Found no notebook model items corresponding to "
            << "model index id");
    return nullptr;
}

NotebookModel::IndexId NotebookModel::idForItem(
    const INotebookModelItem & item) const
{
    const auto * pNotebookItem = item.cast<NotebookItem>();
    if (pNotebookItem) {
        auto it =
            m_indexIdToLocalUidBimap.right.find(pNotebookItem->localUid());

        if (it == m_indexIdToLocalUidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;

            Q_UNUSED(m_indexIdToLocalUidBimap.insert(
                IndexIdToLocalUidBimap::value_type(
                    id, pNotebookItem->localUid())))

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

        auto it = m_indexIdToStackAndLinkedNotebookGuidBimap.right.find(pair);
        if (it == m_indexIdToStackAndLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToStackAndLinkedNotebookGuidBimap.insert(
                IndexIdToStackAndLinkedNotebookGuidBimap::value_type(id, pair)))
            return id;
        }

        return it->second;
    }

    const auto * pLinkedNotebookItem = item.cast<LinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
            pLinkedNotebookItem->linkedNotebookGuid());

        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;

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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto notebookItemIt = localUidIndex.find(notebookItem.localUid());
    if (Q_UNLIKELY(notebookItemIt == localUidIndex.end())) {
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
        bool wasSynchronizable = notebookItemCopy.isSynchronizable();

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
        switchDefaultNotebookLocalUid(notebookItemCopy.localUid());
        break;
    }
    case Column::LastUsed:
    {
        if (!setNotebookIsLastUsed(notebookItemCopy, value.toBool())) {
            return false;
        }

        dirty = true;
        switchLastUsedNotebookLocalUid(notebookItemCopy.localUid());
        break;
    }
    default:
        return false;
    }

    notebookItemCopy.setDirty(dirty);

    bool sortingByName =
        (modelIndex.column() == static_cast<int>(Column::Name)) &&
        (m_sortedColumn == Column::Name);

    QNTRACE(
        "model:notebook",
        "Sorting by name = " << (sortingByName ? "true" : "false"));

    if (sortingByName) {
        Q_EMIT layoutAboutToBeChanged();
    }

    localUidIndex.replace(notebookItemIt, notebookItemCopy);

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

    bool changed = (newName != notebookItem.name());
    if (!changed) {
        QNTRACE("model:notebook", "Notebook name has not changed");
        return true;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    auto nameIt = nameIndex.find(newName.toUpper());
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
    if (!Notebook::validateName(newName, &errorDescription)) {
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
    QVariant textData = notebookData(notebookItem, column);
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
    const NotebookItem & item, const NotebookDataByLocalUid::iterator it)
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

    auto & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, item);

    auto modelItemId = idForItem(*pModelItem);

    QNTRACE(
        "model:notebook",
        "Emitting num notes per notebook update "
            << "dataChanged signal: row = " << row
            << ", model item: " << *pModelItem);

    auto modelIndexFrom =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    auto modelIndexTo =
        createIndex(row, static_cast<int>(Column::NoteCount), modelItemId);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);
    return true;
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
    bool isNotebookItem = (item.type() == INotebookModelItem::Type::Notebook);

    bool isLinkedNotebookItem = !isNotebookItem &&
        (item.type() == INotebookModelItem::Type::LinkedNotebook);

    bool isStackItem = !isNotebookItem && !isLinkedNotebookItem &&
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

bool NotebookModel::LessByName::operator()(
    const INotebookModelItem & lhs, const INotebookModelItem & rhs) const
{
    if ((lhs.type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }
    else if (
        (lhs.type() != INotebookModelItem::Type::AllNotebooksRoot) &&
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
    else if (
        (lhs.type() != INotebookModelItem::Type::LinkedNotebook) &&
        (rhs.type() == INotebookModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName = modelItemName(lhs);
    QString rhsName = modelItemName(rhs);

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
    if ((lhs.type() == INotebookModelItem::Type::AllNotebooksRoot) &&
        (rhs.type() != INotebookModelItem::Type::AllNotebooksRoot))
    {
        return false;
    }
    else if (
        (lhs.type() != INotebookModelItem::Type::AllNotebooksRoot) &&
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
    else if (
        (lhs.type() != INotebookModelItem::Type::LinkedNotebook) &&
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
    const INotebookModelItem * pRhs) const {ITEM_PTR_GREATER(pLhs, pRhs)}

////////////////////////////////////////////////////////////////////////////////

QDebug &
operator<<(QDebug & dbg, const NotebookModel::Column column)
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
