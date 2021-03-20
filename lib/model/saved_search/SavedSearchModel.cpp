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

#include "SavedSearchModel.h"

#include "AllSavedSearchesRootItem.h"
#include "InvisibleRootItem.h"

#include <lib/model/common/NewItemNameGenerator.hpp>
#include <lib/utility/ToOptional.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/UidGenerator.h>

#include <algorithm>
#include <cstddef>
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
    AbstractItemModel(account, parent),
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

    const int row = modelIndex.row();
    const auto & index = m_data.get<ByIndex>();
    if (row >= static_cast<int>(index.size())) {
        QNTRACE(
            "model:saved_search",
            "Index's row is greater than the size "
                << "of the row index");
        return nullptr;
    }

    return const_cast<SavedSearchItem *>(
        &(index[static_cast<std::size_t>(row)]));
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
        return createIndex(
            0, static_cast<int>(Column::Name),
            m_allSavedSearchesRootItemIndexId);
    }

    const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        QNWARNING(
            "model:saved_search",
            "Failed to cast item to saved search one: " << *pItem);
        return {};
    }

    return indexForLocalId(pSavedSearchItem->localId());
}

QModelIndex SavedSearchModel::indexForSavedSearchName(
    const QString & savedSearchName) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();

    const auto it = nameIndex.find(savedSearchName.toUpper());
    if (it == nameIndex.end()) {
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString SavedSearchModel::queryForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (it == localIdIndex.end()) {
        return {};
    }

    return it->query();
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
    QString savedSearchName, QString searchQuery,
    ErrorString & errorDescription)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::createSavedSearch: saved search name = "
            << savedSearchName << ", search query = " << searchQuery);

    if (savedSearchName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Saved search name is empty"));
        return {};
    }

    const int savedSearchNameSize = savedSearchName.size();

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

    const int searchQuerySize = searchQuery.size();

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

    const auto existingItemIndex = indexForSavedSearchName(savedSearchName);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Saved search with such name already exists"));
        return {};
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const int numExistingSavedSearches = static_cast<int>(localIdIndex.size());
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
    item.setLocalId(UidGenerator::Generate());
    QNWARNING("model:saved_search", "New saved search's local id: "
        << item.localId());
    Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageIds.insert(item.localId()))

    item.setName(std::move(savedSearchName));
    item.setQuery(std::move(searchQuery));
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    checkAndCreateModelRootItems();
    Q_EMIT aboutToAddSavedSearch();

    const int row = static_cast<int>(localIdIndex.size());

    beginInsertRows(indexForItem(m_pAllSavedSearchesRootItem), row, row);
    Q_UNUSED(localIdIndex.insert(item))
    Q_ASSERT(localIdIndex.find(item.localId()) != localIdIndex.end());
    endInsertRows();

    updateRandomAccessIndexWithRespectToSorting(item);
    Q_ASSERT(localIdIndex.find(item.localId()) != localIdIndex.end());

    updateSavedSearchInLocalStorage(item);
    if (localIdIndex.find(item.localId()) == localIdIndex.end()) {
        QNWARNING("model:saved_search", "Error detected");
    }

    const auto addedSavedSearchIndex = indexForLocalId(item.localId());
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

QString SavedSearchModel::localIdForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::localIdForItemName: name = " << itemName);

    Q_UNUSED(linkedNotebookGuid)
    const auto index = indexForSavedSearchName(itemName);
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
        QNWARNING(
            "model:saved_search",
            "Failed to case item found by name to "
                << "SavedSearchItem; name = " << itemName);
        return {};
    }

    return pSavedSearchItem->localId();
}

QModelIndex SavedSearchModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    return indexForLocalIdIndexIterator(it);
}

QString SavedSearchModel::itemNameForLocalId(const QString & localId) const
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::itemNameForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model:saved_search", "No saved search with such local id");
        return {};
    }

    return it->name();
}

AbstractItemModel::ItemInfo SavedSearchModel::itemInfoForLocalId(
    const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model:saved_search", "No saved search with such local id");
        return {};
    }

    AbstractItemModel::ItemInfo info;
    info.m_localId = it->localId();
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

QVector<AbstractItemModel::LinkedNotebookInfo>
SavedSearchModel::linkedNotebooksInfo() const
{
    return {};
}

QString SavedSearchModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    Q_UNUSED(linkedNotebookGuid)
    return {};
}

