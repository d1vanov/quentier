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

#include "SavedSearchModel.h"

#include "../NewItemNameGenerator.hpp"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>

#include <algorithm>
#include <limits>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

#define NUM_SAVED_SEARCH_MODEL_COLUMNS (4)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    QNWARNING(                                                                 \
        "model:saved_search",                                                  \
        errorDescription << QLatin1String("" __VA_ARGS__ ""));                 \
    Q_EMIT notifyError(errorDescription) // REPORT_ERROR

namespace quentier {

SavedSearchModel::SavedSearchModel(
    const Account & account,
    LocalStorageManagerAsync & localStorageManagerAsync,
    SavedSearchCache & cache, QObject * parent) :
    ItemModel(account, parent),
    m_cache(cache)
{
    createConnections(localStorageManagerAsync);
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel() = default;

ISavedSearchModelItem * SavedSearchModel::itemForIndex(
    const QModelIndex & modelIndex) const
{
    QNTRACE(
        "model:saved_search",
        "SavedSearchModel::itemForIndex: row = " << modelIndex.row());

    if (!modelIndex.isValid()) {
        QNTRACE("model:saved_search", "Index is invalid");
        return m_pInvisibleRootItem;
    }

    if (!modelIndex.parent().isValid()) {
        return m_pAllSavedSearchesRootItem;
    }

    int row = modelIndex.row();

    const auto & index = m_data.get<ByIndex>();
    if (row >= static_cast<int>(index.size())) {
        QNTRACE(
            "model:saved_search",
            "Index's row is greater than the size "
                << "of the row index");
        return nullptr;
    }

    return const_cast<SavedSearchItem*>(&(index[static_cast<size_t>(row)]));
}

QModelIndex SavedSearchModel::indexForItem(
    const ISavedSearchModelItem * pItem) const
{
    if (!pItem) {
        return {};
    }

    if (pItem == m_pInvisibleRootItem) {
        return {};
    }

    if (pItem == m_pAllSavedSearchesRootItem) {
        return createIndex(0, 0);
    }

    auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        QNWARNING("model:saved_search", "Failed to cast item to saved search "
            << "one: " << *pItem);
        return {};
    }

    return indexForLocalUid(pSavedSearchItem->localUid());
}

QModelIndex SavedSearchModel::indexForSavedSearchName(
    const QString & savedSearchName) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();

    auto it = nameIndex.find(savedSearchName.toUpper());
    if (it == nameIndex.end()) {
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QStringList SavedSearchModel::savedSearchNames() const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();

    QStringList result;
    result.reserve(static_cast<int>(nameIndex.size()));

    for (const auto & search: nameIndex) {
        result << search.name();
    }

    return result;
}

QModelIndex SavedSearchModel::createSavedSearch(
    const QString & savedSearchName, const QString & searchQuery,
    ErrorString & errorDescription)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::createSavedSearch: saved "
            << "search name = " << savedSearchName
            << ", search query = " << searchQuery);

    if (savedSearchName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search name is empty"));
        return QModelIndex();
    }

    int savedSearchNameSize = savedSearchName.size();

    if (savedSearchNameSize < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search name size is below "
                       "the minimal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN);

        return {};
    }

    if (savedSearchNameSize > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search name size is above "
                       "the maximal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX);

        return {};
    }

    if (savedSearchName != savedSearchName.trimmed()) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search name should not start "
                       "or end with whitespace"));

        return {};
    }

    if (searchQuery.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search query is empty"));
        return {};
    }

    int searchQuerySize = searchQuery.size();

    if (searchQuerySize < qevercloud::EDAM_SEARCH_QUERY_LEN_MIN) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search query size is below "
                       "the minimal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_SEARCH_QUERY_LEN_MIN);

        return {};
    }

    if (searchQuerySize > qevercloud::EDAM_SEARCH_QUERY_LEN_MAX) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search query size is above "
                       "the maximal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_SEARCH_QUERY_LEN_MAX);

        return {};
    }

    auto existingItemIndex = indexForSavedSearchName(savedSearchName);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search with such name already exists"));
        return {};
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingSavedSearches = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(
            numExistingSavedSearches + 1 >= m_account.savedSearchCountMax())) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new saved search: the account can "
                       "contain a limited number of saved searches"));

        errorDescription.details() =
            QString::number(m_account.savedSearchCountMax());

        return {};
    }

    SavedSearchItem item;
    item.setLocalUid(UidGenerator::Generate());
    Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.insert(item.localUid()))

    item.setName(std::move(savedSearchName));
    item.setQuery(std::move(searchQuery));
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    Q_EMIT aboutToAddSavedSearch();

    int row = static_cast<int>(localUidIndex.size());

    beginInsertRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.insert(item))
    endInsertRows();

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QModelIndex addedSavedSearchIndex = indexForLocalUid(item.localUid());
    Q_EMIT addedSavedSearch(addedSavedSearchIndex);

    return addedSavedSearchIndex;
}

