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

#include "NotebookModel.h"
#include "NoteModel.h"
#include "NewItemNameGenerator.hpp"
#include <quentier/logging/QuentierLogger.h>
#include <QMimeData>

namespace quentier {

// Limit for the queries to the local storage
#define NOTEBOOK_LIST_LIMIT (40)

#define NUM_NOTEBOOK_MODEL_COLUMNS (8)

#define REPORT_ERROR(error, ...) \
    ErrorString errorDescription(error); \
    QNWARNING(errorDescription << "" __VA_ARGS__ ); \
    emit notifyError(errorDescription)

NotebookModel::NotebookModel(const Account & account, const NoteModel & noteModel,
                             LocalStorageManagerAsync & localStorageManagerAsync,
                             NotebookCache & cache, QObject * parent) :
    ItemModel(parent),
    m_account(account),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_defaultNotebookLocalUid(),
    m_modelItemsByLocalUid(),
    m_modelItemsByStack(),
    m_stackItems(),
    m_indexIdToLocalUidBimap(),
    m_indexIdToStackBimap(),
    m_lastFreeIndexId(1),
    m_cache(cache),
    m_listNotebooksOffset(0),
    m_listNotebooksRequestId(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds(),
    m_expungeNotebookRequestIds(),
    m_findNotebookToRestoreFailedUpdateRequestIds(),
    m_findNotebookToPerformUpdateRequestIds(),
    m_noteCountPerNotebookRequestIds(),
    m_notebookLocalUidByNoteLocalUid(),
    m_receivedNotebookLocalUidsForAllNotes(false),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_lastNewNotebookNameCounter(0),
    m_allNotebooksListed(false)
{
    createConnections(noteModel, localStorageManagerAsync);

    if (noteModel.allNotesListed()) {
        buildNotebookLocalUidByNoteLocalUidsHash(noteModel);
    }

    requestNotebooksList();
}

NotebookModel::~NotebookModel()
{
    delete m_fakeRootItem;
}

void NotebookModel::updateAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("NotebookModel::updateAccount: ") << account);
    m_account = account;
}

const NotebookModelItem * NotebookModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_fakeRootItem;
    }

    return itemForId(static_cast<IndexId>(index.internalId()));
}

QModelIndex NotebookModel::indexForItem(const NotebookModelItem * pItem) const
{
    QNTRACE(QStringLiteral("NotebookModel::indexForItem: ")
            << (pItem ? pItem->toString() : QStringLiteral("<null>")));

    if (!pItem) {
        return QModelIndex();
    }

    if (pItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * pParentItem = pItem->parent();
    if (!pParentItem) {
        QNWARNING(QStringLiteral("The notebook model item has no parent, returning invalid index for it: ") << *pItem);
        return QModelIndex();
    }

    QNTRACE(QStringLiteral("Parent item: ") << *pParentItem);
    int row = pParentItem->rowForChild(pItem);
    QNTRACE(QStringLiteral("Child row: ") << row);

    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Internal error: can't get the row of the child item in parent in NotebookModel, child item: ")
                  << *pItem << QStringLiteral("\nParent item: ") << *pParentItem);
        return QModelIndex();
    }

    IndexId id = idForItem(*pItem);
    if (Q_UNLIKELY(id == 0)) {
        QNWARNING(QStringLiteral("The notebook model item has the internal id of 0: ") << *pItem);
        return QModelIndex();
    }

    return createIndex(row, Columns::Name, id);
}

QModelIndex NotebookModel::indexForLocalUid(const QString & localUid) const
{
    auto it = m_modelItemsByLocalUid.find(localUid);
    if (it == m_modelItemsByLocalUid.end()) {
        QNTRACE(QStringLiteral("Found no notebook model item for local uid ") << localUid);
        return QModelIndex();
    }

    const NotebookModelItem & item = *it;
    return indexForItem(&item);
}

QModelIndex NotebookModel::indexForNotebookName(const QString & notebookName,
                                                const QString & linkedNotebookGuid) const
{
    const NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto range = nameIndex.equal_range(notebookName.toUpper());
    for(auto it = range.first; it != range.second; ++it)
    {
        const NotebookItem & item = *it;
        const QString & itemLinkedNotebookGuid = item.linkedNotebookGuid();

        if (itemLinkedNotebookGuid.isEmpty() && linkedNotebookGuid.isEmpty()) {
            return indexForLocalUid(item.localUid());
        }

        if (itemLinkedNotebookGuid == linkedNotebookGuid) {
            return indexForLocalUid(item.localUid());
        }
    }

    return QModelIndex();
}

QModelIndex NotebookModel::indexForNotebookStack(const QString & stack) const
{
    auto it = m_modelItemsByStack.find(stack);
    if (it == m_modelItemsByStack.end()) {
        return QModelIndex();
    }

    const NotebookModelItem & modelItem = it.value();
    return indexForItem(&modelItem);
}

QStringList NotebookModel::notebookNames(const NotebookFilters filters) const
{
    QNDEBUG(QStringLiteral("NotebookModel::notebookNames: filters = ") << filters);

    if (filters == NotebookFilters(NotebookFilter::NoFilter)) {
        QNTRACE(QStringLiteral("No filter, returning all notebook names"));
        return itemNames();
    }

    QStringList result;

    if ((filters & NotebookFilter::CanCreateNotes) && (filters & NotebookFilter::CannotCreateNotes)) {
        QNTRACE(QStringLiteral("Both can create notes and cannot create notes filters are included, the result is empty"));
        return result;
    }

    if ((filters & NotebookFilter::CanUpdateNotes) && (filters & NotebookFilter::CannotUpdateNotes)) {
        QNTRACE(QStringLiteral("Both can update notes and cannot update notes filters are included, the result is empty"));
        return result;
    }

    if ((filters & NotebookFilter::CanUpdateNotebook) && (filters & NotebookFilter::CannotUpdateNotebook)) {
        QNTRACE(QStringLiteral("Both can update notebooks and cannot update notebooks filters are included, the result is empty"));
        return result;
    }

    if ((filters & NotebookFilter::CanRenameNotebook) && (filters & NotebookFilter::CannotRenameNotebook)) {
        QNTRACE(QStringLiteral("Both can rename notebook and cannot rename notebook filters are inclided, the result is empty"));
        return result;
    }

    const NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    result.reserve(static_cast<int>(nameIndex.size()));
    for(auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it)
    {
        const NotebookItem & item = *it;

        bool canCreateNotes = item.canCreateNotes();
        if ((filters & NotebookFilter::CanCreateNotes) && !canCreateNotes) {
            QNTRACE(QStringLiteral("Skipping notebook item for which notes cannot be created: ") << item);
            continue;
        }

        if ((filters & NotebookFilter::CannotCreateNotes) && canCreateNotes) {
            QNTRACE(QStringLiteral("Skipping notebook item for which notes can be created: ") << item);
            continue;
        }

        bool canUpdateNotes = item.canUpdateNotes();
        if ((filters & NotebookFilter::CanUpdateNotes) && !canUpdateNotes) {
            QNTRACE(QStringLiteral("Skipping notebook item for which notes cannot be updated: ") << item);
            continue;
        }

        if ((filters & NotebookFilter::CannotUpdateNotes) && canUpdateNotes) {
            QNTRACE(QStringLiteral("Skipping notebook item for which notes can be updated: ") << item);
            continue;
        }

        bool canUpdateNotebook = item.isUpdatable();
        if ((filters & NotebookFilter::CanUpdateNotebook) && !canUpdateNotebook) {
            QNTRACE(QStringLiteral("Skipping notebook item which cannot be updated: ") << item);
            continue;
        }

        if ((filters & NotebookFilter::CannotUpdateNotebook) && canUpdateNotebook) {
            QNTRACE(QStringLiteral("Skipping notebook item which can be updated: ") << item);
            continue;
        }

        bool canRenameNotebook = item.nameIsUpdatable();
        if ((filters & NotebookFilter::CanRenameNotebook) && !canRenameNotebook) {
            QNTRACE(QStringLiteral("Skipping notebook item which cannot be renamed: ") << item);
            continue;
        }

        if ((filters & NotebookFilter::CannotRenameNotebook) && canRenameNotebook) {
            QNTRACE(QStringLiteral("Skipping notebook item which can be renamed: ") << item);
            continue;
        }

        result << it->name();
    }

    return result;
}

QModelIndexList NotebookModel::persistentIndexes() const
{
    return persistentIndexList();
}

QModelIndex NotebookModel::defaultNotebookIndex() const
{
    QNDEBUG(QStringLiteral("NotebookModel::defaultNotebookIndex"));

    if (m_defaultNotebookLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("No default notebook local uid"));
        return QModelIndex();
    }

    return indexForLocalUid(m_defaultNotebookLocalUid);
}

QModelIndex NotebookModel::lastUsedNotebookIndex() const
{
    QNDEBUG(QStringLiteral("NotebookModel::lastUsedNotebookIndex"));

    if (m_lastUsedNotebookLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("No last used notebook local uid"));
        return QModelIndex();
    }

    return indexForLocalUid(m_lastUsedNotebookLocalUid);
}

QModelIndex NotebookModel::moveToStack(const QModelIndex & index, const QString & stack)
{
    QNDEBUG(QStringLiteral("NotebookModel::moveToStack: stack = ") << stack);

    if (Q_UNLIKELY(stack.isEmpty())) {
        return removeFromStack(index);
    }

    const NotebookModelItem * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: detected attempt to move a notebook to another stack but "
                                       "the respective model index has no internal id corresponding to the notebook model item"));
        return QModelIndex();
    }

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't move the non-notebook model item to a stack"));
        return QModelIndex();
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: found a notebook model item of notebook type but its pointer "
                                       "to the notebook item is null"));
        return QModelIndex();
    }

    if (pNotebookItem->stack() == stack) {
        QNDEBUG(QStringLiteral("The stack of the item hasn't changed, nothing to do"));
        return index;
    }

    const NotebookModelItem * pParentItem = pModelItem->parent();
    removeModelItemFromParent(*pModelItem);

    if (pParentItem) {
        checkAndRemoveEmptyStackItem(*pParentItem);
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto notebookItemIt = localUidIndex.find(pNotebookItem->localUid());
    if (Q_UNLIKELY(notebookItemIt == localUidIndex.end()))
    {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: failed to find the notebook item to be moved to another stack"));
        auto it = m_modelItemsByLocalUid.find(pNotebookItem->localUid());
        if (it != m_modelItemsByLocalUid.end()) {
            Q_UNUSED(m_modelItemsByLocalUid.erase(it))
        }
        return QModelIndex();
    }

    auto it = m_modelItemsByStack.end();
    auto stackItemIt = m_stackItems.find(stack);
    if (stackItemIt == m_stackItems.end()) {
        stackItemIt = m_stackItems.insert(stack, NotebookStackItem(stack));
        it = addNewStackModelItem(stackItemIt.value());
    }

    if (it == m_modelItemsByStack.end())
    {
        it = m_modelItemsByStack.find(stack);
        if (it == m_modelItemsByStack.end()) {
            QNWARNING(QStringLiteral("Internal error: no notebook model item while it's expected to be here; "
                                     "will try to auto-fix it and add the new model item"));
            it = addNewStackModelItem(stackItemIt.value());
        }
    }

    if (it == m_modelItemsByStack.end()) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: no notebook model item while it's expected to be present"));
        return QModelIndex();
    }

    NotebookItem notebookItemCopy(*pNotebookItem);
    notebookItemCopy.setStack(stack);
    notebookItemCopy.setDirty(true);
    localUidIndex.replace(notebookItemIt, notebookItemCopy);

    updateNotebookInLocalStorage(notebookItemCopy);

    const NotebookModelItem * pNewParentItem = &(*it);

    QModelIndex parentIndex = indexForItem(pNewParentItem);

    int newRow = rowForNewItem(*pNewParentItem, *pModelItem);
    beginInsertRows(parentIndex, newRow, newRow);
    pNewParentItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QNTRACE(QStringLiteral("Emitting \"notifyNotebookStackChanged\" signal"));
    emit notifyNotebookStackChanged(index);
    return indexForItem(pModelItem);
}