QModelIndex SavedSearchModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_pAllSavedSearchesRootItem)) {
        return {};
    }

    return indexForItem(m_pAllSavedSearchesRootItem);
}

QString SavedSearchModel::localIdForItemIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.internalId() == m_allSavedSearchesRootItemIndexId) {
        // All saved searches root item
        return {};
    }

    const int row = index.row();
    const int column = index.column();

    if ((row < 0) || (row >= static_cast<int>(m_data.size())) || (column < 0) ||
        (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return {};
    }

    const auto & item = m_data.get<ByIndex>()[static_cast<size_t>(row)];
    return item.localId();
}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (index.internalId() == m_allSavedSearchesRootItemIndexId) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Dirty)) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Synchronizable)) {
        QVariant synchronizable = dataImpl(index.row(), Column::Synchronizable);

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

    if (index.internalId() == m_allSavedSearchesRootItemIndexId) {
        if (index.column() == static_cast<int>(Column::Name)) {
            switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
            case Qt::ToolTipRole:
                return tr("All saved searches");
            }
        }

        return {};
    }

    const int rowIndex = index.row();
    const int columnIndex = index.column();

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
    if (!parent.isValid()) {
        // Parent is invisible root item
        return (m_pAllSavedSearchesRootItem ? 1 : 0);
    }

    if (parent.internalId() == m_allSavedSearchesRootItemIndexId) {
        // Number of saved search items is requested
        return static_cast<int>(m_data.size());
    }

    return 0;
}

int SavedSearchModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    return NUM_SAVED_SEARCH_MODEL_COLUMNS;
}

QModelIndex SavedSearchModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        // Parent is invisible root item
        if (row != 0) {
            return {};
        }

        if ((column < 0) || (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
            return {};
        }

        return createIndex(row, column, m_allSavedSearchesRootItemIndexId);
    }

    if (parent.internalId() == m_allSavedSearchesRootItemIndexId) {
        // Leaf item's index is requested
        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS) ||
            (parent.column() != static_cast<int>(Column::Name)))
        {
            return {};
        }

        return createIndex(row, column);
    }

    return {};
}

