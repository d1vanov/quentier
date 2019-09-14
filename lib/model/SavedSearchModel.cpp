/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "SavedSearchModel.h"
#include "NewItemNameGenerator.hpp"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>

#include <limits>
#include <algorithm>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

#define NUM_SAVED_SEARCH_MODEL_COLUMNS (4)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    QNWARNING(errorDescription << QLatin1String("" __VA_ARGS__ ""));           \
    Q_EMIT notifyError(errorDescription)                                       \
// REPORT_ERROR

namespace quentier {

SavedSearchModel::SavedSearchModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        SavedSearchCache & cache, QObject * parent) :
    ItemModel(parent),
    m_account(account),
    m_data(),
    m_listSavedSearchesOffset(0),
    m_listSavedSearchesRequestId(),
    m_savedSearchItemsNotYetInLocalStorageUids(),
    m_cache(cache),
    m_addSavedSearchRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_expungeSavedSearchRequestIds(),
    m_findSavedSearchToRestoreFailedUpdateRequestIds(),
    m_findSavedSearchToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_lastNewSavedSearchNameCounter(0),
    m_allSavedSearchesListed(false)
{
    createConnections(localStorageManagerAsync);
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel()
{}

void SavedSearchModel::updateAccount(const Account & account)
{
    QNDEBUG("SavedSearchModel::updateAccount: " << account);
    m_account = account;
}

const SavedSearchModelItem *
SavedSearchModel::itemForIndex(const QModelIndex & modelIndex) const
{
    QNTRACE("SavedSearchModel::itemForIndex: row = " << modelIndex.row());

    if (!modelIndex.isValid()) {
        QNTRACE("Index is invalid");
        return Q_NULLPTR;
    }

    int row = modelIndex.row();

    const SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    if (row >= static_cast<int>(index.size())) {
        QNTRACE("Index's row is greater than the size of the row index");
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

QModelIndex SavedSearchModel::indexForItem(
    const SavedSearchModelItem * pItem) const
{
    if (!pItem) {
        return QModelIndex();
    }

    return indexForLocalUid(pItem->m_localUid);

}

QModelIndex SavedSearchModel::indexForLocalUid(const QString & localUid) const
{
    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    return indexForLocalUidIndexIterator(it);
}

QModelIndex SavedSearchModel::indexForSavedSearchName(
    const QString & savedSearchName) const
{
    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto it = nameIndex.find(savedSearchName.toUpper());
    if (it == nameIndex.end()) {
        return QModelIndex();
    }

    const SavedSearchModelItem & item = *it;
    return indexForItem(&item);
}

QStringList SavedSearchModel::savedSearchNames() const
{
    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    QStringList result;
    result.reserve(static_cast<int>(nameIndex.size()));

    for(auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it) {
        result << it->m_name;
    }

    return result;
}

QModelIndex SavedSearchModel::createSavedSearch(
    const QString & savedSearchName, const QString & searchQuery,
    ErrorString & errorDescription)
{
    QNDEBUG("SavedSearchModel::createSavedSearch: saved search name = "
            << savedSearchName << ", search query = " << searchQuery);

    if (savedSearchName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search name is empty"));
        return QModelIndex();
    }

    int savedSearchNameSize = savedSearchName.size();

    if (savedSearchNameSize < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN)
    {
        errorDescription.setBase(QT_TR_NOOP("Saved search name size is below "
                                            "the minimal acceptable length"));
        errorDescription.details() =
            QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN);
        return QModelIndex();
    }

    if (savedSearchNameSize > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX)
    {
        errorDescription.setBase(QT_TR_NOOP("Saved search name size is above "
                                            "the maximal acceptable length"));
        errorDescription.details() =
            QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX);
        return QModelIndex();
    }

    if (savedSearchName != savedSearchName.trimmed()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search name should not start "
                                            "or end with whitespace"));
        return QModelIndex();
    }

    if (searchQuery.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search query is empty"));
        return QModelIndex();
    }

    int searchQuerySize = searchQuery.size();

    if (searchQuerySize < qevercloud::EDAM_SEARCH_QUERY_LEN_MIN)
    {
        errorDescription.setBase(QT_TR_NOOP("Saved search query size is below "
                                            "the minimal acceptable length"));
        errorDescription.details() =
            QString::number(qevercloud::EDAM_SEARCH_QUERY_LEN_MIN);
        return QModelIndex();
    }

    if (searchQuerySize > qevercloud::EDAM_SEARCH_QUERY_LEN_MAX)
    {
        errorDescription.setBase(QT_TR_NOOP("Saved search query size is above "
                                            "the maximal acceptable length"));
        errorDescription.details() =
            QString::number(qevercloud::EDAM_SEARCH_QUERY_LEN_MAX);
        return QModelIndex();
    }

    QModelIndex existingItemIndex = indexForSavedSearchName(savedSearchName);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search with such name already "
                                            "exists"));
        return QModelIndex();
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingSavedSearches = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingSavedSearches + 1 >= m_account.savedSearchCountMax()))
    {
        errorDescription.setBase(QT_TR_NOOP("Can't create a new saved search: "
                                            "the account can contain a limited "
                                            "number of saved searches"));
        errorDescription.details() = QString::number(m_account.savedSearchCountMax());
        return QModelIndex();
    }

    SavedSearchModelItem item;
    item.m_localUid = UidGenerator::Generate();
    Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.insert(item.m_localUid))

    item.m_name = savedSearchName;
    item.m_query = searchQuery;
    item.m_isDirty = true;
    item.m_isSynchronizable = (m_account.type() != Account::Type::Local);

    Q_EMIT aboutToAddSavedSearch();

    int row = static_cast<int>(localUidIndex.size());

    beginInsertRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.insert(item))
    endInsertRows();

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QModelIndex addedSavedSearchIndex = indexForLocalUid(item.m_localUid);
    Q_EMIT addedSavedSearch(addedSavedSearchIndex);

    return addedSavedSearchIndex;
}