QModelIndex NotebookModel::removeFromStack(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookModel::removeFromStack"));

    const NotebookModelItem * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: detected attempt to remove a notebook from its stack "
                                       "but the respective model index has no internal id corresponding to the notebook model item"));
        return QModelIndex();
    }

    if (pModelItem->type() != NotebookModelItem::Type::Notebook) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't remove the non-notebook model item from the stack"));
        return QModelIndex();
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: found a notebook model item of notebook type but its pointer "
                                       "to the notebook item is null"));
        return QModelIndex();
    }

    if (pNotebookItem->stack().isEmpty())
    {
        QNWARNING(QStringLiteral("The notebook doesn't appear to have the stack but will continue the attempt to remove it from the stack anyway"));
    }
    else
    {
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(pNotebookItem->localUid());
        if (Q_UNLIKELY(it == localUidIndex.end())) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't find the notebook item to be removed from the stack by the local uid"));
            return QModelIndex();
        }

        NotebookItem notebookItemCopy(*pNotebookItem);
        notebookItemCopy.setStack(QString());
        notebookItemCopy.setDirty(true);
        localUidIndex.replace(it, notebookItemCopy);

        updateNotebookInLocalStorage(notebookItemCopy);
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * pParentItem = pModelItem->parent();
    if (!pParentItem)
    {
        QNDEBUG(QStringLiteral("Notebook item doesn't have a parent, will set it to fake root item"));

        int newRow = rowForNewItem(*m_fakeRootItem, *pModelItem);
        beginInsertRows(QModelIndex(), newRow, newRow);
        m_fakeRootItem->insertChild(newRow, pModelItem);
        endInsertRows();

        return indexForItem(pModelItem);
    }

    if (pParentItem == m_fakeRootItem) {
        QNDEBUG(QStringLiteral("The notebook item doesn't belong to any stack, nothing to do"));
        return index;
    }

    removeModelItemFromParent(*pModelItem);

    if (pParentItem) {
        checkAndRemoveEmptyStackItem(*pParentItem);
    }

    int newRow = rowForNewItem(*m_fakeRootItem, *pModelItem);
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_fakeRootItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QNTRACE(QStringLiteral("Emitting \"notifyNotebookStackChanged\" signal"));
    emit notifyNotebookStackChanged(index);
    return indexForItem(pModelItem);
}

QStringList NotebookModel::stacks() const
{
    QStringList result;
    result.reserve(m_stackItems.size());

    for(auto it = m_stackItems.constBegin(), end = m_stackItems.constEnd(); it != end; ++it) {
        result.push_back(it.key());
    }

    return result;
}

QModelIndex NotebookModel::createNotebook(const QString & notebookName,
                                          const QString & notebookStack,
                                          ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NotebookModel::createNotebook: notebook name = ")
            << notebookName << QStringLiteral(", notebook stack = ") << notebookStack);

    if (notebookName.isEmpty()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("", "Notebook name is empty"));
        return QModelIndex();
    }

    int notebookNameSize = notebookName.size();

    if (notebookNameSize < qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("", "Notebook name size is below the minimal acceptable length"));
        errorDescription.details() = QString::number(qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN);
        return QModelIndex();
    }

    if (notebookNameSize > qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("", "Notebook name size is above the maximal acceptable length"));
        errorDescription.details() = QString::number(qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX);
        return QModelIndex();
    }

    QModelIndex existingItemIndex = indexForNotebookName(notebookName);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("", "Notebook with such name already exists"));
        return QModelIndex();
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingNotebooks = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingNotebooks + 1 >= m_account.notebookCountMax())) {
        errorDescription.setBase(QT_TRANSLATE_NOOP("", "Can't create a new notebook: the account can "
                                                   "contain a limited number of notebooks"));
        errorDescription.details() = QString::number(m_account.notebookCountMax());
        return QModelIndex();
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * pParentItem = m_fakeRootItem;

    emit aboutToAddNotebook();

    if (!notebookStack.isEmpty())
    {
        auto it = m_modelItemsByStack.end();
        auto stackItemIt = m_stackItems.find(notebookStack);
        if (stackItemIt == m_stackItems.end()) {
            QNDEBUG(QStringLiteral("Adding new notebook stack item"));
            stackItemIt = m_stackItems.insert(notebookStack, NotebookStackItem(notebookStack));
            it = addNewStackModelItem(stackItemIt.value());
        }

        if (it == m_modelItemsByStack.end())
        {
            it = m_modelItemsByStack.find(notebookStack);
            if (it == m_modelItemsByStack.end()) {
                QNWARNING(QStringLiteral("Internal error: no notebook model item while it's expected to be here; "
                                         "will try to auto-fix it and add the new model item"));
                it = addNewStackModelItem(stackItemIt.value());
            }
        }

        if (it == m_modelItemsByStack.end()) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("", "Internal error: no notebook model item "
                                                       "while it's expected to be there; failed "
                                                       "to auto-fix the problem"));
            emit addedNotebook(QModelIndex());
            return QModelIndex();
        }

        pParentItem = &(it.value());
        QNDEBUG(QStringLiteral("Will put the new notebook under parent item: ") << *pParentItem);
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    // Will insert the notebook to the end of the parent item's children
    int row = pParentItem->numChildren();

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

    // Adding wrapping notebook model item
    NotebookModelItem modelItem(NotebookModelItem::Type::Notebook, &(*insertionResult.first));
    auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);

    beginInsertRows(parentIndex, row, row);
    modelItemInsertionResult.value().setParent(pParentItem);
    endInsertRows();

    updateNotebookInLocalStorage(item);

    QModelIndex addedNotebookIndex = indexForLocalUid(item.localUid());

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG(QStringLiteral("Not sorting by name, returning"));
        emit addedNotebook(addedNotebookIndex);
        return addedNotebookIndex;
    }

    emit layoutAboutToBeChanged();
    for(auto it = m_modelItemsByLocalUid.begin(), end = m_modelItemsByLocalUid.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }
    emit layoutChanged();

    // Need to update the index as the item's row could have changed as a result of sorting
    addedNotebookIndex = indexForLocalUid(item.localUid());

    emit addedNotebook(addedNotebookIndex);
    return addedNotebookIndex;
}

QString NotebookModel::columnName(const NotebookModel::Columns::type column) const
{
    switch(column)
    {
    case Columns::Name:
        return tr("Name");
    case Columns::Synchronizable:
        return tr("Synchronizable");
    case Columns::Dirty:
        return tr("Changed");
    case Columns::Default:
        return tr("Default");
    case Columns::LastUsed:
        return tr("Last used");
    case Columns::Published:
        return tr("Published");
    case Columns::FromLinkedNotebook:
        return tr("External");
    case Columns::NumNotesPerNotebook:
        return tr("Num notes");
    default:
        return QString();
    }
}

void NotebookModel::favoriteNotebook(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookModel::favoriteNotebook: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row() << QStringLiteral(", column = ")
            << index.column() << QStringLiteral(", internal id = ") << index.internalId());

    setNotebookFavorited(index, true);
}

void NotebookModel::unfavoriteNotebook(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("NotebookModel::unfavoriteNotebook: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row() << QStringLiteral(", column = ")
            << index.column() << QStringLiteral(", internal id = ") << index.internalId());

    setNotebookFavorited(index, false);
}

QString NotebookModel::localUidForItemName(const QString & itemName) const
{
    QNDEBUG(QStringLiteral("NotebookModel::localUidForItemName: ") << itemName);

    QModelIndex index = indexForNotebookName(itemName);
    const NotebookModelItem * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        QNTRACE(QStringLiteral("No notebook model item found for index found for this notebook name"));
        return QString();
    }

    if (Q_UNLIKELY(pModelItem->type() != NotebookModelItem::Type::Notebook)) {
        QNTRACE(QStringLiteral("Found notebook model item has wrong type"));
        return QString();
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        QNDEBUG(QStringLiteral("Found notebook model item of notebook type but "
                               "with null pointer to the actual notebook item: ") << *pModelItem);
        return QString();
    }

    return pNotebookItem->localUid();
}

QString NotebookModel::itemNameForLocalUid(const QString & localUid) const
{
    QNDEBUG(QStringLiteral("NotebookModel::itemNameForLocalUid: ") << localUid);

    const NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE(QStringLiteral("No notebook item with such local uid"));
        return QString();
    }

    return it->name();
}

QStringList NotebookModel::itemNames() const
{
    QStringList result;
    const NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    result.reserve(static_cast<int>(nameIndex.size()));
    for(auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it) {
        result << it->name();
    }

    return result;
}

Qt::ItemFlags NotebookModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((index.column() == Columns::Dirty) ||
        (index.column() == Columns::FromLinkedNotebook))
    {
        return indexFlags;
    }

    const NotebookModelItem * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        QNWARNING(QStringLiteral("Can't find notebook model item for a given index: row = ") << index.row()
                  << QStringLiteral(", column = ") << index.column());
        return indexFlags;
    }

    if (pItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * pStackItem = pItem->notebookStackItem();
        if (Q_UNLIKELY(!pStackItem)) {
            QNWARNING(QStringLiteral("Internal inconsistency detected: notebook model item has stack type "
                                     "but the pointer to the stack item is null"));
            return indexFlags;
        }

        indexFlags |= Qt::ItemIsDropEnabled;

        // Check whether all the notebooks in the stack are eligible for editing
        // of the column in question
        QList<const NotebookModelItem*> children = pItem->children();
        for(auto it = children.begin(), end = children.end(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            if (Q_UNLIKELY(!pChildItem)) {
                QNWARNING(QStringLiteral("Detected null pointer to notebook model item within the children of another notebook model item"));
                continue;
            }

            if (Q_UNLIKELY(pChildItem->type() == NotebookModelItem::Type::Stack)) {
                QNWARNING(QStringLiteral("Detected nested notebook stack items which is unexpected and most probably incorrect"));
                continue;
            }

            const NotebookItem * pNotebookItem = pChildItem->notebookItem();
            if (Q_UNLIKELY(!pNotebookItem)) {
                QNWARNING(QStringLiteral("Detected null pointer to notebook item in notebook model item having a type of notebook (not stack)"));
                continue;
            }

            if ((index.column() == Columns::Synchronizable) && pNotebookItem->isSynchronizable()) {
                return indexFlags;
            }

            if ((index.column() == Columns::Name) && !canUpdateNotebookItem(*pNotebookItem)) {
                return indexFlags;
            }
        }
    }
    else
    {
        const NotebookItem * pNotebookItem = pItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            QNWARNING(QStringLiteral("Detected null pointer to notebook item within the notebook model"));
            return indexFlags;
        }

        indexFlags |= Qt::ItemIsDragEnabled;

        if ((index.column() == Columns::Synchronizable) && pNotebookItem->isSynchronizable()) {
            return indexFlags;
        }

        if ((index.column() == Columns::Name) && !canUpdateNotebookItem(*pNotebookItem)) {
            return indexFlags;
        }
    }

    if (index.column() == Columns::Name) {
        indexFlags |= Qt::ItemIsEditable;
    }

    return indexFlags;
}

QVariant NotebookModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_NOTEBOOK_MODEL_COLUMNS)) {
        return QVariant();
    }

    const NotebookModelItem * pItem = itemForIndex(index);
    if (!pItem) {
        return QVariant();
    }

    if (pItem == m_fakeRootItem) {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::Name:
        column = Columns::Name;
        break;
    case Columns::Synchronizable:
        column = Columns::Synchronizable;
        break;
    case Columns::Dirty:
        column = Columns::Dirty;
        break;
    case Columns::Default:
        column = Columns::Default;
        break;
    case Columns::LastUsed:
        column = Columns::LastUsed;
        break;
    case Columns::Published:
        column = Columns::Published;
        break;
    case Columns::FromLinkedNotebook:
        column = Columns::FromLinkedNotebook;
        break;
    case Columns::NumNotesPerNotebook:
        column = Columns::NumNotesPerNotebook;
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(*pItem, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*pItem, column);
    default:
        return QVariant();
    }
}

QVariant NotebookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    return columnName(static_cast<Columns::type>(section));
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    const NotebookModelItem * pParentItem = itemForIndex(parent);
    return (pParentItem ? pParentItem->numChildren() : 0);
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    return NUM_NOTEBOOK_MODEL_COLUMNS;
}

QModelIndex NotebookModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!m_fakeRootItem || (row < 0) || (column < 0) || (column >= NUM_NOTEBOOK_MODEL_COLUMNS) ||
        (parent.isValid() && (parent.column() != Columns::Name)))
    {
        return QModelIndex();
    }

    const NotebookModelItem * pParentItem = itemForIndex(parent);
    if (!pParentItem) {
        return QModelIndex();
    }

    const NotebookModelItem * pItem = pParentItem->childAtRow(row);
    if (!pItem) {
        return QModelIndex();
    }

    IndexId id = idForItem(*pItem);
    if (Q_UNLIKELY(id == 0)) {
        return QModelIndex();
    }

    return createIndex(row, column, id);
}

QModelIndex NotebookModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    const NotebookModelItem * pChildItem = itemForIndex(index);
    if (!pChildItem) {
        return QModelIndex();
    }

    const NotebookModelItem * pParentItem = pChildItem->parent();
    if (!pParentItem) {
        return QModelIndex();
    }

    if (pParentItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * pGrandParentItem = pParentItem->parent();
    if (!pGrandParentItem) {
        return QModelIndex();
    }

    int row = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Internal inconsistency detected in NotebookModel: parent of the item can't find the item within its children: item = ")
                  << *pParentItem << QStringLiteral("\nParent item: ") << *pGrandParentItem);
        return QModelIndex();
    }

    IndexId id = idForItem(*pParentItem);
    if (Q_UNLIKELY(id == 0)) {
        return QModelIndex();
    }

    return createIndex(row, Columns::Name, id);
}