void SavedSearchModel::favoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::favoriteSavedSearch: "
            << "index: is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column());

    setSavedSearchFavorited(index, true);
}

void SavedSearchModel::unfavoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::unfavoriteSavedSearch: "
            << "index: is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column());

    setSavedSearchFavorited(index, false);
}

QString SavedSearchModel::localUidForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::localUidForItemName: "
            << "name = " << itemName);

    Q_UNUSED(linkedNotebookGuid)
    auto index = indexForSavedSearchName(itemName);
    const auto * pItem = itemForIndex(index);
    if (!pItem) {
        QNTRACE(
            "model:saved_search",
            "No saved search with such name was "
                << "found");
        return {};
    }

    const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        QNWARNING("model:saved_search", "Failed to case item found by name to "
            << "SavedSearchItem; name = " << itemName);
        return {};
    }

    return pSavedSearchItem->localUid();
}

QModelIndex SavedSearchModel::indexForLocalUid(const QString & localUid) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    return indexForLocalUidIndexIterator(it);
}

QString SavedSearchModel::itemNameForLocalUid(const QString & localUid) const
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::itemNameForLocalUid: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:saved_search", "No saved search with such local uid");
        return {};
    }

    return it->name();
}

ItemModel::ItemInfo SavedSearchModel::itemInfoForLocalUid(
    const QString & localUid) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:saved_search", "No saved search with such local uid");
        return {};
    }

    ItemModel::ItemInfo info;
    info.m_localUid = it->localUid();
    info.m_name = it->name();

    return info;
}

QStringList SavedSearchModel::itemNames(
    const QString & linkedNotebookGuid) const
{
    if (!linkedNotebookGuid.isEmpty()) {
        return {};
    }

    return savedSearchNames();
}

QVector<ItemModel::LinkedNotebookInfo> SavedSearchModel::linkedNotebooksInfo()
    const
{
    return {};
}

QString SavedSearchModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    Q_UNUSED(linkedNotebookGuid)
    return {};
}

QString SavedSearchModel::localUidForItemIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (!index.parent().isValid()) {
        // All saved searches root item
        return {};
    }

    int row = index.row();
    int column = index.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return {};
    }

    const auto & item = m_data.get<ByIndex>()[static_cast<size_t>(row)];
    return item.localUid();
}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (!index.parent().isValid()) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Dirty)) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Synchronizable)) {
        QVariant synchronizable =
            dataImpl(index.row(), Column::Synchronizable);

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
        return {};
    }

    if (!index.parent().isValid()) {
        if (index.column() == static_cast<int>(Column::Name)) {
            return tr("All saved searches");
        }

        return {};
    }

    int rowIndex = index.row();
    int columnIndex = index.column();

    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size())) ||
        (columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return {};
    }

    Column column;
    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
    case Column::Query:
    case Column::Synchronizable:
    case Column::Dirty:
        column = static_cast<Column>(columnIndex);
        break;
    default:
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(rowIndex, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(rowIndex, column);
    default:
        return {};
    }
}

QVariant SavedSearchModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch (static_cast<Column>(section)) {
    case Column::Name:
        return QVariant(tr("Name"));
    case Column::Query:
        return QVariant(tr("Query"));
    case Column::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Column::Dirty:
        return QVariant(tr("Changed"));
    default:
        return {};
    }
}

int SavedSearchModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        if (!parent.parent().isValid()) {
            return 1;
        }
        else {
            return 0;
        }
    }

    return static_cast<int>(m_data.size());
}

int SavedSearchModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        if (!parent.parent().isValid()) {
            return NUM_SAVED_SEARCH_MODEL_COLUMNS;
        }
        else {
            return 0;
        }

        return 0;
    }

    return NUM_SAVED_SEARCH_MODEL_COLUMNS;
}