void SavedSearchModel::favoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG("SavedSearchModel::favoriteSavedSearch: index: is valid = "
            << (index.isValid() ? "true" : "false")
            << ", row = " << index.row()
            << ", column = " << index.column());

    setSavedSearchFavorited(index, true);
}

void SavedSearchModel::unfavoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG("SavedSearchModel::unfavoriteSavedSearch: index: is valid = "
            << (index.isValid() ? "true" : "false")
            << ", row = " << index.row()
            << ", column = " << index.column());

    setSavedSearchFavorited(index, false);
}

QString SavedSearchModel::localUidForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNDEBUG("SavedSearchModel::localUidForItemName: name = " << itemName);

    Q_UNUSED(linkedNotebookGuid)
    QModelIndex index = indexForSavedSearchName(itemName);
    const SavedSearchModelItem * pItem = itemForIndex(index);
    if (!pItem) {
        QNTRACE("No saved search with such name was found");
        return QString();
    }

    return pItem->m_localUid;
}

QString SavedSearchModel::itemNameForLocalUid(const QString & localUid) const
{
    QNDEBUG("SavedSearchModel::itemNameForLocalUid: " << localUid);

    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("No saved search with such local uid");
        return QString();
    }

    const SavedSearchModelItem & item = *it;
    return item.m_name;
}

QStringList SavedSearchModel::itemNames(const QString & linkedNotebookGuid) const
{
    if (!linkedNotebookGuid.isEmpty()) {
        return QStringList();
    }

    return savedSearchNames();
}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (index.column() == Columns::Dirty) {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        QVariant synchronizable = dataImpl(index.row(), Columns::Synchronizable);
        if (!synchronizable.isNull() && synchronizable.toBool()) {
            return indexFlags;
        }
    }

    indexFlags |= Qt::ItemIsEditable;
    return indexFlags;
}