QModelIndex SavedSearchModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.internalId() == m_allSavedSearchesRootItemIndexId) {
        return {};
    }

    return createIndex(
        0, static_cast<int>(Column::Name), m_allSavedSearchesRootItemIndexId);
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

    if (modelIndex.internalId() == m_allSavedSearchesRootItemIndexId) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    const int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        QNDEBUG("model:saved_search", "Bad row");
        return false;
    }

    const int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
        QNDEBUG("model:saved_search", "Bad column");
        return false;
    }

    SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchItem item = index[static_cast<std::size_t>(rowIndex)];

    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
    {
        QString name = value.toString().trimmed();
        if (name == item.name()) {
            QNDEBUG("model:saved_search", "The name has not changed");
            return true;
        }

        const auto nameIt = nameIndex.find(name.toUpper());
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
        if (!validateSavedSearchName(name, &errorDescription)) {
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

        item.setDirty(
            item.isDirty() | (value.toBool() != item.isSynchronizable()));
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

    if (!parent.isValid()) {
        QNDEBUG(
            "model:saved_search",
            "Skipping the attempt to insert row "
                << "under the invisible root item");
        return false;
    }

    const auto grandparent = parent.parent();
    if (grandparent.isValid()) {
        QNDEBUG(
            "model:saved_search",
            "Skipping the attempt to insert row "
                << "under the leaf saved search item");
        return false;
    }

    auto & index = m_data.get<ByIndex>();
    const int numExistingSavedSearches = static_cast<int>(index.size());
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

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        SavedSearchItem item;
        item.setLocalId(UidGenerator::Generate());

        Q_UNUSED(
            m_savedSearchItemsNotYetInLocalStorageIds.insert(item.localId()));

        item.setName(nameForNewSavedSearch());
        item.setDirty(true);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        auto insertionResult = index.insert(index.begin() + row, item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    Q_EMIT layoutAboutToBeChanged();
    for (const auto & itemIt: qAsConst(addedItems)) {
        updateRandomAccessIndexWithRespectToSorting(*itemIt);
        updateSavedSearchInLocalStorage(*itemIt);
    }
    Q_EMIT layoutChanged();

    return true;
}

bool SavedSearchModel::removeRows(
    int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(!parent.isValid())) {
        QNDEBUG(
            "model:saved_search",
            "Skipping the attempt to remove rows "
                << "directly from under the invisible root item");
        return false;
    }

    const auto grandparent = parent.parent();
    if (grandparent.isValid()) {
        QNDEBUG(
            "model:saved_search",
            "Skipping the attempt to remove rows "
                << "from under the leaf saved search item");
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
        const auto it = index.begin() + row + i;
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

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        const auto it = index.begin() + row + i;

        qevercloud::SavedSearch savedSearch;
        savedSearch.setLocalId(it->localId());

        const auto requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(requestId))

        QNTRACE(
            "model:saved_search",
            "Emitting the request to expunge "
                << "the saved search from the local storage: request id = "
                << requestId << ", saved search local id: " << it->localId());

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

    const auto persistentIndices = persistentIndexList();
    QStringList localIdsToUpdate;

    for (const auto & index: qAsConst(persistentIndices)) {
        if (!index.isValid() ||
            (index.column() != static_cast<int>(Column::Name))) {
            localIdsToUpdate << QString();
            continue;
        }

        const auto * pItem = itemForIndex(index);
        if (!pItem) {
            localIdsToUpdate << QString();
            continue;
        }

        const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
        if (Q_UNLIKELY(!pSavedSearchItem)) {
            localIdsToUpdate << QString();
            continue;
        }

        localIdsToUpdate << pSavedSearchItem->localId();
    }

    auto & index = m_data.get<ByIndex>();

    std::vector<boost::reference_wrapper<const SavedSearchItem>> items(
        index.begin(), index.end());

    if (m_sortOrder == Qt::AscendingOrder) {
        std::sort(items.begin(), items.end(), LessByName());
    }
    else {
        std::sort(items.begin(), items.end(), GreaterByName());
    }

    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    for (const auto & localId: qAsConst(localIdsToUpdate)) {
        if (localId.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        replacementIndices << indexForLocalId(localId);
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void SavedSearchModel::onAddSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onAddSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

    const auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it != m_addSavedSearchRequestIds.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.erase(it));
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onAddSavedSearchFailed(
    qevercloud::SavedSearch search, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_addSavedSearchRequestIds.find(requestId);
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

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(search.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:saved_search",
            "Can't find the saved search item failed "
                << "to be added to the local storage within the model's items");
        return;
    }

    auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find the indexed reference to "
                << "the saved search item failed to be added to the local "
                   "storage: "
                << search);
        return;
    }

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));

    beginRemoveRows(
        indexForItem(m_pAllSavedSearchesRootItem), rowIndex, rowIndex);

    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();
}

void SavedSearchModel::onUpdateSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onUpdateSavedSearchComplete: "
            << search << "\nRequest id = " << requestId);

    const auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onUpdateSavedSearchFailed(
    qevercloud::SavedSearch search, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_updateSavedSearchRequestIds.find(requestId);
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
            << "search: local id = " << search.localId()
            << ", request id = " << requestId);

    Q_EMIT findSavedSearch(search, requestId);
}

void SavedSearchModel::onFindSavedSearchComplete(
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    const auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()
         ? m_findSavedSearchToPerformUpdateRequestIds.find(requestId)
         : m_findSavedSearchToPerformUpdateRequestIds.end());

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

        m_cache.put(search.localId(), search);
        auto & localIdIndex = m_data.get<ByLocalId>();
        const auto it = localIdIndex.find(search.localId());
        if (it != localIdIndex.end()) {
            updateSavedSearchInLocalStorage(*it);
        }
    }
}

void SavedSearchModel::onFindSavedSearchFailed(
    qevercloud::SavedSearch search, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto restoreUpdateIt =
        m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);

    const auto performUpdateIt =
        (restoreUpdateIt ==
         m_findSavedSearchToRestoreFailedUpdateRequestIds.end()
         ? m_findSavedSearchToPerformUpdateRequestIds.find(requestId)
         : m_findSavedSearchToPerformUpdateRequestIds.end());

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
    QList<qevercloud::SavedSearch> foundSearches, QUuid requestId)
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

        m_listSavedSearchesOffset +=
            static_cast<std::size_t>(foundSearches.size());

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
    ErrorString errorDescription, QUuid requestId) // NOLINT
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
    qevercloud::SavedSearch search, QUuid requestId) // NOLINT
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::onExpungeSavedSearchComplete: search = "
            << search << "\nRequest id = " << requestId);

    const auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it != m_expungeSavedSearchRequestIds.end()) {
        Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(search.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG(
            "model:saved_search",
            "Expunged saved search was not found "
                << "within the saved search model items: " << search);
        return;
    }

    auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't project the local id index "
                       "iterator to the random access index iterator within "
                       "the saved searches model"));

        QNWARNING("model:saved_search", error);
        Q_EMIT notifyError(error);
        return;
    }

    Q_EMIT aboutToRemoveSavedSearches();

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));

    beginRemoveRows(
        indexForItem(m_pAllSavedSearchesRootItem), rowIndex, rowIndex);

    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();

    Q_EMIT removedSavedSearches();
}