QModelIndex SavedSearchModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return {};
    }

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return {};
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
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::setData: index: "
            << "is valid = " << (modelIndex.isValid() ? "true" : "false")
            << ", row = " << modelIndex.row()
            << ", column = " << modelIndex.column() << ", value = " << value
            << ", role = " << role);

    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        QNDEBUG("model:saved_search", "Bad row");
        return false;
    }

    int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
        QNDEBUG("model:saved_search", "Bad column");
        return false;
    }

    SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchModelItem item = index[static_cast<size_t>(rowIndex)];

    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
    {
        QString name = value.toString().trimmed();
        bool changed = (name != item.name());
        if (!changed) {
            QNDEBUG("model:saved_search", "The name has not changed");
            return true;
        }

        auto nameIt = nameIndex.find(name.toUpper());
        if (nameIt != nameIndex.end()) {
            ErrorString error(
                QT_TR_NOOP("Can't rename the saved search: no two saved "
                           "searches within the account are allowed to "
                           "have the same name in case-insensitive manner"));
            error.details() = name;
            QNINFO("model:saved_search", error);
            Q_EMIT notifyError(error);
            return false;
        }

        ErrorString errorDescription;
        if (!SavedSearch::validateName(name, &errorDescription)) {
            ErrorString error(QT_TR_NOOP("Can't rename the saved search"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();

            QNINFO(
                "model:saved_search",
                error << "; suggested new name = " << name);

            Q_EMIT notifyError(error);
            return false;
        }

        item.setDirty(true);
        item.setName(std::move(name));
        break;
    }
    case Column::Query:
    {
        QString query = value.toString();
        if (query.isEmpty()) {
            QNDEBUG("model:saved_search", "Query is empty");
            return false;
        }

        item.setDirty(item.isDirty() | (query != item.query()));
        item.setQuery(std::move(query));
        break;
    }
    case Column::Synchronizable:
    {
        if (m_account.type() == Account::Type::Local) {
            ErrorString error(
                QT_TR_NOOP("Can't make the saved search synchronizable "
                           "within the local account"));
            QNINFO("model:saved_search", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (item.isSynchronizable()) {
            ErrorString error(
                QT_TR_NOOP("Can't make already synchronizable "
                           "saved search not synchronizable"));

            QNINFO(
                "model:saved_search",
                error << ", already "
                      << "synchronizable saved search item: " << item);
            Q_EMIT notifyError(error);
            return false;
        }

        item.setDirty(item.isDirty() | (value.toBool() != item.isSynchronizable()));
        item.setSynchronizable(value.toBool());
        break;
    }
    default:
        QNWARNING(
            "model:saved_search",
            "Unidentified column: " << modelIndex.column());
        return false;
    }

    index.replace(index.begin() + rowIndex, item);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QNDEBUG("model:saved_search", "Successfully set the data");
    return true;
}

bool SavedSearchModel::insertRows(
    int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model:saved_search",
        "SavedSearchModel::insertRows: row = " << row << ", count = " << count);

    Q_UNUSED(parent)

    auto & index = m_data.get<ByIndex>();
    int numExistingSavedSearches = static_cast<int>(index.size());
    if (Q_UNLIKELY(
            numExistingSavedSearches + count >=
            m_account.savedSearchCountMax()))
    {
        ErrorString error(
            QT_TR_NOOP("Can't create a new saved search): the account can "
                       "contain a limited number of saved searches"));
        error.details() = QString::number(m_account.savedSearchCountMax());
        QNINFO("model:saved_search", error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<SavedSearchDataByIndex::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        SavedSearchModelItem item;
        item.setLocalUid(UidGenerator::Generate());

        Q_UNUSED(
            m_savedSearchItemsNotYetInLocalStorageUids.insert(item.localUid()));

        item.setName(nameForNewSavedSearch());
        item.setDirty(true);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        auto insertionResult = index.insert(index.begin() + row, item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    Q_EMIT layoutAboutToBeChanged();
    for (auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it)
    {
        const auto & item = *(*it);
        updateRandomAccessIndexWithRespectToSorting(item);
        updateSavedSearchInLocalStorage(item);
    }
    Q_EMIT layoutChanged();

    return true;
}

bool SavedSearchModel::removeRows(
    int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG(
            "model:saved_search",
            "Ignoring the attempt to remove rows "
                << "from saved search model for valid parent model index");
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size()))) {
        ErrorString error(
            QT_TR_NOOP("Detected attempt to remove more rows than "
                       "the saved search model contains"));

        QNINFO(
            "model:saved_search",
            error << ", row = " << row << ", count = " << count
                  << ", number of saved search model items = "
                  << m_data.size());

        Q_EMIT notifyError(error);
        return false;
    }

    auto & index = m_data.get<ByIndex>();

    for (int i = 0; i < count; ++i) {
        auto it = index.begin() + row + i;
        if (!it->guid().isEmpty()) {
            ErrorString error(
                QT_TR_NOOP("Can't delete saved search with non-empty guid"));

            QNINFO(
                "model:saved_search", error << ", saved search item: " << *it);

            Q_EMIT notifyError(error);
            return false;
        }
    }

    Q_EMIT aboutToRemoveSavedSearches();

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        auto it = index.begin() + row + i;

        SavedSearch savedSearch;
        savedSearch.setLocalUid(it->localUid());

        auto requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(requestId))

        QNTRACE(
            "model:saved_search",
            "Emitting the request to expunge "
                << "the saved search from the local storage: request id = "
                << requestId << ", saved search local uid: " << it->localUid());

        Q_EMIT expungeSavedSearch(savedSearch, requestId);
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    endRemoveRows();

    Q_EMIT removedSavedSearches();

    return true;
}

void SavedSearchModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (column != static_cast<int>(Column::Name)) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(
            "model:saved_search",
            "The sort order already established, "
                << "nothing to do");
        return;
    }

    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    auto persistentIndices = persistentIndexList();
    QStringList localUidsToUpdate;

    for (const auto & index: qAsConst(persistentIndices)) {
        if (!index.isValid() || (index.column() != static_cast<int>(Column::Name))) {
            localUidsToUpdate << QString();
            continue;
        }

        const auto * item = reinterpret_cast<const SavedSearchModelItem *>(
            index.internalPointer());

        if (!item) {
            localUidsToUpdate << QString();
            continue;
        }

        localUidsToUpdate << item->localUid();
    }

    auto & index = m_data.get<ByIndex>();

    std::vector<boost::reference_wrapper<const SavedSearchModelItem>> items(
        index.begin(), index.end());

    if (m_sortOrder == Qt::AscendingOrder) {
        std::sort(items.begin(), items.end(), LessByName());
    }
    else {
        std::sort(items.begin(), items.end(), GreaterByName());
    }

    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    for (const auto & localUid: qAsConst(localUidsToUpdate)) {
        if (localUid.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        auto newIndex = indexForLocalUid(localUid);
        replacementIndices << newIndex;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void SavedSearchModel::onAddSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onAddSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

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

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onAddSavedSearchFailed: "
            << "search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_addSavedSearchRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(
            "model:saved_search",
            "Can't find the saved search item failed "
                << "to be added to the local storage within the model's items");
        return;
    }

    auto & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find the indexed reference to "
                << "the saved search item failed to be added to the local "
                   "storage: "
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
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onUpdateSavedSearchComplete: "
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

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onUpdateSavedSearchFailed: search = "
            << search << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:saved_search",
        "Emitting the request to find the saved "
            << "search: local uid = " << search.localUid()
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

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onFindSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    if (restoreUpdateIt !=
        m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))

        onSavedSearchAddedOrUpdated(search);
    }
    else if (
        performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))

        m_cache.put(search.localUid(), search);
        auto & localUidIndex = m_data.get<ByLocalUid>();
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

    if ((restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNWARNING(
        "model:saved_search",
        "SavedSearchModel::onFindSavedSearchFailed: search = "
            << search << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt !=
        m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(
            restoreUpdateIt))
    }
    else if (
        performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    Q_EMIT notifyError(errorDescription);
}

void SavedSearchModel::onListSavedSearchesComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QList<SavedSearch> foundSearches, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onListSavedSearchesComplete: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", num found searches = " << foundSearches.size()
            << ", request id = " << requestId);

    for (const auto & foundSearch: qAsConst(foundSearches)) {
        onSavedSearchAddedOrUpdated(foundSearch);
    }

    m_listSavedSearchesRequestId = QUuid();
    if (!foundSearches.isEmpty()) {
        QNTRACE(
            "model:saved_search",
            "The number of found saved searches is "
                << "greater than zero, requesting more saved searches from "
                << "the local storage");

        m_listSavedSearchesOffset += static_cast<size_t>(foundSearches.size());
        requestSavedSearchesList();
        return;
    }

    m_allSavedSearchesListed = true;
    Q_EMIT notifyAllSavedSearchesListed();
    Q_EMIT notifyAllItemsListed();
}