QVariant SavedSearchModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int rowIndex = index.row();
    int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::Name:
    case Columns::Query:
    case Columns::Synchronizable:
    case Columns::Dirty:
        column = static_cast<Columns::type>(columnIndex);
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(rowIndex, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(rowIndex, column);
    default:
        return QVariant();
    }
}

QVariant SavedSearchModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::Name:
        return QVariant(tr("Name"));
    case Columns::Query:
        return QVariant(tr("Query"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Changed"));
    default:
        return QVariant();
    }
}

int SavedSearchModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_data.size());
}

int SavedSearchModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_SAVED_SEARCH_MODEL_COLUMNS;
}

QModelIndex SavedSearchModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
        (column < 0) || (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex SavedSearchModel::parent(const QModelIndex & index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool SavedSearchModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool SavedSearchModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNDEBUG("SavedSearchModel::setData: index: is valid = "
            << (modelIndex.isValid() ? "true" : "false")
            << ", row = " << modelIndex.row()
            << ", column = " << modelIndex.column()
            << ", value = " << value
            << ", role = " << role);

    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        QNDEBUG("Bad row");
        return false;
    }

    int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
        QNDEBUG("Bad column");
        return false;
    }

    SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchModelItem item = index[static_cast<size_t>(rowIndex)];

    switch(columnIndex)
    {
    case Columns::Name:
        {
            QString name = value.toString().trimmed();
            bool changed = (name != item.m_name);
            if (!changed) {
                QNDEBUG("The name has not changed");
                return true;
            }

            auto nameIt = nameIndex.find(name.toUpper());
            if (nameIt != nameIndex.end())
            {
                ErrorString error(QT_TR_NOOP("Can't rename the saved search: "
                                             "no two saved searches within "
                                             "the account are allowed to have "
                                             "the same name in case-insensitive "
                                             "manner"));
                error.details() = name;
                QNINFO(error);
                Q_EMIT notifyError(error);
                return false;
            }

            ErrorString errorDescription;
            if (!SavedSearch::validateName(name, &errorDescription)) {
                ErrorString error(QT_TR_NOOP("Can't rename the saved search"));
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();
                QNINFO(error << "; suggested new name = " << name);
                Q_EMIT notifyError(error);
                return false;
            }

            item.m_isDirty = true;
            item.m_name = name;
            break;
        }
    case Columns::Query:
        {
            QString query = value.toString();
            if (query.isEmpty()) {
                QNDEBUG("Query is empty");
                return false;
            }

            item.m_isDirty |= (query != item.m_query);
            item.m_query = query;
            break;
        }
    case Columns::Synchronizable:
        {
            if (m_account.type() == Account::Type::Local)
            {
                ErrorString error(QT_TR_NOOP("Can't make the saved search "
                                             "synchronizable within "
                                             "the local account"));
                QNINFO(error);
                Q_EMIT notifyError(error);
                return false;
            }

            if (item.m_isSynchronizable)
            {
                ErrorString error(QT_TR_NOOP("Can't make already synchronizable "
                                             "saved search not synchronizable"));
                QNINFO(error << ", already synchronizable saved search item: "
                       << item);
                Q_EMIT notifyError(error);
                return false;
            }

            item.m_isDirty |= (value.toBool() != item.m_isSynchronizable);
            item.m_isSynchronizable = value.toBool();
            break;
        }
    default:
        QNWARNING("Unidentified column: " << modelIndex.column());
        return false;
    }

    index.replace(index.begin() + rowIndex, item);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QNDEBUG("Successfully set the data");
    return true;
}