bool NotebookModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNDEBUG(QStringLiteral("NotebookModel::setData: row = ") << modelIndex.row()
            << QStringLiteral(", column = ") << modelIndex.column()
            << QStringLiteral(", internal id = ") << modelIndex.internalId()
            << QStringLiteral(", value = ") << value << QStringLiteral(", role = ") << role);

    if (role != Qt::EditRole) {
        QNDEBUG(QStringLiteral("Role is not EditRole, skipping"));
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG(QStringLiteral("Index is not valid"));
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "The \"dirty\" flag is set automatically when the notebook is changed"));
        return false;
    }

    if (modelIndex.column() == Columns::Published) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "The \"published\" flag can't be set manually"));
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "The \"from linked notebook\" flag can't be set manually"));
        return false;
    }

    if (modelIndex.column() == Columns::NumNotesPerNotebook) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "The \"notes per notebook\" column can't be set manually"));
        return false;
    }

    const NotebookModelItem * pModelItem = itemForIndex(modelIndex);
    if (!pModelItem) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "No notebook model item was found for the given model index"));
        return false;
    }

    if (Q_UNLIKELY(pModelItem == m_fakeRootItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set data for the invisible root item within the noteobok model"));
        return false;
    }

    bool isNotebookItem = (pModelItem->type() == NotebookModelItem::Type::Notebook);

    if (Q_UNLIKELY(isNotebookItem && !pModelItem->notebookItem())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal inconsistency detected in the notebook model: the model item of notebook type "
                                       "has a null pointer to the actual notebook item"));
        return false;
    }

    if (Q_UNLIKELY(!isNotebookItem && !pModelItem->notebookStackItem())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal inconsistency detected in the notebook model: the model item of stack type "
                                       "has a null pointer to stack item"));
        return false;
    }

    NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    if (isNotebookItem)
    {
        const NotebookItem * pNotebookItem = pModelItem->notebookItem();

        if (!canUpdateNotebookItem(*pNotebookItem))
        {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't update the notebook, restrictions apply"));
            error.details() = pNotebookItem->name();
            QNINFO(error << QStringLiteral(", notebookItem = ") << *pNotebookItem);
            emit notifyError(error);
            return false;
        }
        else if ((modelIndex.column() == Columns::Name) && !pNotebookItem->nameIsUpdatable())
        {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't update the notebook's name, restrictions apply"));
            error.details() = pNotebookItem->name();
            QNINFO(error << QStringLiteral(", notebookItem = ") << *pNotebookItem);
            emit notifyError(error);
            return false;
        }

        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto notebookItemIt = localUidIndex.find(pNotebookItem->localUid());
        if (Q_UNLIKELY(notebookItemIt == localUidIndex.end())) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't update the notebook: internal error, can't find the notebook to be updated in the model"));
            return false;
        }

        NotebookItem notebookItemCopy = *pNotebookItem;
        bool dirty = notebookItemCopy.isDirty();

        switch(modelIndex.column())
        {
        case Columns::Name:
            {
                QString newName = value.toString().trimmed();
                QNTRACE(QStringLiteral("New suggested name: ") << newName
                        << QStringLiteral(", previous name = ") << notebookItemCopy.name());

                bool changed = (newName != notebookItemCopy.name());
                if (!changed) {
                    QNTRACE(QStringLiteral("Notebook name has not changed"));
                    return true;
                }

                auto nameIt = nameIndex.find(newName.toUpper());
                if (nameIt != nameIndex.end()) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Can't rename the notebook: no two notebooks within the account are allowed "
                                                        "to have the same name in a case-insensitive manner"));
                    QNINFO(error << QStringLiteral(", suggested new name = ") << newName);
                    emit notifyError(error);
                    return false;
                }

                ErrorString errorDescription;
                if (!Notebook::validateName(newName, &errorDescription)) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Can't rename the notebook"));
                    error.appendBase(errorDescription.base());
                    error.appendBase(errorDescription.additionalBases());
                    error.details() = errorDescription.details();
                    QNINFO(error << QStringLiteral("; suggested new name = ") << newName);
                    emit notifyError(error);
                    return false;
                }

                dirty = true;
                notebookItemCopy.setName(newName);
                break;
            }
        case Columns::Synchronizable:
            {
                if (m_account.type() == Account::Type::Local) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Can't make the notebook synchronizable within the local account"));
                    QNINFO(error);
                    emit notifyError(error);
                    return false;
                }

                if (notebookItemCopy.isSynchronizable() && !value.toBool()) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "Can't make the already synchronizable notebook not synchronizable"));
                    QNINFO(error << QStringLiteral(", already synchronizable notebook item: ") << notebookItemCopy);
                    emit notifyError(error);
                    return false;
                }

                dirty |= (value.toBool() != notebookItemCopy.isSynchronizable());
                notebookItemCopy.setSynchronizable(value.toBool());
                break;
            }
        case Columns::Default:
            {
                if (notebookItemCopy.isDefault() == value.toBool()) {
                    QNDEBUG(QStringLiteral("The default state of the notebook hasn't changed, nothing to do"));
                    return true;
                }

                if (notebookItemCopy.isDefault() && !value.toBool()) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "In order to stop the notebook from being the default "
                                                        "one please choose another default notebook"));
                    QNINFO(error);
                    emit notifyError(error);
                    return false;
                }

                notebookItemCopy.setDefault(true);
                dirty = true;
                switchDefaultNotebookLocalUid(notebookItemCopy.localUid());
                break;
            }
        case Columns::LastUsed:
            {
                if (notebookItemCopy.isLastUsed() == value.toBool()) {
                    QNDEBUG(QStringLiteral("The last used state of the notebook hasn't changed, nothing to do"));
                    return true;
                }

                if (notebookItemCopy.isLastUsed() && !value.toBool()) {
                    ErrorString error(QT_TRANSLATE_NOOP("", "The last used flag for the notebook is set automatically "
                                                        "when some of its notes is edited"));
                    QNDEBUG(error);
                    emit notifyError(error);
                    return false;
                }

                notebookItemCopy.setLastUsed(true);
                dirty = true;
                switchLastUsedNotebookLocalUid(notebookItemCopy.localUid());
                break;
            }
        default:
            return false;
        }

        notebookItemCopy.setDirty(dirty);

        bool sortingByName = (modelIndex.column() == Columns::Name) &&
                             (m_sortedColumn == Columns::Name);
        QNTRACE(QStringLiteral("Sorting by name = ")
                << (sortingByName ? QStringLiteral("true") : QStringLiteral("false")));

        if (sortingByName) {
            emit layoutAboutToBeChanged();
        }

        localUidIndex.replace(notebookItemIt, notebookItemCopy);

        QNTRACE(QStringLiteral("Emitting the data changed signal"));
        emit dataChanged(modelIndex, modelIndex);

        if (sortingByName) {
            emit layoutChanged();
        }

        updateNotebookInLocalStorage(notebookItemCopy);
    }
    else
    {
        if (modelIndex.column() != Columns::Name) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't change any column other than name for the notebook stack item"));
            return false;
        }

        const NotebookStackItem * pNotebookStackItem = pModelItem->notebookStackItem();
        QString newStack = value.toString();
        QString previousStack = pNotebookStackItem->name();
        if (newStack == previousStack) {
            QNDEBUG(QStringLiteral("Notebook stack hasn't changed, nothing to do"));
            return true;
        }

#define CHECK_ITEM(pChildItem) \
            if (Q_UNLIKELY(!pChildItem)) { \
                QNWARNING(QStringLiteral("Detected null pointer to notebook model item " \
                                         "within the children of another item: item = ") << *pModelItem); \
                continue; \
            } \
            \
            if (Q_UNLIKELY(pChildItem->type() == NotebookModelItem::Type::Stack)) { \
                QNWARNING(QStringLiteral("Internal inconsistency detected: found notebook stack item " \
                                         "being a child of another notebook stack item: item = ") << *pModelItem); \
                continue; \
            } \
            \
            const NotebookItem * pNotebookItem = pChildItem->notebookItem(); \
            if (Q_UNLIKELY(!pNotebookItem)) { \
                QNWARNING(QStringLiteral("Detected null pointer to notebook item within the children " \
                                         "of notebook stack item: item = ") << *pModelItem); \
                continue; \
            }

        QList<const NotebookModelItem*> children = pModelItem->children();
        for(auto it = children.constBegin(), end = children.constEnd(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            CHECK_ITEM(pChildItem)

            if (!canUpdateNotebookItem(*pNotebookItem)) {
                ErrorString error(QT_TRANSLATE_NOOP("", "Can't update the notebook stack: restrictions on at least one "
                                                    "of stacked notebooks update apply"));
                error.details() = pNotebookItem->name();
                QNINFO(error << QStringLiteral(", notebook item for which the restrictions apply: ") << *pNotebookItem);
                emit notifyError(error);
                return false;
            }
        }

        // Change the stack item
        auto stackModelItemIt = m_modelItemsByStack.find(previousStack);
        if (stackModelItemIt == m_modelItemsByStack.end()) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: can't update the notebook stack, can't find the notebook "
                                                "model item for the previous stack value"));
            QNWARNING(error << QStringLiteral(", previous stack: \"") << previousStack
                      << QStringLiteral("\", new stack: \"") << newStack << QStringLiteral("\""));
            emit notifyError(error);
            return false;
        }

        NotebookModelItem stackModelItemCopy(stackModelItemIt.value());
        if (Q_UNLIKELY(stackModelItemCopy.type() != NotebookModelItem::Type::Stack)) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't update the notebook stack, internal inconsistency detected: "
                                           "non-stack model item is kept within the hash of model items supposed to be stack items"));
            return false;
        }

        pNotebookStackItem = stackModelItemCopy.notebookStackItem();
        if (Q_UNLIKELY(!pNotebookStackItem)) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: can't update the notebook stack, detected null pointer "
                                           "to the notebook stack item within the notebook model item wrapping it"));
            return false;
        }

        NotebookStackItem newStackItem(*pNotebookStackItem);
        newStackItem.setName(newStack);

        // 1) Remove the previous stack item

        int stackItemRow = m_fakeRootItem->rowForChild(&(stackModelItemIt.value()));
        if (Q_UNLIKELY(stackItemRow < 0)) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't update the notebook stack item: can't find the row of the stack item "
                                           "to be updated within the root item"));
            return false;
        }

        bool sortingByName = (m_sortedColumn == Columns::Name);
        QNTRACE(QStringLiteral("Sorting by name = ") << (sortingByName ? QStringLiteral("true") : QStringLiteral("false")));

        if (sortingByName) {
            emit layoutAboutToBeChanged();
        }

        // Remove children from the stack model item about to be removed; set the parent to the fake root item for now
        for(auto it = children.constBegin(), end = children.constEnd(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            if (Q_UNLIKELY(!pChildItem)) {
                continue;
            }

            int row = stackModelItemIt.value().rowForChild(pChildItem);
            if (Q_UNLIKELY(row < 0)) {
                QNWARNING(QStringLiteral("Couldn't find row for the child notebook item of the stack item "
                                         "about to be removed; stack item: ") << stackModelItemIt.value()
                          << QStringLiteral("\nChild item: ") << *pChildItem);
                continue;
            }

            beginRemoveRows(modelIndex, row, row);
            Q_UNUSED(stackModelItemIt.value().takeChild(row))
            endRemoveRows();

            row = rowForNewItem(*m_fakeRootItem, *pChildItem);
            beginInsertRows(QModelIndex(), row, row);
            m_fakeRootItem->insertChild(row, pChildItem);
            endInsertRows();

            QNTRACE(QStringLiteral("Temporarily inserted the child notebook item from removed stack item at the fake root item "
                                   "at row ") << row << QStringLiteral(", child item: ") << *pChildItem);
        }

        // Remove the reparented children from the copy of the stack model item as well
        while(!stackModelItemCopy.children().isEmpty()) {
            Q_UNUSED(stackModelItemCopy.takeChild(0))
        }

        // As we've just reparented some items to the fake root item, need to figure out the stack item's row again
        stackItemRow = m_fakeRootItem->rowForChild(&(stackModelItemIt.value()));
        if (Q_UNLIKELY(stackItemRow < 0)) {
            REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't update the notebook stack item, fatal error: can't find the row of the stack item "
                                           "to be updated within the root item after temporarily moving its children under the root item"));
            return false;
        }

        beginRemoveRows(QModelIndex(), stackItemRow, stackItemRow);
        Q_UNUSED(m_fakeRootItem->takeChild(stackItemRow))

        auto stackItemIt = m_stackItems.find(previousStack);
        if (stackItemIt != m_stackItems.end()) {
            Q_UNUSED(m_stackItems.erase(stackItemIt))
        }

        Q_UNUSED(m_modelItemsByStack.erase(stackModelItemIt));

        auto indexIt = m_indexIdToStackBimap.right.find(previousStack);
        if (indexIt != m_indexIdToStackBimap.right.end()) {
            m_indexIdToStackBimap.right.erase(indexIt);
        }

        endRemoveRows();

        // 2) Insert the new stack item

        stackItemIt = m_stackItems.insert(newStack, newStackItem);
        stackModelItemCopy.setNotebookStackItem(&(stackItemIt.value()));

        stackModelItemIt = m_modelItemsByStack.insert(newStack, stackModelItemCopy);

        stackItemRow = rowForNewItem(*m_fakeRootItem, stackModelItemIt.value());
        QModelIndex stackItemIndex = index(stackItemRow, Columns::Name, QModelIndex());

        beginInsertRows(QModelIndex(), stackItemRow, stackItemRow);
        m_fakeRootItem->insertChild(stackItemRow, &(stackModelItemIt.value()));

        IndexId indexId = m_lastFreeIndexId++;
        m_indexIdToStackBimap.insert(IndexIdToStackBimap::value_type(indexId, newStack));
        endInsertRows();

        // 3) Move all children of the previous stack items to the new one
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

        for(auto it = children.constBegin(), end = children.constEnd(); it != end; ++it)
        {
            const NotebookModelItem * pChildItem = *it;
            CHECK_ITEM(pChildItem)

            auto notebookItemIt = localUidIndex.find(pNotebookItem->localUid());
            if (notebookItemIt == localUidIndex.end()) {
                QNWARNING(QStringLiteral("Internal inconsistency detected: can't find the iterator for the notebook item "
                                         "for which the stack it being changed: non-found notebook item: ") << *pNotebookItem);
                continue;
            }

            int row = m_fakeRootItem->rowForChild(pChildItem);
            if (Q_UNLIKELY(row < 0)) {
                QNWARNING(QStringLiteral("Internal error: can't find the row of one of the removed stack item's children "
                                         "within the fake root item to which they have been temporarily moved; fake root item: ")
                          << *m_fakeRootItem << QStringLiteral("\nChild item: ") << *pChildItem);
                continue;
            }

            beginRemoveRows(QModelIndex(), row, row);
            Q_UNUSED(m_fakeRootItem->takeChild(row))
            endRemoveRows();

            row = rowForNewItem(stackModelItemIt.value(), *pChildItem);
            beginInsertRows(stackItemIndex, row, row);
            stackModelItemIt.value().insertChild(row, pChildItem);
            endInsertRows();

            // Also update the stack within the notebook item
            NotebookItem notebookItemCopy(*pNotebookItem);
            notebookItemCopy.setStack(newStack);

            bool wasDirty = notebookItemCopy.isDirty();
            notebookItemCopy.setDirty(true);

            localUidIndex.replace(notebookItemIt, notebookItemCopy);

            QModelIndex notebookItemIndex = indexForLocalUid(pNotebookItem->localUid());

            QNTRACE(QStringLiteral("Emitting the data changed signal"));
            emit dataChanged(notebookItemIndex, notebookItemIndex);

            if (!wasDirty) {
                notebookItemIndex = index(notebookItemIndex.row(), Columns::Dirty, notebookItemIndex.parent());
                emit dataChanged(notebookItemIndex, notebookItemIndex);
            }

            updateNotebookInLocalStorage(notebookItemCopy);
        }

        if (sortingByName) {
            emit layoutChanged();
        }

        emit notifyNotebookStackRenamed(previousStack, newStack);