void SavedSearchModel::onListSavedSearchesFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListSavedSearchesOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onListSavedSearchesFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", error: " << errorDescription
            << ", request id = " << requestId);

    m_listSavedSearchesRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void SavedSearchModel::onExpungeSavedSearchComplete(
    SavedSearch search, QUuid requestId)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it != m_expungeSavedSearchRequestIds.end()) {
        Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
        return;
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(
            "model:saved_search",
            "Expunged saved search was not found "
                << "within the saved search model items: " << search);
        return;
    }

    auto & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local uid index "
                       "iterator to the random access index iterator within "
                       "the saved searches model"));

        QNWARNING("model:saved_search", error);
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

    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onExpungeSavedSearchFailed: search = "
            << search << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG("model:saved_search", "SavedSearchModel::createConnections");

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(
        this, &SavedSearchModel::addSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddSavedSearchRequest);

    QObject::connect(
        this, &SavedSearchModel::updateSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateSavedSearchRequest);

    QObject::connect(
        this, &SavedSearchModel::findSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindSavedSearchRequest);

    QObject::connect(
        this, &SavedSearchModel::listSavedSearches, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListSavedSearchesRequest);

    QObject::connect(
        this, &SavedSearchModel::expungeSavedSearch, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onExpungeSavedSearchRequest);

    // localStorageManagerAsync's signals to local slots
    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addSavedSearchComplete, this,
        &SavedSearchModel::onAddSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addSavedSearchFailed, this,
        &SavedSearchModel::onAddSavedSearchFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchComplete, this,
        &SavedSearchModel::onUpdateSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateSavedSearchFailed, this,
        &SavedSearchModel::onUpdateSavedSearchFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchComplete, this,
        &SavedSearchModel::onFindSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findSavedSearchFailed, this,
        &SavedSearchModel::onFindSavedSearchFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesComplete, this,
        &SavedSearchModel::onListSavedSearchesComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listSavedSearchesFailed, this,
        &SavedSearchModel::onListSavedSearchesFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeSavedSearchComplete, this,
        &SavedSearchModel::onExpungeSavedSearchComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeSavedSearchFailed, this,
        &SavedSearchModel::onExpungeSavedSearchFailed);
}

void SavedSearchModel::requestSavedSearchesList()
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::requestSavedSearchesList: "
            << "offset = " << m_listSavedSearchesOffset);

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    LocalStorageManager::ListSavedSearchesOrder order =
        LocalStorageManager::ListSavedSearchesOrder::NoOrder;

    LocalStorageManager::OrderDirection direction =
        LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();

    QNTRACE(
        "model:saved_search",
        "Emitting the request to list saved "
            << "searches: offset = " << m_listSavedSearchesOffset
            << ", request id = " << m_listSavedSearchesRequestId);

    Q_EMIT listSavedSearches(
        flags, SAVED_SEARCH_LIST_LIMIT, m_listSavedSearchesOffset, order,
        direction, m_listSavedSearchesRequestId);
}

void SavedSearchModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    auto & rowIndex = m_data.get<ByIndex>();
    auto & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(search.localUid(), search);

    SavedSearchModelItem item(search.localUid());

    if (search.hasGuid()) {
        item.setGuid(search.guid());
    }

    if (search.hasName()) {
        item.setName(search.name());
    }

    if (search.hasQuery()) {
        item.setQuery(search.query());
    }

    item.setSynchronizable(!search.isLocal());
    item.setDirty(search.isDirty());
    item.setFavorited(search.isFavorited());

    auto itemIt = localUidIndex.find(search.localUid());
    bool newSavedSearch = (itemIt == localUidIndex.end());
    if (newSavedSearch) {
        Q_EMIT aboutToAddSavedSearch();

        int row = rowForNewItem(item);
        beginInsertRows(QModelIndex(), row, row);
        auto insertionResult = localUidIndex.insert(item);
        itemIt = insertionResult.first;
        endInsertRows();

        updateRandomAccessIndexWithRespectToSorting(*itemIt);

        auto addedSavedSearchIndex = indexForLocalUid(search.localUid());
        Q_EMIT addedSavedSearch(addedSavedSearchIndex);

        return;
    }

    auto savedSearchIndexBefore = indexForLocalUid(search.localUid());
    Q_EMIT aboutToUpdateSavedSearch(savedSearchIndexBefore);

    localUidIndex.replace(itemIt, item);

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local "
                       "uid index iterator to the random access "
                       "index iterator within the favorites model"));

        QNWARNING("model:saved_search", error);
        Q_EMIT notifyError(error);
        Q_EMIT updatedSavedSearch(QModelIndex());
        return;
    }

    qint64 position = std::distance(rowIndex.begin(), indexIt);
    if (Q_UNLIKELY(
            position > static_cast<qint64>(std::numeric_limits<int>::max()))) {
        ErrorString error(
            QT_TR_NOOP("Too many stored elements to handle for "
                       "saved searches model"));

        QNWARNING("model:saved_search", error);
        Q_EMIT notifyError(error);
        Q_EMIT updatedSavedSearch(QModelIndex());
        return;
    }

    auto modelIndexFrom = createIndex(static_cast<int>(position), 0);

    auto modelIndexTo = createIndex(
        static_cast<int>(position), NUM_SAVED_SEARCH_MODEL_COLUMNS - 1);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    updateRandomAccessIndexWithRespectToSorting(item);

    QModelIndex savedSearchIndexAfter = indexForLocalUid(search.localUid());
    Q_EMIT updatedSavedSearch(savedSearchIndexAfter);
}

QVariant SavedSearchModel::dataImpl(
    const int row, const Column column) const
{
    QNTRACE(
        "model:saved_search",
        "SavedSearchModel::dataImpl: row = " << row << ", column = " << column);

    if (Q_UNLIKELY((row < 0) || (row > static_cast<int>(m_data.size())))) {
        QNTRACE(
            "model:saved_search",
            "Invalid row " << row << ", data size is " << m_data.size());
        return {};
    }

    const auto & item = m_data.get<ByIndex>()[static_cast<size_t>(row)];
    switch (column) {
    case Column::Name:
        return QVariant(item.name());
    case Column::Query:
        return QVariant(item.query());
    case Column::Synchronizable:
        return QVariant(item.isSynchronizable());
    case Column::Dirty:
        return QVariant(item.isDirty());
    default:
        return {};
    }
}

QVariant SavedSearchModel::dataAccessibleText(
    const int row, const Column column) const
{
    QNTRACE(
        "model:saved_search",
        "SavedSearchModel::dataAccessibleText: row = " << row << ", column = "
                                                       << column);

    auto textData = dataImpl(row, column);
    if (textData.isNull()) {
        return {};
    }

    QString accessibleText = tr("Saved search: ");

    switch (column) {
    case Column::Name:
        accessibleText += tr("name is ") + textData.toString();
        break;
    case Column::Query:
        accessibleText += tr("query is ") + textData.toString();
        break;
    case Column::Synchronizable:
        accessibleText +=
            (textData.toBool() ? tr("synchronizable")
                               : tr("not synchronizable"));
        break;
    case Column::Dirty:
        accessibleText +=
            (textData.toBool() ? tr("changed") : tr("not changed"));
        break;
    default:
        return {};
    }

    return QVariant(accessibleText);
}

QString SavedSearchModel::nameForNewSavedSearch() const
{
    QString baseName = tr("New saved search");
    const auto & nameIndex = m_data.get<ByNameUpper>();

    return newItemName<SavedSearchDataByNameUpper>(
        nameIndex, m_lastNewSavedSearchNameCounter, baseName);
}