bool SavedSearchModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE("SavedSearchModel::insertRows: row = " << row
            << ", count = " << count);

    Q_UNUSED(parent)

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    int numExistingSavedSearches = static_cast<int>(index.size());
    if (Q_UNLIKELY(numExistingSavedSearches + count >=
                   m_account.savedSearchCountMax()))
    {
        ErrorString error(QT_TR_NOOP("Can't create a new saved search): "
                                     "the account can contain "
                                     "a limited number of saved searches"));
        error.details() = QString::number(m_account.savedSearchCountMax());
        QNINFO(error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<SavedSearchDataByIndex::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        SavedSearchModelItem item;
        item.m_localUid = UidGenerator::Generate();
        Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.insert(item.m_localUid));
        item.m_name = nameForNewSavedSearch();
        item.m_isDirty = true;
        item.m_isSynchronizable = (m_account.type() != Account::Type::Local);

        auto insertionResult = index.insert(index.begin() + row, item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    Q_EMIT layoutAboutToBeChanged();
    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const SavedSearchModelItem & item = *(*it);
        updateRandomAccessIndexWithRespectToSorting(item);
        updateSavedSearchInLocalStorage(item);
    }
    Q_EMIT layoutChanged();

    return true;
}

bool SavedSearchModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG("Ignoring the attempt to remove rows from saved "
                "search model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size())))
    {
        ErrorString error(QT_TR_NOOP("Detected attempt to remove more rows than "
                                     "the saved search model contains"));
        QNINFO(error << ", row = " << row << ", count = " << count
               << ", number of saved search model items = " << m_data.size());
        Q_EMIT notifyError(error);
        return false;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row + i;
        if (!it->m_guid.isEmpty())
        {
            ErrorString error(QT_TR_NOOP("Can't delete saved search with "
                                         "non-empty guid"));
            QNINFO(error << ", saved search item: " << *it);
            Q_EMIT notifyError(error);
            return false;
        }
    }

    Q_EMIT aboutToRemoveSavedSearches();

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row + i;

        SavedSearch savedSearch;
        savedSearch.setLocalUid(it->m_localUid);

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(requestId))
        QNTRACE("Emitting the request to expunge the saved search "
                << "from the local storage: request id = "
                << requestId << ", saved search local uid: "
                << it->m_localUid);
        Q_EMIT expungeSavedSearch(savedSearch, requestId);
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    endRemoveRows();

    Q_EMIT removedSavedSearches();

    return true;
}

void SavedSearchModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("SavedSearchModel::sort: column = " << column
            << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (column != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG("The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    QModelIndexList persistentIndices = persistentIndexList();
    QStringList localUidsToUpdate;
    for(auto it = persistentIndices.begin(),
        end = persistentIndices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        if (!index.isValid() || (index.column() != Columns::Name)) {
            localUidsToUpdate << QString();
            continue;
        }

        const SavedSearchModelItem * item =
            reinterpret_cast<const SavedSearchModelItem*>(index.internalPointer());
        if (!item) {
            localUidsToUpdate << QString();
            continue;
        }

        localUidsToUpdate << item->m_localUid;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    std::vector<boost::reference_wrapper<const SavedSearchModelItem> > items(
        index.begin(), index.end());

    if (m_sortOrder == Qt::AscendingOrder) {
        std::sort(items.begin(), items.end(), LessByName());
    }
    else {
        std::sort(items.begin(), items.end(), GreaterByName());
    }

    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    for(auto it = localUidsToUpdate.begin(),
        end = localUidsToUpdate.end(); it != end; ++it)
    {
        const QString & localUid = *it;
        if (localUid.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }
        QModelIndex newIndex = indexForLocalUid(localUid);
        replacementIndices << newIndex;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void SavedSearchModel::onAddSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onAddSavedSearchComplete: " << search
            << "\nRequest id = " << requestId);

    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it != m_addSavedSearchRequestIds.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.erase(it));
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onAddSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it == m_addSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onAddSavedSearchFailed: search = "
            << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_addSavedSearchRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Can't find the saved search item failed to be "
                "added to the local storage within the model's items");
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the saved "
                  << "search item failed to be added to the local storage: "
                  << search);
        return;
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();
}