void SavedSearchModel::onExpungeSavedSearchFailed(
    qevercloud::SavedSearch search, ErrorString errorDescription, // NOLINT
    QUuid requestId)
{
    const auto it = m_expungeSavedSearchRequestIds.find(requestId);
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

void SavedSearchModel::createConnections( // NOLINT
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

    const LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    const LocalStorageManager::ListSavedSearchesOrder order =
        LocalStorageManager::ListSavedSearchesOrder::NoOrder;

    const LocalStorageManager::OrderDirection direction =
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

void SavedSearchModel::onSavedSearchAddedOrUpdated(
    const qevercloud::SavedSearch & search)
{
    auto & rowIndex = m_data.get<ByIndex>();
    auto & localIdIndex = m_data.get<ByLocalId>();

    m_cache.put(search.localId(), search);

    SavedSearchItem item(search.localId());

    if (search.guid()) {
        item.setGuid(*search.guid());
    }

    if (search.name()) {
        item.setName(*search.name());
    }

    if (search.query()) {
        item.setQuery(*search.query());
    }

    item.setSynchronizable(!search.isLocalOnly());
    item.setDirty(search.isLocallyModified());
    item.setFavorited(search.isLocallyFavorited());

    auto itemIt = localIdIndex.find(search.localId());
    if (itemIt == localIdIndex.end()) {
        checkAndCreateModelRootItems();
        Q_EMIT aboutToAddSavedSearch();

        const int row = rowForNewItem(item);
        const auto parentIndex = indexForItem(m_pAllSavedSearchesRootItem);

        beginInsertRows(parentIndex, row, row);
        const auto insertionResult = localIdIndex.insert(item);
        itemIt = insertionResult.first;
        endInsertRows();

        updateRandomAccessIndexWithRespectToSorting(*itemIt);

        const auto addedSavedSearchIndex = indexForLocalId(search.localId());
        Q_EMIT addedSavedSearch(addedSavedSearchIndex);

        return;
    }

    const auto savedSearchIndexBefore = indexForLocalId(search.localId());
    Q_EMIT aboutToUpdateSavedSearch(savedSearchIndexBefore);

    localIdIndex.replace(itemIt, item);

    const auto indexIt = m_data.project<ByIndex>(itemIt);
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

    const qint64 position = std::distance(rowIndex.begin(), indexIt);
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

    const auto modelIndexFrom = createIndex(static_cast<int>(position), 0);

    const auto modelIndexTo = createIndex(
        static_cast<int>(position), NUM_SAVED_SEARCH_MODEL_COLUMNS - 1);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    updateRandomAccessIndexWithRespectToSorting(item);

    const auto savedSearchIndexAfter = indexForLocalId(search.localId());
    Q_EMIT updatedSavedSearch(savedSearchIndexAfter);
}

QVariant SavedSearchModel::dataImpl(const int row, const Column column) const
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

    const auto textData = dataImpl(row, column);
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

int SavedSearchModel::rowForNewItem(const SavedSearchItem & newItem) const
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

    return static_cast<int>(std::distance(nameIndex.begin(), it));
}

void SavedSearchModel::updateRandomAccessIndexWithRespectToSorting(
    const SavedSearchItem & item)
{
    if (m_sortedColumn != Column::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(item.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find saved search item by local id: " << item);
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

    const int oldRow =
        static_cast<int>(std::distance(index.begin(), originalRandomAccessIt));

    const int newRow =
        static_cast<int>(std::distance(index.begin(), newRandomAccessIt));

    if (oldRow == newRow) {
        QNDEBUG("model:saved_search", "Already at the appropriate row");
        return;
    }

    if (oldRow + 1 == newRow) {
        QNDEBUG("model:saved_search", "Already before the appropriate row");
        return;
    }

    const auto parentIndex = indexForItem(m_pAllSavedSearchesRootItem);

    if (!beginMoveRows(parentIndex, oldRow, oldRow, parentIndex, newRow)) {
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
    const SavedSearchItem & item)
{
    QNDEBUG(
        "model:saved_search",
        "SavedSearchModel::updateSavedSearchInLocalStorage: local id = "
            << item);

    qevercloud::SavedSearch savedSearch;

    const auto notYetSavedItemIt =
        m_savedSearchItemsNotYetInLocalStorageIds.find(item.localId());

    if (notYetSavedItemIt == m_savedSearchItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG("model:saved_search", "Updating the saved search");

        const auto * pCachedSearch = m_cache.get(item.localId());
        if (Q_UNLIKELY(!pCachedSearch)) {
            const auto requestId = QUuid::createUuid();
            Q_UNUSED(
                m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))

            qevercloud::SavedSearch dummy;
            dummy.setLocalId(item.localId());

            Q_EMIT findSavedSearch(dummy, requestId);

            QNDEBUG(
                "model:saved_search",
                "Emitted the request to find the saved search: local id = "
                    << item.localId() << ", request id = " << requestId);
            return;
        }

        savedSearch = *pCachedSearch;
    }

    savedSearch.setLocalId(item.localId());
    savedSearch.setGuid(toOptional(item.guid()));
    savedSearch.setName(toOptional(item.name()));
    savedSearch.setQuery(toOptional(item.query()));
    savedSearch.setLocalOnly(!item.isSynchronizable());
    savedSearch.setLocallyModified(item.isDirty());
    savedSearch.setLocallyFavorited(item.isFavorited());

    const auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_savedSearchItemsNotYetInLocalStorageIds.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.insert(requestId));
        Q_EMIT addSavedSearch(savedSearch, requestId);

        QNTRACE(
            "model:saved_search",
            "Emitted the request to add the saved search to the local storage: "
                << "id = " << requestId << ", saved search: " << savedSearch);

        Q_UNUSED(
            m_savedSearchItemsNotYetInLocalStorageIds.erase(notYetSavedItemIt))
    }
    else {
        Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId));

        // While the saved search is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(savedSearch.localId()))

        Q_EMIT updateSavedSearch(savedSearch, requestId);

        QNTRACE(
            "model:saved_search",
            "Emitted the request to update the saved search in the local "
                << "storage: id = " << requestId
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

    const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag to the saved search: "
                       "non-saved search item is targeted"));
        return;
    }

    if (favorited == pSavedSearchItem->isFavorited()) {
        QNDEBUG("model:saved_search", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    const auto it = localIdIndex.find(pSavedSearchItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "the modified saved search entry was not found "
                       "within the model"));
        return;
    }

    SavedSearchItem itemCopy(*pSavedSearchItem);
    itemCopy.setFavorited(favorited);

    localIdIndex.replace(it, itemCopy);

    updateSavedSearchInLocalStorage(itemCopy);
}

void SavedSearchModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_pInvisibleRootItem)) {
        m_pInvisibleRootItem = new InvisibleRootItem;
        QNDEBUG("model:saved_search", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_pAllSavedSearchesRootItem)) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_pAllSavedSearchesRootItem = new AllSavedSearchesRootItem;
        m_pAllSavedSearchesRootItem->setParent(m_pInvisibleRootItem);
        endInsertRows();
        QNDEBUG("model:saved_search", "Created all saved searches root item");
    }
}

QModelIndex SavedSearchModel::indexForLocalIdIndexIterator(
    const SavedSearchDataByLocalId::const_iterator it) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (it == localIdIndex.end()) {
        return {};
    }

    const auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model:saved_search",
            "Can't find the indexed reference to "
                << "the saved search item: " << *it);
        return {};
    }

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));

    return createIndex(rowIndex, static_cast<int>(Column::Name));
}

bool SavedSearchModel::LessByName::operator()(
    const SavedSearchItem & lhs, const SavedSearchItem & rhs) const noexcept
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

bool SavedSearchModel::GreaterByName::operator()(
    const SavedSearchItem & lhs, const SavedSearchItem & rhs) const noexcept
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const SavedSearchModel::Column column)
{
    using Column = SavedSearchModel::Column;

    switch (column) {
    case Column::Name:
        dbg << "name";
        break;
    case Column::Query:
        dbg << "query";
        break;
    case Column::Synchronizable:
        dbg << "synchronizable";
        break;
    case Column::Dirty:
        dbg << "dirty";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(column) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