#undef CHECK_ITEM

    }

    QNDEBUG(QStringLiteral("Successfully set the data"));
    return true;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG(QStringLiteral("NotebookModel::insertRows: row = ") << row << QStringLiteral(", count = ") << count
            << QStringLiteral(", parent: is valid = ") << (parent.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << parent.row() << QStringLiteral(", column = ") << parent.column());

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * pParentItem = (parent.isValid()
                                             ? itemForIndex(parent)
                                             : m_fakeRootItem);
    if (!pParentItem) {
        QNDEBUG(QStringLiteral("No model item for given model index"));
        return false;
    }

    if (pParentItem->type() == NotebookModelItem::Type::Notebook) {
        QNDEBUG(QStringLiteral("Can't insert notebook under another notebook, only under the notebook stack"));
        return false;
    }

    const NotebookStackItem * pStackItem = pParentItem->notebookStackItem();
    if (Q_UNLIKELY(!pStackItem)) {
        QNDEBUG(QStringLiteral("Detected null pointer to notebook stack item within the notebook model item of stack type: model item = ")
                << *pParentItem);
        return false;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingNotebooks = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingNotebooks + count >= m_account.notebookCountMax())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't create a new notebook: the account can contain a limited number of notebooks"));
        error.details() = QString::number(m_account.notebookCountMax());
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    std::vector<NotebookDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        // Adding notebook item
        NotebookItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(pStackItem->name());
        item.setSynchronizable(m_account.type() != Account::Type::Local);
        item.setUpdatable(true);
        item.setNameIsUpdatable(true);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        // Adding wrapping notebook model item
        NotebookModelItem modelItem(NotebookModelItem::Type::Notebook, &(*(addedItems.back())));
        auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);
        modelItemInsertionResult.value().setParent(pParentItem);
    }
    endInsertRows();

    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const NotebookItem & item = *(*it);
        updateNotebookInLocalStorage(item);
    }

    if (m_sortedColumn == Columns::Name)
    {
        emit layoutAboutToBeChanged();
        for(auto it = m_modelItemsByLocalUid.begin(), end = m_modelItemsByLocalUid.end(); it != end; ++it) {
            updateItemRowWithRespectToSorting(*it);
        }
        emit layoutChanged();
    }

    QNDEBUG(QStringLiteral("Successfully inserted the row(s)"));
    return true;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG(QStringLiteral("NotebookModel::removeRows: row = ") << row
            << QStringLiteral(", count = ") << count << QStringLiteral(", parent index: row = ")
            << parent.row() << QStringLiteral(", column = ") << parent.column()
            << QStringLiteral(", internal id = ") << parent.internalId());

    RemoveRowsScopeGuard removeRowsScopeGuard(*this);
    Q_UNUSED(removeRowsScopeGuard)

    if (!m_fakeRootItem) {
        QNDEBUG(QStringLiteral("No fake root item"));
        return false;
    }

    const NotebookModelItem * pParentItem = (parent.isValid()
                                             ? itemForIndex(parent)
                                             : m_fakeRootItem);
    if (!pParentItem) {
        QNDEBUG(QStringLiteral("No item corresponding to parent index"));
        return false;
    }

    if (Q_UNLIKELY((pParentItem != m_fakeRootItem) && (pParentItem->type() != NotebookModelItem::Type::Stack))) {
        QNDEBUG(QStringLiteral("Can't remove row(s) from parent item not being a stack item: ") << *pParentItem);
        return false;
    }

    QStringList notebookLocalUidsToRemove;
    QStringList notebookStacksToRemove;

    // First simply collect all the local uids and stacks of items to be removed
    // while checking for each of them whether they can be safely removed
    for(int i = 0; i < count; ++i)
    {
        const NotebookModelItem * pChildItem = pParentItem->childAtRow(row + i);
        if (!pChildItem) {
            QNWARNING(QStringLiteral("Detected null pointer to child notebook model item on attempt to remove row ")
                      << row + i << QStringLiteral(" from parent item: ") << *pParentItem);
            continue;
        }

        QNTRACE(QStringLiteral("Removing item at ") << pChildItem << QStringLiteral(": ") << *pChildItem
                << QStringLiteral(" at row ") << (row + i) << QStringLiteral(" from parent item at ") << pParentItem
                << QStringLiteral(": ") << *pParentItem);

        bool isNotebookItem = (pChildItem->type() == NotebookModelItem::Type::Notebook);
        if (Q_UNLIKELY(isNotebookItem && !pChildItem->notebookItem())) {
            QNWARNING(QStringLiteral("Detected null pointer to notebook item in notebook model item being removed: row in parent = ")
                      << row + i << QStringLiteral(", parent item: ") << *pParentItem << QStringLiteral("\nChild item: ") << *pChildItem);
            continue;
        }

        if (Q_UNLIKELY(!isNotebookItem && !pChildItem->notebookStackItem())) {
            QNWARNING(QStringLiteral("Detected null pointer to notebook stack item in notebook model item being removed: row in parent = ")
                      << row + i << QStringLiteral(", parent item: ") << *pParentItem << QStringLiteral("\nChild item: ") << *pChildItem);
            continue;
        }

        if (isNotebookItem)
        {
            const NotebookItem * pNotebookItem = pChildItem->notebookItem();

#define CHECK_NOTEBOOK_ITEM(pNotebookItem) \
            if (pNotebookItem->isSynchronizable()) { \
                ErrorString error(QT_TRANSLATE_NOOP("", "One of notebooks being removed along with the stack containing it " \
                                                    "is synchronizable, it can't be removed")); \
                QNINFO(error << QStringLiteral(", notebook: ") << *pNotebookItem); \
                emit notifyError(error); \
                return false; \
            } \
            \
            if (!pNotebookItem->linkedNotebookGuid().isEmpty()) { \
                ErrorString error(QT_TRANSLATE_NOOP("", "One of notebooks being removed along with the stack containing it " \
                                                    "is the linked notebook from another account, it can't be removed")); \
                QNINFO(error << QStringLiteral(", notebook: ") << *pNotebookItem); \
                emit notifyError(error); \
                return false; \
            }

            CHECK_NOTEBOOK_ITEM(pNotebookItem)
            notebookLocalUidsToRemove << pNotebookItem->localUid();
            QNTRACE(QStringLiteral("Marked notebook local uid ") << pNotebookItem->localUid()
                    << QStringLiteral(" as the one scheduled for removal"));
        }
        else
        {
            QNTRACE(QStringLiteral("Removing notebook stack: first remove all its child notebooks and then itself"));

            QList<const NotebookModelItem*> notebookModelItemsWithinStack = pChildItem->children();
            for(int j = 0, size = notebookModelItemsWithinStack.size(); j < size; ++j)
            {
                const NotebookModelItem * notebookModelItem = notebookModelItemsWithinStack[j];
                if (Q_UNLIKELY(!notebookModelItem)) {
                    QNWARNING(QStringLiteral("Detected null pointer to notebook model item within the children of the stack item being removed: ")
                              << *pChildItem);
                    continue;
                }

                if (Q_UNLIKELY(notebookModelItem->type() != NotebookModelItem::Type::Notebook)) {
                    QNWARNING(QStringLiteral("Detected notebook model item within the stack item which is not a notebook by type; stack item: ")
                              << *pChildItem << QStringLiteral("\nIts child with wrong type: ") << *notebookModelItem);
                    continue;
                }

                const NotebookItem * pNotebookItem = notebookModelItem->notebookItem();
                if (Q_UNLIKELY(!pNotebookItem)) {
                    QNWARNING(QStringLiteral("Detected null pointer to notebook item in notebook model item being one of those removed "
                                             "along with the stack item containing them; stack item: ") << *pChildItem);
                    continue;
                }

                QNTRACE(QStringLiteral("Removing notebook item under stack at ") << pNotebookItem
                        << QStringLiteral(": ") << *pNotebookItem);

                CHECK_NOTEBOOK_ITEM(pNotebookItem)
                notebookLocalUidsToRemove << pNotebookItem->localUid();
                QNTRACE(QStringLiteral("Marked notebook local uid ") << pNotebookItem->localUid()
                        << QStringLiteral(" as the one scheduled for removal"));
            }

            notebookStacksToRemove << pChildItem->notebookStackItem()->name();
            QNTRACE(QStringLiteral("Marked notebook stack ") << pChildItem->notebookStackItem()->name()
                    << QStringLiteral(" as the one scheduled for removal"));
        }
    }

    // Now we are sure all the items collected for the deletion can actually be deleted

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(int i = 0, size = notebookLocalUidsToRemove.size(); i < size; ++i)
    {
        const QString & localUid = notebookLocalUidsToRemove[i];
        QNTRACE(QStringLiteral("Processing notebook local uid ") << localUid
                << QStringLiteral(" scheduled for removal"));

        // WARNING: need to remove the model item before the underlying notebook item
        // to prevent the latter one to contain the dangling pointer to the removed
        // notebook item
        auto modelItemIt = m_modelItemsByLocalUid.find(localUid);
        if (Q_LIKELY(modelItemIt != m_modelItemsByLocalUid.end()))
        {
            const NotebookModelItem & modelItem = *modelItemIt;
            QNTRACE(QStringLiteral("Model item corresponding to the notebook: ") << modelItem);

            const NotebookModelItem * pParentItem = modelItem.parent();
            QNTRACE(QStringLiteral("Notebook's parent item at ") << pParentItem
                    << QStringLiteral(": ") << (pParentItem ? pParentItem->toString() : QStringLiteral("<null>")));

            removeModelItemFromParent(modelItem);

            QNTRACE(QStringLiteral("Model item corresponding to the notebook after parent removal: ") << modelItem
                    << QStringLiteral("\nParent item after removing the child notebook item from it: ") << pParentItem);

            if (pParentItem && (pParentItem != m_fakeRootItem) &&
                (pParentItem->type() == NotebookModelItem::Type::Stack) &&
                pParentItem->notebookStackItem() &&
                (pParentItem->numChildren() == 0))
            {
                const NotebookStackItem * pParentStackItem = pParentItem->notebookStackItem();

                QNDEBUG(QStringLiteral("The last child was removed from the stack item, "
                                       "need to remove it as well: stack = ") << pParentStackItem->name());
                notebookStacksToRemove << pParentStackItem->name();
                QNTRACE(QStringLiteral("Marked notebook stack ") << pParentStackItem->name()
                        << QStringLiteral(" as the one scheduled for removal"));
            }

            Q_UNUSED(m_modelItemsByLocalUid.erase(modelItemIt))
            QNTRACE(QStringLiteral("Erased the notebok model item corresponding to local uid ") << localUid);
        }
        else
        {
            QNWARNING(QStringLiteral("Internal error detected: can't find the notebook model item corresponding to the notebook item "
                                     "being removed: local uid = ") << localUid);
        }

        auto it = localUidIndex.find(localUid);
        if (Q_LIKELY(it != localUidIndex.end()))
        {
            Q_UNUSED(localUidIndex.erase(it))
            QNTRACE(QStringLiteral("Erased the notebok item corresponding to local uid ") << localUid);

            auto indexIt = m_indexIdToLocalUidBimap.right.find(localUid);
            if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
                m_indexIdToLocalUidBimap.right.erase(indexIt);
            }

            expungeNotebookFromLocalStorage(localUid);
        }
        else {
            QNWARNING(QStringLiteral("Internal error detected: can't find the notebook item to remove from the NotebookModel: local uid = ")
                      << localUid);
        }
    }

    QNTRACE(QStringLiteral("Finished removing the notebook items, now switching to removing the notebook stack items (if any)"));

    for(int i = 0, size = notebookStacksToRemove.size(); i < size; ++i)
    {
        const QString & stack = notebookStacksToRemove[i];
        QNTRACE(QStringLiteral("Processing notebook stack scheduled for removal: ") << stack);

        // WARNING: need to remove the model item before the underlying notebook stack item
        // to prevent the latter one to contain the dangling pointer to the removed
        // notebook stack item
        auto stackModelItemIt = m_modelItemsByStack.find(stack);
        if (Q_LIKELY(stackModelItemIt != m_modelItemsByStack.end()))
        {
            const NotebookModelItem & stackModelItem = *stackModelItemIt;
            QNTRACE(QStringLiteral("Notebook stack model item: ") << stackModelItem);

            int row = m_fakeRootItem->rowForChild(&stackModelItem);
            QNTRACE(QStringLiteral("Row for the notebook stack model item within the fake root item: ") << row
                    << QStringLiteral(", fake root item: ") << *m_fakeRootItem);

            removeModelItemFromParent(stackModelItem);

            QNTRACE(QStringLiteral("Notebook stack model item after parent removal: ") << stackModelItem
                    << QStringLiteral("\nFake root item after removing the child stack item from it: ") << *m_fakeRootItem);

            Q_UNUSED(m_modelItemsByStack.erase(stackModelItemIt))
            QNTRACE(QStringLiteral("Erased the notebook stack model item corresponding to stack ") << stack);

            auto indexIt = m_indexIdToStackBimap.right.find(stack);
            if (indexIt != m_indexIdToStackBimap.right.end()) {
                m_indexIdToStackBimap.right.erase(indexIt);
            }
        }
        else
        {
            QNWARNING(QStringLiteral("Internal error detected: can't find the notebook model item corresponding to the notebook stack item "
                                     "being removed from the notebook model: stack = ") << stack);
        }

        auto stackItemIt = m_stackItems.find(stack);
        if (Q_LIKELY(stackItemIt != m_stackItems.end())) {
            Q_UNUSED(m_stackItems.erase(stackItemIt))
            QNTRACE(QStringLiteral("Erased the notebook stack item corresponding to stack ") << stack);
        }
        else {
            QNWARNING(QStringLiteral("Internal error detected: can't find the notebook stack item being removed from the notebook model: stack = ")
                      << stack);
        }
    }

    QNDEBUG(QStringLiteral("Successfully removed the row(s)"));
    return true;
}

void NotebookModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(QStringLiteral("NotebookModel::sort: column = ") << column
            << QStringLiteral(", order = ") << order << QStringLiteral(" (")
            << (order == Qt::AscendingOrder ? QStringLiteral("ascending") : QStringLiteral("descending"))
            << QStringLiteral(")"));

    if (column != Columns::Name) {
        QNDEBUG(QStringLiteral("Only sorting by name is implemented at this time"));
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(QStringLiteral("The sort order already established, nothing to do"));
        return;
    }

    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    for(auto it = m_modelItemsByLocalUid.begin(), end = m_modelItemsByLocalUid.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }

    for(auto it = m_modelItemsByStack.begin(), end = m_modelItemsByStack.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }

    updatePersistentModelIndices();
    emit layoutChanged();
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
        return Q_NULLPTR;
    }

    const NotebookModelItem * pItem = itemForIndex(indexes.at(0));
    if (!pItem) {
        return Q_NULLPTR;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *pItem;

    QMimeData * pMimeData = new QMimeData;
    pMimeData->setData(NOTEBOOK_MODEL_MIME_TYPE, qCompress(encodedItem, NOTEBOOK_MODEL_MIME_DATA_MAX_COMPRESSION));
    return pMimeData;
}

bool NotebookModel::dropMimeData(const QMimeData * pMimeData, Qt::DropAction action,
                                 int row, int column, const QModelIndex & parentIndex)
{
    QNDEBUG(QStringLiteral("NotebookModel::dropMimeData: action = ") << action << QStringLiteral(", row = ") << row
            << QStringLiteral(", column = ") << column << QStringLiteral(", parent index: is valid = ")
            << (parentIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", parent row = ") << parentIndex.row() << QStringLiteral(", parent column = ")
            << (parentIndex.column()) << QStringLiteral(", internal id = ") << parentIndex.internalId()
            <<  QStringLiteral(", mime data formats: ")
            << (pMimeData ? pMimeData->formats().join(QStringLiteral("; ")) : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!pMimeData || !pMimeData->hasFormat(NOTEBOOK_MODEL_MIME_TYPE)) {
        return false;
    }

    const NotebookModelItem * pNewParentItem = itemForIndex(parentIndex);
    if (!pNewParentItem) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error, can't drop the notebook: no new parent item was found"));
        return false;
    }

    if ((pNewParentItem != m_fakeRootItem) && (pNewParentItem->type() != NotebookModelItem::Type::Stack)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't drop the notebook onto another notebook"));
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    QByteArray data = qUncompress(pMimeData->data(NOTEBOOK_MODEL_MIME_TYPE));
    NotebookModelItem item;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> item;

    if (item.type() != NotebookModelItem::Type::Notebook) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: the dropped item type is not a notebook"));
        return false;
    }

    auto it = m_modelItemsByLocalUid.find(item.notebookItem()->localUid());
    if (it == m_modelItemsByLocalUid.end()) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Internal error: failed to find the dropped model item by local uid in the notebook model"));
        return false;
    }

    const NotebookModelItem * originalParentItem = it->parent();
    int originalRow = -1;
    if (originalParentItem) {
        // Need to manually remove the item from its original parent
        originalRow = originalParentItem->rowForChild(&(*it));
    }

    if (originalRow >= 0) {
        QModelIndex originalParentIndex = indexForItem(originalParentItem);
        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(originalParentItem->takeChild(originalRow))
        endRemoveRows();
    }

    Q_UNUSED(m_modelItemsByLocalUid.erase(it))
    it = m_modelItemsByLocalUid.insert(item.notebookItem()->localUid(), item);

    beginInsertRows(parentIndex, row, row);

    pNewParentItem->insertChild(row, &(*it));

    NotebookItem itemCopy(*(item.notebookItem()));
    itemCopy.setStack(pNewParentItem->notebookStackItem()->name());
    itemCopy.setDirty(true);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto notebookItemIt = localUidIndex.find(item.notebookItem()->localUid());
    if (Q_LIKELY(notebookItemIt != localUidIndex.end())) {
        Q_UNUSED(localUidIndex.replace(notebookItemIt, itemCopy))
    }
    else {
        Q_UNUSED(localUidIndex.insert(itemCopy))
    }

    endInsertRows();

    updateItemRowWithRespectToSorting(*it);
    updateNotebookInLocalStorage(*(it->notebookItem()));
    return true;
}

void NotebookModel::onAllNotesListed()
{
    QNDEBUG(QStringLiteral("NotebookModel::onAllNotesListed"));

    NoteModel * pNoteModel = qobject_cast<NoteModel*>(sender());
    if (Q_UNLIKELY(!pNoteModel)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Internal error: received all notes listed event from some sender "
                                                       "which is not an instance of NoteModel"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::disconnect(pNoteModel, QNSIGNAL(NoteModel,notifyAllNotesListed),
                        this, QNSLOT(NotebookModel,onAllNotesListed));

    buildNotebookLocalUidByNoteLocalUidsHash(*pNoteModel);
}

void NotebookModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onAddNotebookComplete: notebook = ") << notebook << QStringLiteral("\nRequest id = ")
            << requestId);

    auto it = m_addNotebookRequestIds.find(requestId);
    if (it != m_addNotebookRequestIds.end()) {
        Q_UNUSED(m_addNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onAddNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNotebookRequestIds.find(requestId);
    if (it == m_addNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onAddNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_addNotebookRequestIds.erase(it))

    emit notifyError(errorDescription);

    removeItemByLocalUid(notebook.localUid());
}

void NotebookModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onUpdateNotebookComplete: notebook = ") << notebook << QStringLiteral("\nRequest id = ")
            << requestId);

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it != m_updateNotebookRequestIds.end()) {
        Q_UNUSED(m_updateNotebookRequestIds.erase(it))
        return;
    }

    onNotebookAddedOrUpdated(notebook);
}

void NotebookModel::onUpdateNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onUpdateNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_updateNotebookRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to find the notebook: local uid = ") << notebook.localUid()
            << QStringLiteral(", request id = ") << requestId);
    emit findNotebook(notebook, requestId);
}

void NotebookModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onFindNotebookComplete: notebook = ") << notebook << QStringLiteral("\nRequest id = ") << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onNotebookAddedOrUpdated(notebook);
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(notebook.localUid(), notebook);
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(notebook.localUid());
        if (it != localUidIndex.end()) {
            updateNotebookInLocalStorage(*it);
        }
    }
}

void NotebookModel::onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findNotebookToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findNotebookToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findNotebookToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findNotebookToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onFindNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    if (restoreUpdateIt != m_findNotebookToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findNotebookToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void NotebookModel::onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListNotebooksOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString linkedNotebookGuid, QList<Notebook> foundNotebooks, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onListNotebooksComplete: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
            << QStringLiteral(", direction = ") << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid) << QStringLiteral(", num found notebooks = ")
            << foundNotebooks.size() << QStringLiteral(", request id = ") << requestId);

    for(auto it = foundNotebooks.constBegin(), end = foundNotebooks.constEnd(); it != end; ++it) {
        onNotebookAddedOrUpdated(*it);
        requestNoteCountForNotebook(*it);
    }

    m_listNotebooksRequestId = QUuid();

    if (foundNotebooks.size() == static_cast<int>(limit)) {
        QNTRACE(QStringLiteral("The number of found notebooks matches the limit, requesting more notebooks from the local storage"));
        m_listNotebooksOffset += limit;
        requestNotebooksList();
        return;
    }

    m_allNotebooksListed = true;
    emit notifyAllNotebooksListed();
    emit notifyAllItemsListed();
}

void NotebookModel::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListNotebooksOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onListNotebooksFailed: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
            << QStringLiteral(", direction = ") << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid) << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listNotebooksRequestId = QUuid();

    emit notifyError(errorDescription);
}

void NotebookModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onExpungeNotebookComplete: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it != m_expungeNotebookRequestIds.end()) {
        Q_UNUSED(m_expungeNotebookRequestIds.erase(it))
        return;
    }

    emit aboutToRemoveNotebooks();
    removeItemByLocalUid(notebook.localUid());
    emit removedNotebooks();
}

void NotebookModel::onExpungeNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeNotebookRequestIds.find(requestId);
    if (it == m_expungeNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onExpungeNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_expungeNotebookRequestIds.erase(it))

    onNotebookAddedOrUpdated(notebook);
    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onGetNoteCountPerNotebookComplete(int noteCount, Notebook notebook, QUuid requestId)
{
    auto it = m_noteCountPerNotebookRequestIds.find(requestId);
    if (it == m_noteCountPerNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NotebookModel::onGetNoteCountPerNotebookComplete: note count = ") << noteCount
            << QStringLiteral(", notebook = ") << notebook << QStringLiteral("\nRequest id = ") << requestId);

    Q_UNUSED(m_noteCountPerNotebookRequestIds.erase(it))

    QString notebookLocalUid = notebook.localUid();

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Can't find the notebook item by local uid: ") << notebookLocalUid);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNumNotesPerNotebook(noteCount);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onGetNoteCountPerNotebookFailed(ErrorString errorDescription, Notebook notebook, QUuid requestId)
{
    auto it = m_noteCountPerNotebookRequestIds.find(requestId);
    if (it == m_noteCountPerNotebookRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("NotebookModel::onGetNoteCountPerNotebookFailed: error description = ") << errorDescription
              << QStringLiteral(", notebook: ") << notebook << QStringLiteral("\nRequest id = ") << requestId);

    Q_UNUSED(m_noteCountPerNotebookRequestIds.erase(it))

    // Not much can be done here - will just attempt ot "remove" the count from the item

    QString notebookLocalUid = notebook.localUid();

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Can't find the notebook item by local uid: ") << notebookLocalUid);
        return;
    }

    NotebookItem item = *itemIt;
    item.setNumNotesPerNotebook(-1);

    Q_UNUSED(updateNoteCountPerNotebookIndex(item, itemIt))
}

void NotebookModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onAddNoteComplete: note = ") << note
            << QStringLiteral(", request id = ") << requestId);

    if (note.hasNotebookLocalUid())
    {
        if (m_receivedNotebookLocalUidsForAllNotes) {
            m_notebookLocalUidByNoteLocalUid[note.localUid()] = note.notebookLocalUid();
        }

        bool res = onAddNoteWithNotebookLocalUid(note.notebookLocalUid());
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
        QNDEBUG(QStringLiteral("Added note has no notebook local uid and no notebook guid, re-requesting the note count for all notebooks"));
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onUpdateNoteComplete: note = ") << note
            << QStringLiteral("\nUpdate resources = ") << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", update tags = ") << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", request id = ") << requestId);

    if (!m_receivedNotebookLocalUidsForAllNotes || !note.hasNotebookLocalUid()) {
        // It's quite unlikely the note update was about moving it to another notebook but as long as there's no way to
        // check it at this point, will re-request the note count for all notebooks
        requestNoteCountForAllNotebooks();
        return;
    }

    const QString & newNotebookLocalUid = note.notebookLocalUid();

    QString oldNotebookLocalUid;
    auto it = m_notebookLocalUidByNoteLocalUid.find(note.localUid());
    if (it != m_notebookLocalUidByNoteLocalUid.end()) {
        oldNotebookLocalUid = it.value();
    }

    if (oldNotebookLocalUid == newNotebookLocalUid) {
        QNDEBUG(QStringLiteral("The note's notebook local uid hasn't changed: ") << oldNotebookLocalUid);
        return;
    }

    // Update the note local uid to notebook local uid hash at this point because we might need to return early further
    if (it != m_notebookLocalUidByNoteLocalUid.end()) {
        it.value() = newNotebookLocalUid;
    }
    else {
        m_notebookLocalUidByNoteLocalUid[note.localUid()] = newNotebookLocalUid;
    }

    if (oldNotebookLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Can't determine the previous notebook of this note, fallback to re-requesting the note counts "
                               "for all notebooks"));
        requestNoteCountForAllNotebooks();
        return;
    }

    QNDEBUG(QStringLiteral("The note's notebook local uid has changed: was ") << oldNotebookLocalUid
            << QStringLiteral(", now ") << newNotebookLocalUid);

    Notebook oldNotebook;
    oldNotebook.setLocalUid(oldNotebookLocalUid);
    requestNoteCountForNotebook(oldNotebook);

    Notebook newNotebook;
    newNotebook.setLocalUid(newNotebookLocalUid);
    requestNoteCountForNotebook(newNotebook);
}

void NotebookModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModel::onExpungeNoteComplete: note = ") << note
            << QStringLiteral("\nRequest id = ") << requestId);

    QString notebookLocalUid;
    if (note.hasNotebookLocalUid())
    {
        notebookLocalUid = note.notebookLocalUid();
    }
    else if (m_receivedNotebookLocalUidsForAllNotes)
    {
        auto it = m_notebookLocalUidByNoteLocalUid.find(note.localUid());
        if (it != m_notebookLocalUidByNoteLocalUid.end()) {
            notebookLocalUid = it.value();
            Q_UNUSED(m_notebookLocalUidByNoteLocalUid.erase(it))
        }
    }

    if (!notebookLocalUid.isEmpty())
    {
        bool res = onExpungeNoteWithNotebookLocalUid(note.notebookLocalUid());
        if (res) {
            return;
        }
    }

    Notebook notebook;
    if (!notebookLocalUid.isEmpty()) {
        notebook.setLocalUid(notebookLocalUid);
    }
    else if (note.hasNotebookGuid()) {
        notebook.setGuid(note.notebookGuid());
    }
    else {
        QNDEBUG(QStringLiteral("Expunged note has no notebook local uid and no notebook guid, re-requesting the note count for all notebooks"));
        requestNoteCountForAllNotebooks();
        return;
    }

    requestNoteCountForNotebook(notebook);
}

void NotebookModel::createConnections(const NoteModel & noteModel, LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG(QStringLiteral("NotebookModel::createConnections"));

    if (!noteModel.allNotesListed()) {
        QObject::connect(&noteModel, QNSIGNAL(NoteModel,notifyAllNotesListed),
                         this, QNSLOT(NotebookModel,onAllNotesListed));
    }

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this, QNSIGNAL(NotebookModel,addNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,updateNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,listNotebooks,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                    LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onListNotebooksRequest,
                                                       LocalStorageManager::ListObjectsOptions,
                                                       size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,expungeNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onExpungeNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,requestNoteCountPerNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onGetNoteCountPerNotebookRequest,Notebook,QUuid));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listNotebooksComplete,
                                                         LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QString,
                                                         QList<Notebook>,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listNotebooksFailed,
                                                         LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,
                                                         QString,ErrorString,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(NotebookModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountPerNotebookComplete,int,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onGetNoteCountPerNotebookComplete,int,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountPerNotebookFailed,ErrorString,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onGetNoteCountPerNotebookFailed,ErrorString,Notebook,QUuid));
}

void NotebookModel::requestNotebooksList()
{
    QNDEBUG(QStringLiteral("NotebookModel::requestNotebooksList: offset = ") << m_listNotebooksOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListNotebooksOrder::type order = LocalStorageManager::ListNotebooksOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listNotebooksRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to list notebooks: offset = ") << m_listNotebooksOffset << QStringLiteral(", request id = ")
            << m_listNotebooksRequestId);
    emit listNotebooks(flags, NOTEBOOK_LIST_LIMIT, m_listNotebooksOffset, order, direction, QString(), m_listNotebooksRequestId);
}

void NotebookModel::requestNoteCountForNotebook(const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("NotebookModel::requestNoteCountForNotebook: ") << notebook);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerNotebookRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting request to get the note count per notebook: request id = ") << requestId);
    emit requestNoteCountPerNotebook(notebook, requestId);
}

void NotebookModel::requestNoteCountForAllNotebooks()
{
    QNDEBUG(QStringLiteral("NotebookModel::requestNoteCountForAllNotebooks"));

    const NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it) {
        const NotebookItem & item = *it;
        Notebook notebook;
        notebook.setLocalUid(item.localUid());
        requestNoteCountForNotebook(notebook);
    }
}

QVariant NotebookModel::dataImpl(const NotebookModelItem & item, const Columns::type column) const
{
    bool isNotebookItem = (item.type() == NotebookModelItem::Type::Notebook);
    if (Q_UNLIKELY((isNotebookItem && !item.notebookItem()) || (!isNotebookItem && !item.notebookStackItem()))) {
        QNWARNING(QStringLiteral("Detected null pointer to ") << (isNotebookItem ? QStringLiteral("notebook") : QStringLiteral("stack"))
                  << QStringLiteral(" item inside the noteobok model item"));
        return QVariant();
    }

    switch(column)
    {
    case Columns::Name:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->name());
            }
            else {
                return QVariant(item.notebookStackItem()->name());
            }
        }
    case Columns::Synchronizable:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isSynchronizable());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Dirty:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isDirty());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Default:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isDefault());
            }
            else {
                return QVariant();
            }
        }
    case Columns::LastUsed:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isLastUsed());
            }
            else {
                return QVariant();
            }
        }
    case Columns::Published:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->isPublished());
            }
            else {
                return QVariant();
            }
        }
    case Columns::FromLinkedNotebook:
        {
            if (isNotebookItem) {
                return QVariant( !(item.notebookItem()->linkedNotebookGuid().isEmpty()) );
            }
            else {
                return QVariant();
            }
        }
    case Columns::NumNotesPerNotebook:
        {
            if (isNotebookItem) {
                return QVariant(item.notebookItem()->numNotesPerNotebook());
            }
            else {
                return QVariant();
            }
        }
    default:
        return QVariant();
    }
}

QVariant NotebookModel::dataAccessibleText(const NotebookModelItem & item, const Columns::type column) const
{
    bool isNotebookItem = (item.type() == NotebookModelItem::Type::Notebook);
    if (Q_UNLIKELY((isNotebookItem && !item.notebookItem()) || (!isNotebookItem && !item.notebookStackItem()))) {
        QNWARNING(QStringLiteral("Detected null pointer to ") << (isNotebookItem ? QStringLiteral("notebook") : QStringLiteral("stack"))
                  << QStringLiteral(" item inside the noteobok model item"));
        return QVariant();
    }

    QVariant textData = dataImpl(item, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = (isNotebookItem ? tr("Notebook: ") : tr("Notebook stack: "));

    switch(column)
    {
    case Columns::Name:
        accessibleText += tr("name is ") + textData.toString();
        break;
    case Columns::Synchronizable:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("synchronizable") : tr("not synchronizable"));
            break;
        }
    case Columns::Dirty:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("dirty") : tr("not dirty"));
            break;
        }
    case Columns::Default:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("default") : tr("not default"));
            break;
        }
    case Columns::LastUsed:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("last used") : tr("not last used"));
            break;
        }
    case Columns::Published:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("published") : tr("not published"));
            break;
        }
    case Columns::FromLinkedNotebook:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            accessibleText += (textData.toBool() ? tr("from linked notebook") : tr("from own account"));
            break;
        }
    case Columns::NumNotesPerNotebook:
        {
            if (!isNotebookItem) {
                return QVariant();
            }

            int numNotesPerNotebook = textData.toInt();
            accessibleText += tr("number of notes per notebook") + QStringLiteral(": ") + QString::number(numNotesPerNotebook);
            break;
        }
    default:
        return QVariant();
    }

    return QVariant(accessibleText);
}

bool NotebookModel::canUpdateNotebookItem(const NotebookItem & item) const
{
    return item.isUpdatable();
}

void NotebookModel::updateNotebookInLocalStorage(const NotebookItem & item)
{
    QNDEBUG(QStringLiteral("NotebookModel::updateNotebookInLocalStorage: local uid = ") << item.localUid());

    Notebook notebook;

    auto notYetSavedItemIt = m_notebookItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_notebookItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG(QStringLiteral("Updating the notebook"));

        const Notebook * pCachedNotebook = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedNotebook))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findNotebookToPerformUpdateRequestIds.insert(requestId))
            Notebook dummy;
            dummy.setLocalUid(item.localUid());
            emit findNotebook(dummy, requestId);
            QNTRACE(QStringLiteral("Emitted the request to find the notebook: local uid = ") << item.localUid()
                    << QStringLiteral(", request id = ") << requestId);
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

    // If all item's properties related to the restrictions are "true", there's no real need for the restrictions;
    // Otherwise need to set the restrictions
    if (!item.canCreateNotes() || !item.canUpdateNotes() || !item.isUpdatable() || !item.nameIsUpdatable())
    {
        notebook.setCanCreateNotes(item.canCreateNotes());
        notebook.setCanUpdateNotes(item.canUpdateNotes());
        notebook.setCanUpdateNotebook(item.isUpdatable());
        notebook.setCanRenameNotebook(item.nameIsUpdatable());
    }

    m_cache.put(item.localUid(), notebook);

    // NOTE: deliberately not setting the updatable property from the item
    // as it can't be changed by the model and only serves the utilitary purposes

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_notebookItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addNotebookRequestIds.insert(requestId))

        QNTRACE(QStringLiteral("Emitting the request to add the notebook to local storage: id = ")
                << requestId << QStringLiteral(", notebook = ") << notebook);
        emit addNotebook(notebook, requestId);

        Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateNotebookRequestIds.insert(requestId))

        // While the notebook is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(notebook.localUid()))

        QNTRACE(QStringLiteral("Emitting the request to update notebook in the local storage: id = ")
                << requestId << QStringLiteral(", notebook = ") << notebook);
        emit updateNotebook(notebook, requestId);
    }
}

void NotebookModel::expungeNotebookFromLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::expungeNotebookFromLocalStorage: local uid = ")
            << localUid);

    Notebook dummyNotebook;
    dummyNotebook.setLocalUid(localUid);

    Q_UNUSED(m_cache.remove(localUid))

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_expungeNotebookRequestIds.insert(requestId))
    QNDEBUG(QStringLiteral("Emitting the request to expunge the notebook from local storage: "
                           "request id = ") << requestId
            << QStringLiteral(", local uid = ") << localUid);
    emit expungeNotebook(dummyNotebook, requestId);
}

QString NotebookModel::nameForNewNotebook() const
{
    QString baseName = tr("New notebook");
    const NotebookDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    return newItemName<NotebookDataByNameUpper>(nameIndex, m_lastNewNotebookNameCounter, baseName);
}