void SavedSearchModel::onUpdateSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onUpdateSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onUpdateSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onUpdateSavedSearchFailed: search = "
            << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find the saved search: "
            << "local uid = " << search.localUid()
            << ", request id = " << requestId);
    Q_EMIT findSavedSearch(search, requestId);
}

void SavedSearchModel::onFindSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt =
        m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) )
    {
        return;
    }

    QNDEBUG("SavedSearchModel::onFindSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onSavedSearchAddedOrUpdated(search);
    }
    else if (performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(search.localUid(), search);
        SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(search.localUid());
        if (it != localUidIndex.end()) {
            updateSavedSearchInLocalStorage(*it);
        }
    }
}

void SavedSearchModel::onFindSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt =
        m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) )
    {
        return;
    }

    QNWARNING("SavedSearchModel::onFindSavedSearchFailed: search = " << search
              << "\nError description = " << errorDescription
              << ", request id = " << requestId);

    if (restoreUpdateIt != m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void SavedSearchModel::onListSavedSearchesComplete(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    QList<SavedSearch> foundSearches, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG("SavedSearchModel::onListSavedSearchesComplete: flag = "
            << flag << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", num found searches = " << foundSearches.size()
            << ", request id = " << requestId);

    for(auto it = foundSearches.constBegin(),
        end = foundSearches.constEnd(); it != end; ++it)
    {
        onSavedSearchAddedOrUpdated(*it);
    }

    m_listSavedSearchesRequestId = QUuid();
    if (!foundSearches.isEmpty()) {
        QNTRACE("The number of found saved searches is greater "
                "than zero, requesting more saved searches from "
                "the local storage");
        m_listSavedSearchesOffset += static_cast<size_t>(foundSearches.size());
        requestSavedSearchesList();
        return;
    }

    m_allSavedSearchesListed = true;
    Q_EMIT notifyAllSavedSearchesListed();
    Q_EMIT notifyAllItemsListed();
}

void SavedSearchModel::onListSavedSearchesFailed(
    LocalStorageManager::ListObjectsOptions flag,
    size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder::type order,
    LocalStorageManager::OrderDirection::type orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG("SavedSearchModel::onListSavedSearchesFailed: flag = "
            << flag << ", limit = " << limit
            << ", offset = " << offset
            << ", order = " << order
            << ", direction = " << orderDirection
            << ", error: " << errorDescription
            << ", request id = " << requestId);

    m_listSavedSearchesRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void SavedSearchModel::onExpungeSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it != m_expungeSavedSearchRequestIds.end()) {
        Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
        return;
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Expunged saved search was not found within "
                << "the saved search model items: " << search);
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't project the local "
                                     "uid index iterator to the random access "
                                     "index iterator within the saved searches "
                                     "model"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    Q_EMIT aboutToRemoveSavedSearches();

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();

    Q_EMIT removedSavedSearches();
}