int SavedSearchModel::rowForNewItem(const SavedSearchModelItem & newItem) const
{
    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return static_cast<int>(m_data.size());
    }

    const auto & nameIndex = m_data.get<ByNameUpper>();

    auto it = nameIndex.end();
    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(
            nameIndex.begin(), nameIndex.end(), newItem, LessByName());
    }
    else {
        it = std::lower_bound(
            nameIndex.begin(), nameIndex.end(), newItem, GreaterByName());
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
    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(item.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find saved search item by local "
                << "uid: " << item);
        return;
    }

    const auto & nameIndex = m_data.get<ByNameUpper>();

    auto appropriateNameIt = nameIndex.end();
    if (m_sortOrder == Qt::AscendingOrder) {
        appropriateNameIt = std::lower_bound(
            nameIndex.begin(), nameIndex.end(), item, LessByName());
    }
    else {
        appropriateNameIt = std::lower_bound(
            nameIndex.begin(), nameIndex.end(), item, GreaterByName());
    }

    auto & index = m_data.get<ByIndex>();

    auto originalRandomAccessIt = m_data.project<ByIndex>(itemIt);
    auto newRandomAccessIt = m_data.project<ByIndex>(appropriateNameIt);

    int oldRow =
        static_cast<int>(std::distance(index.begin(), originalRandomAccessIt));

    int newRow =
        static_cast<int>(std::distance(index.begin(), newRandomAccessIt));

    if (oldRow == newRow) {
        QNDEBUG("model:saved_search", "Already at the appropriate row");
        return;
    }
    else if (oldRow + 1 == newRow) {
        QNDEBUG("model:saved_search", "Already before the appropriate row");
        return;
    }

    bool res =
        beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);

    if (!res) {
        QNWARNING(
            "model:saved_search",
            "Internal error, can't move row within "
                << "the saved search model for sorting purposes");
        return;
    }

    index.relocate(newRandomAccessIt, originalRandomAccessIt);
    endMoveRows();
}

void SavedSearchModel::updateSavedSearchInLocalStorage(
    const SavedSearchModelItem & item)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::updateSavedSearchInLocalStorage: local uid = "
            << item);

    SavedSearch savedSearch;

    auto notYetSavedItemIt =
        m_savedSearchItemsNotYetInLocalStorageUids.find(item.localUid());

    if (notYetSavedItemIt == m_savedSearchItemsNotYetInLocalStorageUids.end()) {
        QNDEBUG("model:saved_search", "Updating the saved search");

        const auto * pCachedSearch = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedSearch)) {
            auto requestId = QUuid::createUuid();
            Q_UNUSED(
                m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))

            SavedSearch dummy;
            dummy.setLocalUid(item.localUid());

            Q_EMIT findSavedSearch(dummy, requestId);

            QNDEBUG(
                "model:saved_search",
                "Emitted the request to find "
                    << "the saved search: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            return;
        }

        savedSearch = *pCachedSearch;
    }

    savedSearch.setLocalUid(item.localUid());
    savedSearch.setGuid(item.guid());
    savedSearch.setName(item.name());
    savedSearch.setQuery(item.query());
    savedSearch.setLocal(!item.isSynchronizable());
    savedSearch.setDirty(item.isDirty());
    savedSearch.setFavorited(item.isFavorited());

    auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_savedSearchItemsNotYetInLocalStorageUids.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.insert(requestId));
        Q_EMIT addSavedSearch(savedSearch, requestId);

        QNTRACE(
            "model:saved_search",
            "Emitted the request to add the saved "
                << "search to the local storage: id = " << requestId
                << ", saved search: " << savedSearch);

        Q_UNUSED(
            m_savedSearchItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else {
        Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId));

        // While the saved search is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(savedSearch.localUid()))

        Q_EMIT updateSavedSearch(savedSearch, requestId);

        QNTRACE(
            "model:saved_search",
            "Emitted the request to update the saved "
                << "search in the local storage: id = " << requestId
                << ", saved search: " << savedSearch);
    }
}

void SavedSearchModel::setSavedSearchFavorited(
    const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "model index is invalid"));
        return;
    }

    const auto * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "can't find the model item corresponding to index"));
        return;
    }

    if (favorited == pItem->isFavorited()) {
        QNDEBUG("model:saved_search", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(pItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "the modified saved search entry was not found "
                       "within the model"));
        return;
    }

    SavedSearchModelItem itemCopy(*pItem);
    itemCopy.setFavorited(favorited);

    localUidIndex.replace(it, itemCopy);

    updateSavedSearchInLocalStorage(itemCopy);
}

QModelIndex SavedSearchModel::indexForLocalUidIndexIterator(
    const SavedSearchDataByLocalUid::const_iterator it) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    if (it == localUidIndex.end()) {
        return QModelIndex();
    }

    const auto & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find the indexed reference to "
                << "the saved search item: " << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, static_cast<int>(Column::Name));
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