NotebookModel::RemoveRowsScopeGuard::RemoveRowsScopeGuard(NotebookModel & model) :
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
    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(notebook.localUid(), notebook);

    auto itemIt = localUidIndex.find(notebook.localUid());
    bool newNotebook = (itemIt == localUidIndex.end());
    if (newNotebook)
    {
        emit aboutToAddNotebook();

        onNotebookAdded(notebook);

        QModelIndex addedNotebookIndex = indexForLocalUid(notebook.localUid());
        emit addedNotebook(addedNotebookIndex);
    }
    else
    {
        QModelIndex notebookIndexBefore = indexForLocalUid(notebook.localUid());
        emit aboutToUpdateNotebook(notebookIndexBefore);

        onNotebookUpdated(notebook, itemIt);

        QModelIndex notebookIndexAfter = indexForLocalUid(notebook.localUid());
        emit updatedNotebook(notebookIndexAfter);
    }
}

void NotebookModel::onNotebookAdded(const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("NotebookModel::onNotebookAdded: notebook local uid = ") << notebook.localUid());

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const NotebookModelItem * pParentItem = Q_NULLPTR;

    if (notebook.hasStack())
    {
        const QString & stack = notebook.stack();
        auto it = m_modelItemsByStack.find(stack);
        if (it == m_modelItemsByStack.end()) {
            auto stackItemIt = m_stackItems.insert(stack, NotebookStackItem(stack));
            it = addNewStackModelItem(stackItemIt.value());
        }

        pParentItem = &(*it);
    }
    else
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new NotebookModelItem;
        }

        pParentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    NotebookItem item;
    notebookToItem(notebook, item);

    int row = pParentItem->numChildren();

    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    const NotebookItem * insertedItem = &(*it);

    auto modelItemIt = m_modelItemsByLocalUid.insert(item.localUid(),
                                                     NotebookModelItem(NotebookModelItem::Type::Notebook,
                                                                       insertedItem, Q_NULLPTR));
    const NotebookModelItem * insertedModelItem = &(*modelItemIt);

    beginInsertRows(parentIndex, row, row);
    insertedModelItem->setParent(pParentItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*insertedModelItem);
}

void NotebookModel::onNotebookUpdated(const Notebook & notebook, NotebookDataByLocalUid::iterator it)
{
    QNDEBUG(QStringLiteral("NotebookModel::onNotebookUpdated: notebook local uid = ")
            << notebook.localUid());

    NotebookItem notebookItemCopy;
    notebookToItem(notebook, notebookItemCopy);

    auto modelItemIt = m_modelItemsByLocalUid.find(notebook.localUid());
    if (Q_UNLIKELY(modelItemIt == m_modelItemsByLocalUid.end())) {
        QNWARNING(QStringLiteral("Can't find the notebook model item corresponding to the updated notebook item: ")
                  << notebookItemCopy);
        return;
    }

    const NotebookModelItem * pModelItem = &(modelItemIt.value());

    const NotebookModelItem * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(QStringLiteral("Can't find the parent notebook model item for updated notebook item: ") << *pModelItem);
        return;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Can't find the row of the child notebook model item within its parent model item: parent item = ")
                  << *pParentItem << QStringLiteral("\nChild item = ") << *pModelItem);
        return;
    }

    bool shouldChangeParent = false;

    QString previousStackName;
    if (pParentItem->type() == NotebookModelItem::Type::Stack)
    {
        const NotebookStackItem * parentStackItem = pParentItem->notebookStackItem();
        if (Q_UNLIKELY(!parentStackItem)) {
            QNWARNING(QStringLiteral("Detected null pointer to notebook stack item in the parent item for the updated notebook item: ")
                      << *pParentItem);
            return;
        }

        previousStackName = parentStackItem->name();
    }

    if (!notebook.hasStack() && !previousStackName.isEmpty())
    {
        // Need to remove the notebook model item from the current stack and set the fake root item as the parent
        QModelIndex parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        if (!m_fakeRootItem) {
            m_fakeRootItem = new NotebookModelItem;
        }

        pParentItem = m_fakeRootItem;
        shouldChangeParent = true;
    }
    else if (notebook.hasStack() && (notebook.stack() != previousStackName))
    {
        // Need to remove the notebook from its current parent and insert it under the stack item,
        // either existing or new
        QModelIndex parentIndex = indexForItem(pParentItem);
        beginRemoveRows(parentIndex, row, row);
        Q_UNUSED(pParentItem->takeChild(row))
        endRemoveRows();

        auto stackModelItemIt = m_modelItemsByStack.find(notebook.stack());
        if (stackModelItemIt == m_modelItemsByStack.end())
        {
            auto stackItemIt = m_stackItems.insert(notebook.stack(), NotebookStackItem(notebook.stack()));
            const NotebookStackItem * pNewStackItem = &(stackItemIt.value());

            if (!m_fakeRootItem) {
                m_fakeRootItem = new NotebookModelItem;
            }

            NotebookModelItem newStackModelItem(NotebookModelItem::Type::Stack, Q_NULLPTR, pNewStackItem);
            stackModelItemIt = m_modelItemsByStack.insert(notebook.stack(), newStackModelItem);
            int newRow = m_fakeRootItem->numChildren();
            beginInsertRows(QModelIndex(), newRow, newRow);
            stackModelItemIt->setParent(m_fakeRootItem);
            endInsertRows();

            updateItemRowWithRespectToSorting(*stackModelItemIt);
        }

        pParentItem = &(stackModelItemIt.value());
        shouldChangeParent = true;
    }

    bool notebookStackChanged = false;
    if (shouldChangeParent)
    {
        QModelIndex parentItemIndex = indexForItem(pParentItem);
        int newRow = pParentItem->numChildren();
        beginInsertRows(parentItemIndex, newRow, newRow);
        pModelItem->setParent(pParentItem);
        endInsertRows();

        row = pParentItem->rowForChild(pModelItem);
        if (Q_UNLIKELY(row < 0)) {
            QNWARNING(QStringLiteral("Can't find the row for the child notebook item within its parent right after setting "
                                     "the parent item to the child item! Parent item = ") << *pParentItem
                      << QStringLiteral("\nChild item = ") << *pModelItem);
            return;
        }

        notebookStackChanged = true;
    }

    int numNotesPerNotebook = it->numNotesPerNotebook();
    notebookItemCopy.setNumNotesPerNotebook(numNotesPerNotebook);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, notebookItemCopy);

    IndexId modelItemId = idForItem(*pModelItem);

    QModelIndex modelIndexFrom = createIndex(row, 0, modelItemId);
    QModelIndex modelIndexTo = createIndex(row, NUM_NOTEBOOK_MODEL_COLUMNS - 1, modelItemId);
    emit dataChanged(modelIndexFrom, modelIndexTo);

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG(QStringLiteral("Not sorting by name, returning"));
        return;
    }

    updateItemRowWithRespectToSorting(*pModelItem);

    if (notebookStackChanged) {
        emit notifyNotebookStackChanged(modelIndexFrom);
    }
}

NotebookModel::ModelItems::iterator NotebookModel::addNewStackModelItem(const NotebookStackItem & stackItem)
{
    QNDEBUG(QStringLiteral("NotebookModel::addNewStackModelItem: stack item = ") << stackItem);

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    NotebookModelItem newStackItem(NotebookModelItem::Type::Stack, Q_NULLPTR, &stackItem);
    QNTRACE(QStringLiteral("Created new stack model item: ") << newStackItem);
    auto it = m_modelItemsByStack.insert(stackItem.name(), newStackItem);

    int row = rowForNewItem(*m_fakeRootItem, newStackItem);
    QNTRACE(QStringLiteral("Will insert the new item at row ") << row);

    beginInsertRows(QModelIndex(), row, row);
    m_fakeRootItem->insertChild(row, &(it.value()));
    endInsertRows();

    updateItemRowWithRespectToSorting(it.value());
    QNTRACE(QStringLiteral("New stack model item after updating the item row "
                           "with respect to sorting: ")
            << it.value() << QStringLiteral("\nFake root item: ") << *m_fakeRootItem);
    return it;
}

void NotebookModel::removeItemByLocalUid(const QString & localUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::removeItemByLocalUid: local uid = ") << localUid);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Can't find item to remove from the notebook model"));
        return;
    }

    const NotebookItem & item = *itemIt;

    auto notebookModelItemIt = m_modelItemsByLocalUid.find(item.localUid());
    if (Q_UNLIKELY(notebookModelItemIt == m_modelItemsByLocalUid.end())) {
        QNWARNING(QStringLiteral("Can't find the notebook model item corresponding the the notebook item with local uid ")
                  << item.localUid());
        return;
    }

    const NotebookModelItem * pModelItem = &(*notebookModelItemIt);
    const NotebookModelItem * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(QStringLiteral("Can't find the parent notebook model item for the notebook being removed from the model: local uid = ")
                  << item.localUid());
        return;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_LIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Can't find the notebook item's row within its parent model item: notebook item = ") << *pModelItem
                  << QStringLiteral("\nStack item = ") << *pParentItem);
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
}

void NotebookModel::notebookToItem(const Notebook & notebook, NotebookItem & item)
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
        const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();
        item.setUpdatable(!restrictions.noUpdateNotebook.isSet() || !restrictions.noUpdateNotebook.ref());
        item.setNameIsUpdatable(!restrictions.noRenameNotebook.isSet() || !restrictions.noRenameNotebook.ref());
        item.setCanCreateNotes(!restrictions.noCreateNotes.isSet() || !restrictions.noCreateNotes.ref());
        item.setCanUpdateNotes(!restrictions.noUpdateNotes.isSet() || !restrictions.noUpdateNotes.ref());
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

void NotebookModel::removeModelItemFromParent(const NotebookModelItem & modelItem)
{
    QNDEBUG(QStringLiteral("NotebookModel::removeModelItemFromParent: ") << modelItem);

    const NotebookModelItem * pParentItem = modelItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNDEBUG(QStringLiteral("No parent item, nothing to do"));
        return;
    }

    QNTRACE(QStringLiteral("Parent item: ") << *pParentItem);
    int row = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Can't find the child notebook model item's row within its parent; child item = ")
                  << modelItem << QStringLiteral(", parent item = ") << *pParentItem);
        return;
    }

    QNTRACE(QStringLiteral("Will remove the child at row ") << row);

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();
}

int NotebookModel::rowForNewItem(const NotebookModelItem & parentItem, const NotebookModelItem & newItem) const
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return parentItem.numChildren();
    }

    QList<const NotebookModelItem*> children = parentItem.children();
    auto it = children.end();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(children.begin(), children.end(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(children.begin(), children.end(), &newItem, GreaterByName());
    }

    if (it == children.end()) {
        return parentItem.numChildren();
    }

    int row = static_cast<int>(std::distance(children.begin(), it));
    return row;
}

void NotebookModel::updateItemRowWithRespectToSorting(const NotebookModelItem & modelItem)
{
    QNTRACE(QStringLiteral("NotebookModel::updateItemRowWithRespectToSorting"));

    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const NotebookModelItem * pParentItem = modelItem.parent();
    if (!pParentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new NotebookModelItem;
        }

        pParentItem = m_fakeRootItem;
        int row = rowForNewItem(*pParentItem, modelItem);

        beginInsertRows(QModelIndex(), row, row);
        pParentItem->insertChild(row, &modelItem);
        endInsertRows();
        return;
    }

    int currentItemRow = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(QStringLiteral("Can't update notebook model item's row: can't find its original row within parent: ")
                  << modelItem);
        return;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(pParentItem->takeChild(currentItemRow))
    endRemoveRows();

    int appropriateRow = rowForNewItem(*pParentItem, modelItem);

    beginInsertRows(parentIndex, appropriateRow, appropriateRow);
    pParentItem->insertChild(appropriateRow, &modelItem);
    endInsertRows();

    QNTRACE(QStringLiteral("Moved item from row ") << currentItemRow << QStringLiteral(" to row ") << appropriateRow
            << QStringLiteral("; item: ") << modelItem);
}

void NotebookModel::updatePersistentModelIndices()
{
    QNDEBUG(QStringLiteral("NotebookModel::updatePersistentModelIndices"));

    // Ensure any persistent model indices would be updated appropriately
    QModelIndexList indices = persistentIndexList();
    for(auto it = indices.begin(), end = indices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        const NotebookModelItem * pItem = itemForId(static_cast<IndexId>(index.internalId()));
        QModelIndex replacementIndex = indexForItem(pItem);
        changePersistentIndex(index, replacementIndex);
    }
}