void SavedSearchModel::onExpungeSavedSearchFailed(
    SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it == m_expungeSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onExpungeSavedSearchFailed: search = "
            << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG("SavedSearchModel::createConnections");

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this,
                     QNSIGNAL(SavedSearchModel,addSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onAddSavedSearchRequest,
                            SavedSearch,QUuid));
    QObject::connect(this,
                     QNSIGNAL(SavedSearchModel,updateSavedSearch,
                              SavedSearch,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onUpdateSavedSearchRequest,
                            SavedSearch,QUuid));
    QObject::connect(this,
                     QNSIGNAL(SavedSearchModel,findSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onFindSavedSearchRequest,
                            SavedSearch,QUuid));
    QObject::connect(this,
                     QNSIGNAL(SavedSearchModel,listSavedSearches,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,
                              LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onListSavedSearchesRequest,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,
                            LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this,
                     QNSIGNAL(SavedSearchModel,expungeSavedSearch,
                              SavedSearch,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onExpungeSavedSearchRequest,
                            SavedSearch,QUuid));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onAddSavedSearchComplete,
                            SavedSearch,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,addSavedSearchFailed,
                              SavedSearch,ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onAddSavedSearchFailed,
                            SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onUpdateSavedSearchComplete,
                            SavedSearch,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,updateSavedSearchFailed,
                              SavedSearch,ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onUpdateSavedSearchFailed,
                            SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onFindSavedSearchComplete,
                            SavedSearch,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,findSavedSearchFailed,
                              SavedSearch,ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onFindSavedSearchFailed,
                            SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listSavedSearchesComplete,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              QList<SavedSearch>,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onListSavedSearchesComplete,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            QList<SavedSearch>,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listSavedSearchesFailed,
                              LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,
                              LocalStorageManager::OrderDirection::type,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onListSavedSearchesFailed,
                            LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,
                            LocalStorageManager::OrderDirection::type,
                            ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeSavedSearchComplete,
                              SavedSearch,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onExpungeSavedSearchComplete,
                            SavedSearch,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,expungeSavedSearchFailed,
                              SavedSearch,ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchModel,onExpungeSavedSearchFailed,
                            SavedSearch,ErrorString,QUuid));
}

void SavedSearchModel::requestSavedSearchesList()
{
    QNDEBUG("SavedSearchModel::requestSavedSearchesList: offset = "
            << m_listSavedSearchesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListSavedSearchesOrder::type order =
        LocalStorageManager::ListSavedSearchesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list saved searches: offset = "
            << m_listSavedSearchesOffset << ", request id = "
            << m_listSavedSearchesRequestId);
    Q_EMIT listSavedSearches(flags, SAVED_SEARCH_LIST_LIMIT, m_listSavedSearchesOffset,
                             order, direction, m_listSavedSearchesRequestId);
}

void SavedSearchModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    SavedSearchDataByIndex & rowIndex = m_data.get<ByIndex>();
    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(search.localUid(), search);

    SavedSearchModelItem item(search.localUid());

    if (search.hasGuid()) {
        item.m_guid = search.guid();
    }

    if (search.hasName()) {
        item.m_name = search.name();
    }

    if (search.hasQuery()) {
        item.m_query = search.query();
    }

    item.m_isSynchronizable = !search.isLocal();
    item.m_isDirty = search.isDirty();
    item.m_isFavorited = search.isFavorited();

    SavedSearchDataByLocalUid::iterator itemIt = localUidIndex.find(search.localUid());
    bool newSavedSearch = (itemIt == localUidIndex.end());
    if (newSavedSearch)
    {
        Q_EMIT aboutToAddSavedSearch();

        int row = rowForNewItem(item);
        beginInsertRows(QModelIndex(), row, row);
        auto insertionResult = localUidIndex.insert(item);
        itemIt = insertionResult.first;
        endInsertRows();

        updateRandomAccessIndexWithRespectToSorting(*itemIt);

        QModelIndex addedSavedSearchIndex = indexForLocalUid(search.localUid());
        Q_EMIT addedSavedSearch(addedSavedSearchIndex);

        return;
    }

    QModelIndex savedSearchIndexBefore = indexForLocalUid(search.localUid());
    Q_EMIT aboutToUpdateSavedSearch(savedSearchIndexBefore);

    localUidIndex.replace(itemIt, item);

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(QT_TR_NOOP("Internal error: can't project the local "
                                     "uid index iterator to the random access "
                                     "index iterator within the favorites model"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        Q_EMIT updatedSavedSearch(QModelIndex());
        return;
    }

    qint64 position = std::distance(rowIndex.begin(), indexIt);
    if (Q_UNLIKELY(position > static_cast<qint64>(std::numeric_limits<int>::max()))) {
        ErrorString error(QT_TR_NOOP("Too many stored elements to handle for "
                                     "saved searches model"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        Q_EMIT updatedSavedSearch(QModelIndex());
        return;
    }

    QModelIndex modelIndexFrom = createIndex(static_cast<int>(position), 0);
    QModelIndex modelIndexTo = createIndex(static_cast<int>(position),
                                           NUM_SAVED_SEARCH_MODEL_COLUMNS - 1);
    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    updateRandomAccessIndexWithRespectToSorting(item);

    QModelIndex savedSearchIndexAfter = indexForLocalUid(search.localUid());
    Q_EMIT updatedSavedSearch(savedSearchIndexAfter);
}

QVariant SavedSearchModel::dataImpl(
    const int row, const Columns::type column) const
{
    QNTRACE("SavedSearchModel::dataImpl: row = " << row
            << ", column = " << column);

    if (Q_UNLIKELY((row < 0) || (row > static_cast<int>(m_data.size())))) {
        QNTRACE("Invalid row " << row << ", data size is " << m_data.size());
        return QVariant();
    }

    const SavedSearchModelItem & item =
        m_data.get<ByIndex>()[static_cast<size_t>(row)];
    switch(column)
    {
    case Columns::Name:
        return QVariant(item.m_name);
    case Columns::Query:
        return QVariant(item.m_query);
    case Columns::Synchronizable:
        return QVariant(item.m_isSynchronizable);
    case Columns::Dirty:
        return QVariant(item.m_isDirty);
    default:
        return QVariant();
    }
}

QVariant SavedSearchModel::dataAccessibleText(
    const int row, const Columns::type column) const
{
    QNTRACE("SavedSearchModel::dataAccessibleText: row = " << row
            << ", column = " << column);

    QVariant textData = dataImpl(row, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = tr("Saved search: ");

    switch(column)
    {
    case Columns::Name:
        accessibleText += tr("name is ") + textData.toString();
        break;
    case Columns::Query:
        accessibleText += tr("query is ") + textData.toString();
        break;
    case Columns::Synchronizable:
        accessibleText += (textData.toBool()
                           ? tr("synchronizable")
                           : tr("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (textData.toBool()
                           ? tr("changed")
                           : tr("not changed"));
        break;
    default:
        return QVariant();
    }

    return QVariant(accessibleText);
}

QString SavedSearchModel::nameForNewSavedSearch() const
{
    QString baseName = tr("New saved search");
    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    return newItemName<SavedSearchDataByNameUpper>(
        nameIndex, m_lastNewSavedSearchNameCounter, baseName);
}

int SavedSearchModel::rowForNewItem(const SavedSearchModelItem & newItem) const
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return static_cast<int>(m_data.size());
    }

    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto it = nameIndex.end();
    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(nameIndex.begin(), nameIndex.end(),
                              newItem, LessByName());
    }
    else {
        it = std::lower_bound(nameIndex.begin(), nameIndex.end(),
                              newItem, GreaterByName());
    }

    if (it == nameIndex.end()) {
        return static_cast<int>(m_data.size());
    }

    int row = static_cast<int>(std::distance(nameIndex.begin(), it));
    return row;
}

void SavedSearchModel::updateRandomAccessIndexWithRespectToSorting(
    const SavedSearchModelItem & item)
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(item.m_localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNWARNING("Can't find saved search item by local uid: " << item);
        return;
    }

    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto appropriateNameIt = nameIndex.end();
    if (m_sortOrder == Qt::AscendingOrder) {
        appropriateNameIt = std::lower_bound(nameIndex.begin(), nameIndex.end(),
                                             item, LessByName());
    }
    else {
        appropriateNameIt = std::lower_bound(nameIndex.begin(), nameIndex.end(),
                                             item, GreaterByName());
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();

    SavedSearchDataByIndex::iterator originalRandomAccessIt =
        m_data.project<ByIndex>(itemIt);
    SavedSearchDataByIndex::iterator newRandomAccessIt =
        m_data.project<ByIndex>(appropriateNameIt);
    int oldRow = static_cast<int>(std::distance(index.begin(), originalRandomAccessIt));
    int newRow = static_cast<int>(std::distance(index.begin(), newRandomAccessIt));

    if (oldRow == newRow) {
        QNDEBUG("Already at the appropriate row");
        return;
    }
    else if (oldRow + 1 == newRow) {
        QNDEBUG("Already before the appropriate row");
        return;
    }

    bool res = beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
    if (!res) {
        QNWARNING("Internal error, can't move row within the saved "
                  "search model for sorting purposes");
        return;
    }

    index.relocate(newRandomAccessIt, originalRandomAccessIt);
    endMoveRows();
}

void SavedSearchModel::updateSavedSearchInLocalStorage(
    const SavedSearchModelItem & item)
{
    QNDEBUG("SavedSearchModel::updateSavedSearchInLocalStorage: local uid = "
            << item);

    SavedSearch savedSearch;

    auto notYetSavedItemIt =
        m_savedSearchItemsNotYetInLocalStorageUids.find(item.m_localUid);
    if (notYetSavedItemIt == m_savedSearchItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG("Updating the saved search");

        const SavedSearch * pCachedSearch = m_cache.get(item.m_localUid);
        if (Q_UNLIKELY(!pCachedSearch))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))
                SavedSearch dummy;
            dummy.setLocalUid(item.m_localUid);
            Q_EMIT findSavedSearch(dummy, requestId);
            QNDEBUG("Emitted the request to find the saved search: "
                    << "local uid = " << item.m_localUid
                    << ", request id = " << requestId);
            return;
        }

        savedSearch = *pCachedSearch;
    }

    savedSearch.setLocalUid(item.m_localUid);
    savedSearch.setGuid(item.m_guid);
    savedSearch.setName(item.m_name);
    savedSearch.setQuery(item.m_query);
    savedSearch.setLocal(!item.m_isSynchronizable);
    savedSearch.setDirty(item.m_isDirty);
    savedSearch.setFavorited(item.m_isFavorited);

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_savedSearchItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addSavedSearchRequestIds.insert(requestId));
        Q_EMIT addSavedSearch(savedSearch, requestId);

        QNTRACE("Emitted the request to add the saved search to "
                << "the local storage: id = " << requestId
                << ", saved search: " << savedSearch);

        Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId));

        // While the saved search is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(savedSearch.localUid()))

        Q_EMIT updateSavedSearch(savedSearch, requestId);

        QNTRACE("Emitted the request to update the saved search "
                << "in the local storage: id = " << requestId
                << ", saved search: " << savedSearch);
    }
}

void SavedSearchModel::setSavedSearchFavorited(
    const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the saved search: "
                                "model index is invalid"));
        return;
    }

    const SavedSearchModelItem * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the saved search: "
                                "can't find the model item corresponding to index"));
        return;
    }

    if (favorited == pItem->m_isFavorited) {
        QNDEBUG("Favorited flag's value hasn't changed");
        return;
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(pItem->m_localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the saved search: "
                                "the modified saved search entry was not found "
                                "within the model"));
        return;
    }

    SavedSearchModelItem itemCopy(*pItem);
    itemCopy.m_isFavorited = favorited;

    localUidIndex.replace(it, itemCopy);

    updateSavedSearchInLocalStorage(itemCopy);
}

QModelIndex SavedSearchModel::indexForLocalUidIndexIterator(
    const SavedSearchDataByLocalUid::const_iterator it) const
{
    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    if (it == localUidIndex.end()) {
        return QModelIndex();
    }

    const SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the saved search item: "
                  << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Name);
}

bool SavedSearchModel::LessByName::operator()(
    const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

bool SavedSearchModel::GreaterByName::operator()(
    const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

} // namespace quentier