bool NotebookModel::onAddNoteWithNotebookLocalUid(const QString & notebookLocalUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::onAddNoteWithNotebookLocalUid: ") << notebookLocalUid);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Wasn't able to find the notebook item by the specified local uid"));
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.numNotesPerNotebook();
    ++noteCount;
    item.setNumNotesPerNotebook(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

bool NotebookModel::onExpungeNoteWithNotebookLocalUid(const QString & notebookLocalUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::onExpungeNoteWithNotebookLocalUid: ") << notebookLocalUid);

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(notebookLocalUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Wasn't able to find the notebook item by the specified local uid"));
        return false;
    }

    NotebookItem item = *it;
    int noteCount = item.numNotesPerNotebook();
    --noteCount;
    noteCount = std::max(noteCount, 0);
    item.setNumNotesPerNotebook(noteCount);

    return updateNoteCountPerNotebookIndex(item, it);
}

void NotebookModel::switchDefaultNotebookLocalUid(const QString & localUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::switchDefaultNotebookLocalUid: ") << localUid);

    if (!m_defaultNotebookLocalUid.isEmpty())
    {
        QNTRACE(QStringLiteral("There has already been some notebook chosen as the default, "
                               "switching the default flag off for it"));

        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

        auto previousDefaultItemIt = localUidIndex.find(m_defaultNotebookLocalUid);
        if (previousDefaultItemIt != localUidIndex.end())
        {
            NotebookItem previousDefaultItemCopy(*previousDefaultItemIt);
            QNTRACE(QStringLiteral("Previous default notebook item: ") << previousDefaultItemCopy);

            previousDefaultItemCopy.setDefault(false);
            bool wasDirty = previousDefaultItemCopy.isDirty();
            previousDefaultItemCopy.setDirty(true);

            localUidIndex.replace(previousDefaultItemIt, previousDefaultItemCopy);

            QModelIndex previousDefaultItemIndex = indexForLocalUid(m_defaultNotebookLocalUid);
            emit dataChanged(previousDefaultItemIndex, previousDefaultItemIndex);

            if (!wasDirty) {
                previousDefaultItemIndex = index(previousDefaultItemIndex.row(), Columns::Dirty, previousDefaultItemIndex.parent());
                emit dataChanged(previousDefaultItemIndex, previousDefaultItemIndex);
            }

            updateNotebookInLocalStorage(previousDefaultItemCopy);
        }
    }

    m_defaultNotebookLocalUid = localUid;
    QNTRACE(QStringLiteral("Set default notebook local uid to ") << localUid);
}

void NotebookModel::switchLastUsedNotebookLocalUid(const QString & localUid)
{
    QNDEBUG(QStringLiteral("NotebookModel::setLastUsedNotebook: ") << localUid);

    if (!m_lastUsedNotebookLocalUid.isEmpty())
    {
        QNTRACE(QStringLiteral("There has already been some notebook chosen as the last used one, "
                               "switching the last used flag off for it"));

        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

        auto previousLastUsedItemIt = localUidIndex.find(m_lastUsedNotebookLocalUid);
        if (previousLastUsedItemIt != localUidIndex.end())
        {
            NotebookItem previousLastUsedItemCopy(*previousLastUsedItemIt);
            QNTRACE(QStringLiteral("Previous last used notebook item: ") << previousLastUsedItemCopy);

            previousLastUsedItemCopy.setLastUsed(false);
            bool wasDirty = previousLastUsedItemCopy.isDirty();
            previousLastUsedItemCopy.setDirty(true);

            localUidIndex.replace(previousLastUsedItemIt, previousLastUsedItemCopy);

            QModelIndex previousLastUsedItemIndex = indexForLocalUid(m_lastUsedNotebookLocalUid);
            emit dataChanged(previousLastUsedItemIndex, previousLastUsedItemIndex);

            if (!wasDirty) {
                previousLastUsedItemIndex = index(previousLastUsedItemIndex.row(), Columns::Dirty, previousLastUsedItemIndex.parent());
                emit dataChanged(previousLastUsedItemIndex, previousLastUsedItemIndex);
            }

            updateNotebookInLocalStorage(previousLastUsedItemCopy);
        }
    }

    m_lastUsedNotebookLocalUid = localUid;
    QNTRACE(QStringLiteral("Set last used notebook local uid to ") << localUid);
}

void NotebookModel::checkAndRemoveEmptyStackItem(const NotebookModelItem & modelItem)
{
    QNDEBUG(QStringLiteral("NotebookModel::checkAndRemoveEmptyStackItem: ") << modelItem);

    if ((&modelItem != m_fakeRootItem) &&
        (modelItem.type() == NotebookModelItem::Type::Stack) &&
        modelItem.notebookStackItem() &&
        (modelItem.numChildren() == 0))
    {
        QNDEBUG(QStringLiteral("Removed the last child of the previous notebook's stack, "
                               "need to remove the stack item itself"));

        const NotebookStackItem * pStackItem = modelItem.notebookStackItem();
        QString previousStack = pStackItem->name();

        removeModelItemFromParent(modelItem);

        auto it = m_modelItemsByStack.find(previousStack);
        if (it != m_modelItemsByStack.end())
        {
            Q_UNUSED(m_modelItemsByStack.erase(it))

            auto stackIt = m_stackItems.find(previousStack);
            if (stackIt != m_stackItems.end()) {
                Q_UNUSED(m_stackItems.erase(stackIt))
            }
            else {
                QNWARNING(QStringLiteral("Can't find stack item to remove, stack ") << previousStack);
            }
        }
        else {
            QNWARNING(QStringLiteral("Can't find stack model item to remove, stack ") << previousStack);
        }
    }
}

void NotebookModel::setNotebookFavorited(const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set favorited flag for the notebook: the model index is invalid"));
        return;
    }

    const NotebookModelItem * pModelItem = itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set favorited flag for the notebook: failed to find the model item "
                                       "corresponding to the model index"));
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != NotebookModelItem::Type::Notebook)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set favorited flag for the notebook: the item attempted to be favorited "
                                       "or unfavorited is not a notebook"));
        return;
    }

    const NotebookItem * pNotebookItem = pModelItem->notebookItem();
    if (Q_UNLIKELY(!pNotebookItem)) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set favorited flag for the notebook: internal error, the model item "
                                       "has null pointer to the notebook item"));
        return;
    }

    if (favorited == pNotebookItem->isFavorited()) {
        QNDEBUG(QStringLiteral("Favorited flag's value hasn't changed"));
        return;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(pNotebookItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TRANSLATE_NOOP("", "Can't set favorited flag for the notebook: the notebook item was not found within the model"));
        return;
    }

    NotebookItem itemCopy(*pNotebookItem);
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the notebook as dirty as favorited property is not included into the synchronization protocol

    localUidIndex.replace(it, itemCopy);

    updateNotebookInLocalStorage(itemCopy);
}

void NotebookModel::beginRemoveNotebooks()
{
    emit aboutToRemoveNotebooks();
}

void NotebookModel::endRemoveNotebooks()
{
    emit removedNotebooks();
}

void NotebookModel::buildNotebookLocalUidByNoteLocalUidsHash(const NoteModel & noteModel)
{
    QNDEBUG(QStringLiteral("NotebookModel::buildNotebookLocalUidByNoteLocalUidsHash"));

    m_notebookLocalUidByNoteLocalUid.clear();

    int numNotes = noteModel.rowCount(QModelIndex());
    for(int i = 0; i < numNotes; ++i)
    {
        const NoteModelItem * pNoteModelItem = noteModel.itemAtRow(i);
        if (Q_UNLIKELY(!pNoteModelItem)) {
            QNWARNING(QStringLiteral("Can't find note model item at row ") << i << QStringLiteral(" even though there are ")
                      << numNotes << QStringLiteral(" rows within the model"));
            continue;
        }

        const QString & notebookLocalUid = pNoteModelItem->notebookLocalUid();
        if (Q_UNLIKELY(notebookLocalUid.isEmpty())) {
            QNWARNING(QStringLiteral("Found note model item without notebook local uid: ") << *pNoteModelItem);
            continue;
        }

        QNTRACE(QStringLiteral("Notebook local uid for note local uid ") << pNoteModelItem->localUid()
                << QStringLiteral(": ") << notebookLocalUid);

        m_notebookLocalUidByNoteLocalUid[pNoteModelItem->localUid()] = notebookLocalUid;
    }

    m_receivedNotebookLocalUidsForAllNotes = true;
}

const NotebookModelItem * NotebookModel::itemForId(const IndexId id) const
{
    QNDEBUG(QStringLiteral("NotebookModelItem * NotebookModel::itemForId: ") << id);

    auto localUidIt = m_indexIdToLocalUidBimap.left.find(id);
    if (localUidIt != m_indexIdToLocalUidBimap.left.end())
    {
        const QString & localUid = localUidIt->second;
        QNTRACE(QStringLiteral("Found notebook local uid corresponding to model index internal id: ") << localUid);

        auto itemIt = m_modelItemsByLocalUid.find(localUid);
        if (itemIt != m_modelItemsByLocalUid.end()) {
            QNTRACE(QStringLiteral("Found notebook model item corresponding to local uid: ") << itemIt.value());
            return &(itemIt.value());
        }
        else {
            QNTRACE(QStringLiteral("Found no notebook model item corresponding to local uid"));
            return Q_NULLPTR;
        }
    }

    auto stackIt = m_indexIdToStackBimap.left.find(id);
    if (stackIt != m_indexIdToStackBimap.left.end())
    {
        const QString & stack = stackIt->second;
        QNTRACE(QStringLiteral("Found notebook stack corresponding to model index internal id: ") << stack);

        auto itemIt = m_modelItemsByStack.find(stack);
        if (itemIt != m_modelItemsByStack.end()) {
            QNTRACE(QStringLiteral("Found notebook model item corresponding to stack: ") << itemIt.value());
            return &(itemIt.value());
        }
        else {
            QNTRACE(QStringLiteral("Found no notebook model item corresponding to stack"));
            return Q_NULLPTR;
        }
    }

    QNDEBUG(QStringLiteral("Found no notebook model items corresponding to model index id"));
    return Q_NULLPTR;
}

NotebookModel::IndexId NotebookModel::idForItem(const NotebookModelItem & item) const
{
    if ((item.type() == NotebookModelItem::Type::Notebook) && item.notebookItem())
    {
        auto it = m_indexIdToLocalUidBimap.right.find(item.notebookItem()->localUid());
        if (it == m_indexIdToLocalUidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLocalUidBimap.insert(IndexIdToLocalUidBimap::value_type(id, item.notebookItem()->localUid())))
            return id;
        }

        return it->second;
    }
    else if ((item.type() == NotebookModelItem::Type::Stack) && item.notebookStackItem())
    {
        auto it = m_indexIdToStackBimap.right.find(item.notebookStackItem()->name());
        if (it == m_indexIdToStackBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToStackBimap.insert(IndexIdToStackBimap::value_type(id, item.notebookStackItem()->name())))
            return id;
        }

        return it->second;
    }

    QNWARNING(QStringLiteral("Detected attempt to assign id to unidentified notebook model item: ") << item);
    return 0;
}

bool NotebookModel::updateNoteCountPerNotebookIndex(const NotebookItem & item, const NotebookDataByLocalUid::iterator it)
{
    QNDEBUG(QStringLiteral("NotebookModel::updateNoteCountPerNotebookIndex: ") << item);

    const QString & notebookLocalUid = item.localUid();

    auto modelItemIt = m_modelItemsByLocalUid.find(notebookLocalUid);
    if (Q_UNLIKELY(modelItemIt == m_modelItemsByLocalUid.end())) {
        QNWARNING(QStringLiteral("Can't find the notebook model item corresponding to the notebook into which the note was inserted: ")
                  << item);
        return false;
    }

    const NotebookModelItem * pModelItem = &(modelItemIt.value());

    const NotebookModelItem * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(QStringLiteral("Can't find the parent notebook model item for the notebook item into which the note was inserted: ")
                  << *pModelItem);
        return false;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Can't find the row of the child notebook model item within its parent model item: parent item = ")
                  << *pParentItem << QStringLiteral("\nChild item = ") << *pModelItem);
        return false;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    localUidIndex.replace(it, item);

    IndexId modelItemId = idForItem(*pModelItem);

    QNTRACE(QStringLiteral("Emitting num notes per notebook update dataChanged signal: row = ") << row
            << QStringLiteral(", model item: ") << *pModelItem);
    QModelIndex modelIndexFrom = createIndex(row, Columns::NumNotesPerNotebook, modelItemId);
    QModelIndex modelIndexTo = createIndex(row, Columns::NumNotesPerNotebook, modelItemId);
    emit dataChanged(modelIndexFrom, modelIndexTo);
    return true;
}

bool NotebookModel::LessByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

#define ITEM_PTR_LESS(lhs, rhs) \
    if (!lhs) { \
        return true; \
    } \
    else if (!rhs) { \
        return false; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::LessByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

#define MODEL_ITEM_NAME(item, itemName) \
    if ((item.type() == NotebookModelItem::Type::Notebook) && item.notebookItem()) { \
        itemName = item.notebookItem()->nameUpper(); \
    } \
    else if ((item.type() == NotebookModelItem::Type::Stack) && item.notebookStackItem()) { \
        itemName = item.notebookStackItem()->name().toUpper(); \
    }

bool NotebookModel::LessByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

#define ITEM_PTR_GREATER(lhs, rhs) \
    if (!lhs) { \
        return false; \
    } \
    else if (!rhs) { \
        return true; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::GreaterByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

} // namespace quentier
