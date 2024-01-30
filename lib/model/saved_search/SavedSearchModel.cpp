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

#include "SavedSearchModel.h"

#include "AllSavedSearchesRootItem.h"
#include "InvisibleSavedSearchRootItem.h"

#include <lib/exception/Utils.h>
#include <lib/model/common/NewItemNameGenerator.hpp>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/UidGenerator.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    QNWARNING(                                                                 \
        "model::SavedSearchModel",                                                  \
        errorDescription << QLatin1String("" __VA_ARGS__ ""));                 \
    Q_EMIT notifyError(errorDescription) // REPORT_ERROR

namespace quentier {

namespace {

constexpr int gSavedSearchModelColumnCount = 4;

template <class T>
void printSavedSearchModelColumn(const SavedSearchModel::Column column, T & t)
{
    using Column = SavedSearchModel::Column;

    switch (column) {
    case Column::Name:
        t << "name";
        break;
    case Column::Query:
        t << "query";
        break;
    case Column::Synchronizable:
        t << "synchronizable";
        break;
    case Column::Dirty:
        t << "dirty";
        break;
    default:
        t << "Unknown (" << static_cast<qint64>(column) << ")";
        break;
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

SavedSearchModel::SavedSearchModel(
    Account account,
    local_storage::ILocalStoragePtr localStorage,
    SavedSearchCache & cache, QObject * parent) :
    AbstractItemModel{std::move(account), parent},
    m_localStorage{std::move(localStorage)},
    m_cache{cache}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            QStringLiteral("SavedSearchModel ctor: local storage is null")}};
    }

    connectToLocalStorageEvents(m_localStorage->notifier());
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel() = default;

ISavedSearchModelItem * SavedSearchModel::itemForIndex(
    const QModelIndex & modelIndex) const
{
    QNTRACE(
        "model::SavedSearchModel",
        "SavedSearchModel::itemForIndex: row = " << modelIndex.row());

    if (!modelIndex.isValid()) {
        QNTRACE("model::SavedSearchModel", "Index is invalid");
        return m_invisibleRootItem;
    }

    if (!modelIndex.parent().isValid()) {
        return m_allSavedSearchesRootItem;
    }

    const int row = modelIndex.row();
    const auto & index = m_data.get<ByIndex>();
    if (row >= static_cast<int>(index.size())) {
        QNTRACE(
            "model::SavedSearchModel",
            "Index's row is greater than the size of row index");
        return nullptr;
    }

    return const_cast<SavedSearchItem *>(
        &(index[static_cast<std::size_t>(row)]));
}

QModelIndex SavedSearchModel::indexForItem(
    const ISavedSearchModelItem * item) const
{
    if (!item) {
        return {};
    }

    if (item == m_invisibleRootItem) {
        return {};
    }

    if (item == m_allSavedSearchesRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name),
            m_allSavedSearchesRootItemIndexId);
    }

    auto * savedSearchItem = item->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!savedSearchItem)) {
        QNWARNING(
            "model::SavedSearchModel",
            "Failed to cast item to saved search one: " << *item);
        return {};
    }

    return indexForLocalId(savedSearchItem->localId());
}

QModelIndex SavedSearchModel::indexForSavedSearchName(
    const QString & savedSearchName) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();
    if (const auto it = nameIndex.find(savedSearchName.toUpper());
        it != nameIndex.end())
    {
        const auto & item = *it;
        return indexForItem(&item);
    }

    return {};
}

QString SavedSearchModel::queryForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (const auto it = localIdIndex.find(localId); it != localIdIndex.end()) {
        return it->query();
    }

    return {};
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
        "model::SavedSearchModel",
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
    m_savedSearchItemsNotYetInLocalStorageIds.insert(item.localId());

    item.setName(std::move(savedSearchName));
    item.setQuery(std::move(searchQuery));
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    checkAndCreateModelRootItems();
    Q_EMIT aboutToAddSavedSearch();

    const int row = static_cast<int>(localIdIndex.size());

    beginInsertRows(indexForItem(m_allSavedSearchesRootItem), row, row);
    localIdIndex.insert(item);
    endInsertRows();

    updateRandomAccessIndexWithRespectToSorting(item);
    updateSavedSearchInLocalStorage(item);

    auto addedSavedSearchIndex = indexForLocalId(item.localId());
    Q_EMIT addedSavedSearch(addedSavedSearchIndex);

    return addedSavedSearchIndex;
}

void SavedSearchModel::favoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "model::SavedSearchModel",
        "SavedSearchModel::favoriteSavedSearch: "
            << "index: is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column());

    setSavedSearchFavorited(index, true);
}

void SavedSearchModel::unfavoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(
        "model::SavedSearchModel",
        "SavedSearchModel::unfavoriteSavedSearch: "
            << "index: is valid = " << (index.isValid() ? "true" : "false")
            << ", row = " << index.row() << ", column = " << index.column());

    setSavedSearchFavorited(index, false);
}

QString SavedSearchModel::localIdForItemName(
    const QString & itemName,
    [[maybe_unused]] const QString & linkedNotebookGuid) const
{
    QNDEBUG(
        "model::SavedSearchModel",
        "SavedSearchModel::localIdForItemName: name = " << itemName);

    const auto index = indexForSavedSearchName(itemName);
    const auto * item = itemForIndex(index);
    if (!item) {
        QNTRACE(
            "model::SavedSearchModel",
            "No saved search with such name was found");
        return {};
    }

    const auto * savedSearchItem = item->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!savedSearchItem)) {
        QNWARNING(
            "model::SavedSearchModel",
            "Failed to case item found by name to "
                << "SavedSearchItem; name = " << itemName);
        return {};
    }

    return savedSearchItem->localId();
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
        "model::SavedSearchModel",
        "SavedSearchModel::itemNameForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (const auto it = localIdIndex.find(localId); it != localIdIndex.end()) {
        return it->name();
    }

    QNTRACE("model::SavedSearchModel", "No saved search with such local id");
    return {};
}

AbstractItemModel::ItemInfo SavedSearchModel::itemInfoForLocalId(
    const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (const auto it = localIdIndex.find(localId); it != localIdIndex.end()) {
        AbstractItemModel::ItemInfo info;
        info.m_localId = it->localId();
        info.m_name = it->name();
    }

    QNTRACE("model::SavedSearchModel", "No saved search with such local id");
    return {};
}

QStringList SavedSearchModel::itemNames(
    const QString & linkedNotebookGuid) const
{
    if (!linkedNotebookGuid.isEmpty()) {
        return {};
    }

    return savedSearchNames();
}

QList<AbstractItemModel::LinkedNotebookInfo>
SavedSearchModel::linkedNotebooksInfo() const
{
    return {};
}

QString SavedSearchModel::linkedNotebookUsername(
    [[maybe_unused]] const QString & linkedNotebookGuid) const
{
    return {};
}

QModelIndex SavedSearchModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_allSavedSearchesRootItem)) {
        return {};
    }

    return indexForItem(m_allSavedSearchesRootItem);
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
        (column >= gSavedSearchModelColumnCount))
    {
        return {};
    }

    const auto & item = m_data.get<ByIndex>()[static_cast<std::size_t>(row)];
    return item.localId();
}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    auto indexFlags = QAbstractItemModel::flags(index);
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
        (columnIndex < 0) || (columnIndex >= gSavedSearchModelColumnCount))
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
        return QVariant{section + 1};
    }

    switch (static_cast<Column>(section)) {
    case Column::Name:
        return QVariant{tr("Name")};
    case Column::Query:
        return QVariant{tr("Query")};
    case Column::Synchronizable:
        return QVariant{tr("Synchronizable")};
    case Column::Dirty:
        return QVariant{tr("Changed")};
    default:
        return {};
    }
}

int SavedSearchModel::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        // Parent is invisible root item
        return (m_allSavedSearchesRootItem ? 1 : 0);
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

    return gSavedSearchModelColumnCount;
}

QModelIndex SavedSearchModel::index(
    const int row, const int column, const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        // Parent is invisible root item
        if (row != 0) {
            return {};
        }

        if ((column < 0) || (column >= gSavedSearchModelColumnCount)) {
            return {};
        }

        return createIndex(row, column, m_allSavedSearchesRootItemIndexId);
    }

    if (parent.internalId() == m_allSavedSearchesRootItemIndexId) {
        // Leaf item's index is requested
        if ((row < 0) || (row >= static_cast<int>(m_data.size())) ||
            (column < 0) || (column >= gSavedSearchModelColumnCount) ||
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
    [[maybe_unused]] const int section,
    [[maybe_unused]] const Qt::Orientation orientation,
    [[maybe_unused]] const QVariant & value, [[maybe_unused]] const int role)
{
    return false;
}

bool SavedSearchModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, const int role)
{
    QNDEBUG(
        "model::SavedSearchModel",
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

    int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        QNDEBUG("model::SavedSearchModel", "Bad row");
        return false;
    }

    int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= gSavedSearchModelColumnCount)) {
        QNDEBUG("model::SavedSearchModel", "Bad column");
        return false;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    auto & index = m_data.get<ByIndex>();
    auto item = index[static_cast<size_t>(rowIndex)];

    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
    {
        QString name = value.toString().trimmed();
        bool changed = (name != item.name());
        if (!changed) {
            QNDEBUG("model::SavedSearchModel", "The name has not changed");
            return true;
        }

        auto nameIt = nameIndex.find(name.toUpper());
        if (nameIt != nameIndex.end()) {
            ErrorString error(
                QT_TR_NOOP("Can't rename the saved search: no two saved "
                           "searches within the account are allowed to "
                           "have the same name in case-insensitive manner"));
            error.details() = name;
            QNINFO("model::SavedSearchModel", error);
            Q_EMIT notifyError(error);
            return false;
        }

        ErrorString errorDescription;
        if (!validateSavedSearchName(name, &errorDescription)) {
            ErrorString error(QT_TR_NOOP("Can't rename saved search"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();

            QNINFO(
                "model::SavedSearchModel",
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
            QNDEBUG("model::SavedSearchModel", "Query is empty");
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
                QT_TR_NOOP("Can't make saved search synchronizable "
                           "within a local account"));
            QNINFO("model::SavedSearchModel", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (item.isSynchronizable()) {
            ErrorString error(
                QT_TR_NOOP("Can't make already synchronizable "
                           "saved search not synchronizable"));

            QNINFO(
                "model::SavedSearchModel",
                error << ", already synchronizable saved search item: "
                      << item);
            Q_EMIT notifyError(error);
            return false;
        }

        item.setDirty(
            item.isDirty() || (value.toBool() != item.isSynchronizable()));
        item.setSynchronizable(value.toBool());
        break;
    }
    default:
        QNWARNING(
            "model::SavedSearchModel",
            "Unidentified column: " << modelIndex.column());
        return false;
    }

    index.replace(index.begin() + rowIndex, item);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    updateRandomAccessIndexWithRespectToSorting(item);
    updateSavedSearchInLocalStorage(item);

    QNDEBUG("model::SavedSearchModel", "Successfully set the data");
    return true;
}

bool SavedSearchModel::insertRows(
    const int row, const int count, const QModelIndex & parent)
{
    QNTRACE(
        "model::SavedSearchModel",
        "SavedSearchModel::insertRows: row = " << row << ", count = " << count);

    if (!parent.isValid()) {
        QNDEBUG(
            "model::SavedSearchModel",
            "Skipping the attempt to insert row under the invisible root item");
        return false;
    }

    const auto grandparent = parent.parent();
    if (grandparent.isValid()) {
        QNDEBUG(
            "model::SavedSearchModel",
            "Skipping the attempt to insert row under the leaf saved search "
            "item");
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
        QNINFO("model::SavedSearchModel", error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<SavedSearchDataByIndex::iterator> addedItems;
    addedItems.reserve(static_cast<std::size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        SavedSearchItem item;
        item.setLocalId(UidGenerator::Generate());
        m_savedSearchItemsNotYetInLocalStorageIds.insert(item.localId());

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
    if (Q_UNLIKELY(!parent.isValid())) {
        QNDEBUG(
            "model::SavedSearchModel",
            "Skipping the attempt to remove rows directly from under the "
            "invisible root item");
        return false;
    }

    const auto grandparent = parent.parent();
    if (grandparent.isValid()) {
        QNDEBUG(
            "model::SavedSearchModel",
            "Skipping the attempt to remove rows from under the leaf saved "
            "search item");
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size()))) {
        ErrorString error(
            QT_TR_NOOP("Detected attempt to remove more rows than "
                       "the saved search model contains"));

        QNINFO(
            "model::SavedSearchModel",
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
                "model::SavedSearchModel", error << ", saved search item: " << *it);

            Q_EMIT notifyError(error);
            return false;
        }
    }

    Q_EMIT aboutToRemoveSavedSearches();

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        const auto it = index.begin() + row + i;

        QNTRACE(
            "model::SavedSearchModel",
            "Requesting to expunge "
                << "saved search from local storage: local id: "
                << it->localId());

        auto expungeSavedSearchFuture =
            m_localStorage->expungeSavedSearchByLocalId(it->localId());

        auto expungeSavedSearchThenFuture = threading::then(
            std::move(expungeSavedSearchFuture), this,
            [this, localId = it->localId()] {
                onSavedSearchExpunged(localId);
            });

        threading::onFailed(
            std::move(expungeSavedSearchThenFuture), this,
            [this, localId = it->localId()](const QException & e) {
                QNWARNING(
                    "model::SavedSearchModel",
                    "Failed to expunge saved search by local id: " << e.what());
                auto message = exceptionMessage(e);
                Q_EMIT notifyError(std::move(message));
            });
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    endRemoveRows();

    Q_EMIT removedSavedSearches();
    return true;
}

void SavedSearchModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(
        "model::SavedSearchModel",
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
            "model::SavedSearchModel",
            "The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;

    Q_EMIT layoutAboutToBeChanged();

    const auto persistentIndices = persistentIndexList();
    QStringList localIdsToUpdate;

    for (const auto & index: std::as_const(persistentIndices)) {
        if (!index.isValid() ||
            (index.column() != static_cast<int>(Column::Name))) {
            localIdsToUpdate << QString{};
            continue;
        }

        const auto * item = itemForIndex(index);
        if (!item) {
            localIdsToUpdate << QString{};
            continue;
        }

        const auto * savedSearchItem = item->cast<SavedSearchItem>();
        if (Q_UNLIKELY(!savedSearchItem)) {
            localIdsToUpdate << QString{};
            continue;
        }

        localIdsToUpdate << savedSearchItem->localId();
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
    for (const auto & localId: std::as_const(localIdsToUpdate)) {
        if (localId.isEmpty()) {
            replacementIndices << QModelIndex();
            continue;
        }

        auto newIndex = indexForLocalId(localId);
        replacementIndices << newIndex;
    }

    changePersistentIndexList(persistentIndices, replacementIndices);

    Q_EMIT layoutChanged();
}

void SavedSearchModel::connectToLocalStorageEvents(
    local_storage::ILocalStorageNotifier * notifier)
{
    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::savedSearchPut,
        this,
        [this](const qevercloud::SavedSearch & savedSearch) {
            onSavedSearchAddedOrUpdated(savedSearch);
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::savedSearchExpunged,
        this,
        [this](const QString & localId) {
            removeSavedSearchItem(localId);
        });
}

void SavedSearchModel::requestSavedSearchesList()
{
    QNDEBUG(
        "model::SavedSearchModel",
        "SavedSearchModel::requestSavedSearchesList: "
            << "offset = " << m_listSavedSearchesOffset);

    local_storage::ILocalStorage::ListSavedSearchesOptions options;
    options.m_order =
        local_storage::ILocalStorage::ListSavedSearchesOrder::NoOrder;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;
    options.m_offset = m_listSavedSearchesOffset;
    options.m_limit = 100;

    QNTRACE(
        "model::SavedSearchModel",
        "Requesting a list of saved searches: offset = "
            << m_listSavedSearchesOffset);

    auto listSavedSearchesFuture = m_localStorage->listSavedSearches(options);

    auto listSavedSearchesThenFuture = threading::then(
        std::move(listSavedSearchesFuture), this,
        [this](const QList<qevercloud::SavedSearch> & savedSearches) {
            onSavedSearchesListed(savedSearches);
        });

    threading::onFailed(
        std::move(listSavedSearchesThenFuture), this,
        [this](const QException & e) {
            QNWARNING(
                "model::SavedSearchModel",
                "Failed to list saved searches: " << e.what());
            auto message = exceptionMessage(e);
            Q_EMIT notifyError(std::move(message));
        });
}

void SavedSearchModel::onSavedSearchesListed(
    const QList<qevercloud::SavedSearch> & savedSearches)
{
    for (const auto & savedSearch: std::as_const(savedSearches)) {
        onSavedSearchAddedOrUpdated(savedSearch);
    }

    if (!savedSearches.isEmpty()) {
        m_listSavedSearchesOffset +=
            static_cast<std::size_t>(savedSearches.size());
        requestSavedSearchesList();
        return;
    }

    m_allSavedSearchesListed = true;
    Q_EMIT notifyAllSavedSearchesListed();
    Q_EMIT notifyAllItemsListed();
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
        const auto parentIndex = indexForItem(m_allSavedSearchesRootItem);

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

        QNWARNING("model::SavedSearchModel", error);
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

        QNWARNING("model::SavedSearchModel", error);
        Q_EMIT notifyError(error);
        Q_EMIT updatedSavedSearch(QModelIndex{});
        return;
    }

    const auto modelIndexFrom = createIndex(static_cast<int>(position), 0);

    const auto modelIndexTo = createIndex(
        static_cast<int>(position), gSavedSearchModelColumnCount - 1);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    updateRandomAccessIndexWithRespectToSorting(item);

    QModelIndex savedSearchIndexAfter = indexForLocalId(search.localId());
    Q_EMIT updatedSavedSearch(savedSearchIndexAfter);
}

QVariant SavedSearchModel::dataImpl(const int row, const Column column) const
{
    QNTRACE(
        "model::SavedSearchModel",
        "SavedSearchModel::dataImpl: row = " << row << ", column = " << column);

    if (Q_UNLIKELY((row < 0) || (row > static_cast<int>(m_data.size())))) {
        QNTRACE(
            "model::SavedSearchModel",
            "Invalid row " << row << ", data size is " << m_data.size());
        return {};
    }

    const auto & item = m_data.get<ByIndex>()[static_cast<std::size_t>(row)];
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
        "model::SavedSearchModel",
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

    return QVariant{std::move(accessibleText)};
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
    auto itemIt = localIdIndex.find(item.localId());
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNWARNING(
            "model::SavedSearchModel",
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

    int oldRow =
        static_cast<int>(std::distance(index.begin(), originalRandomAccessIt));

    int newRow =
        static_cast<int>(std::distance(index.begin(), newRandomAccessIt));

    if (oldRow == newRow) {
        QNDEBUG("model::SavedSearchModel", "Already at the appropriate row");
        return;
    }
    else if (oldRow + 1 == newRow) {
        QNDEBUG("model::SavedSearchModel", "Already before the appropriate row");
        return;
    }

    const auto parentIndex = indexForItem(m_allSavedSearchesRootItem);

    bool res = beginMoveRows(parentIndex, oldRow, oldRow, parentIndex, newRow);

    if (!res) {
        QNWARNING(
            "model::SavedSearchModel",
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
        "model::SavedSearchModel",
        "SavedSearchModel::updateSavedSearchInLocalStorage: local id = "
            << item.localId());

    qevercloud::SavedSearch savedSearch;

    auto notYetSavedItemIt =
        m_savedSearchItemsNotYetInLocalStorageIds.find(item.localId());

    if (notYetSavedItemIt == m_savedSearchItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG("model::SavedSearchModel", "Updating saved search");

        const auto * cachedSearch = m_cache.get(item.localId());
        if (Q_UNLIKELY(!cachedSearch)) {
            auto findSavedSearchFuture =
                m_localStorage->findSavedSearchByLocalId(item.localId());

            auto updateSavedSearchFuture = threading::then(
                std::move(findSavedSearchFuture), this,
                [this, localId = item.localId()](
                    const std::optional<qevercloud::SavedSearch> &
                        savedSearch) {
                    if (Q_UNLIKELY(!savedSearch)) {
                        ErrorString error{QT_TR_NOOP(
                            "Could not find saved search in local storage by "
                            "local id")};
                        error.details() = localId;
                        QNWARNING("model::SavedSearchModel", error);
                        Q_EMIT notifyError(std::move(error));
                        return;
                    }

                    m_cache.put(localId, *savedSearch);
                    auto & localIdIndex = m_data.get<ByLocalId>();
                    if (const auto it = localIdIndex.find(localId);
                        it != localIdIndex.end())
                    {
                        updateSavedSearchInLocalStorage(*it);
                    }
                });

            threading::onFailed(
                std::move(updateSavedSearchFuture), this,
                [this, localId = item.localId()](const QException & e) {
                    auto message = exceptionMessage(e);
                    QNWARNING(
                        "model::SavedSearchModel",
                        "Failed to find and update saved search in local "
                            << "storage; local id: " << localId << ", error: "
                            << message);
                    Q_EMIT notifyError(std::move(message));
                });

            return;
        }

        savedSearch = *cachedSearch;
    }

    savedSearch.setLocalId(item.localId());
    savedSearch.setGuid(item.guid());
    savedSearch.setName(item.name());
    savedSearch.setQuery(item.query());
    savedSearch.setLocalOnly(!item.isSynchronizable());
    savedSearch.setLocallyModified(item.isDirty());
    savedSearch.setLocallyFavorited(item.isFavorited());

    if (notYetSavedItemIt != m_savedSearchItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG(
            "model::SavedSearchModel",
            "Adding saved search to local storage: " << savedSearch);

        m_savedSearchItemsNotYetInLocalStorageIds.erase(notYetSavedItemIt);

        auto putSavedSearchFuture =
            m_localStorage->putSavedSearch(std::move(savedSearch));

        threading::onFailed(
            std::move(putSavedSearchFuture), this,
            [this, localId = item.localId()](const QException & e) {
                auto message = exceptionMessage(e);
                QNWARNING(
                    "model::SavedSearchModel",
                    "Failed to add saved search to local storage: " << message);
                Q_EMIT notifyError(std::move(message));
                removeSavedSearchItem(localId);
            });

        return;
    }

    // While the saved search is being updated in the local storage,
    // remove its stale copy from the cache
    m_cache.remove(savedSearch.localId());

    QNDEBUG(
        "model::SavedSearchModel",
        "Updating saved search in local storage: " << savedSearch);

    auto putSavedSearchFuture =
        m_localStorage->putSavedSearch(std::move(savedSearch));

    threading::onFailed(
        std::move(putSavedSearchFuture), this,
        [this, localId = item.localId()](const QException & e) {
            auto message = exceptionMessage(e);
            QNWARNING(
                "model::SavedSearchModel",
                "Failed to update saved search in local storage: " << message);
            Q_EMIT notifyError(std::move(message));

            // Try to restore the saved search to its actual version from
            // the local storage
            restoreSavedSearchItemFromLocalStorage(localId);
        });
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

    const auto * item = itemForIndex(index);
    if (Q_UNLIKELY(!item)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "can't find the model item corresponding to index"));
        return;
    }

    const auto * savedSearchItem = item->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!savedSearchItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag to the saved search: "
                       "non-saved search item is targeted"));
        return;
    }

    if (favorited == savedSearchItem->isFavorited()) {
        QNDEBUG(
            "model::SavedSearchModel", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    auto it = localIdIndex.find(savedSearchItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the saved search: "
                       "the modified saved search entry was not found "
                       "within the model"));
        return;
    }

    SavedSearchItem itemCopy(*savedSearchItem);
    itemCopy.setFavorited(favorited);

    localIdIndex.replace(it, itemCopy);

    updateSavedSearchInLocalStorage(itemCopy);
}

void SavedSearchModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_invisibleRootItem)) {
        m_invisibleRootItem = new InvisibleSavedSearchRootItem;
        QNDEBUG("model::SavedSearchModel", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_allSavedSearchesRootItem)) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_allSavedSearchesRootItem = new AllSavedSearchesRootItem;
        m_allSavedSearchesRootItem->setParent(m_invisibleRootItem);
        endInsertRows();
        QNDEBUG("model::SavedSearchModel", "Created all saved searches root item");
    }
}

QModelIndex SavedSearchModel::indexForLocalIdIndexIterator(
    const SavedSearchDataByLocalId::const_iterator it) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (it == localIdIndex.end()) {
        return QModelIndex{};
    }

    const auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model::SavedSearchModel",
            "Can't find the indexed reference to the saved search item: "
                << *it);
        return QModelIndex{};
    }

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, static_cast<int>(Column::Name));
}

void SavedSearchModel::removeSavedSearchItem(const QString & localId)
{
    Q_UNUSED(m_cache.remove(localId))

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        return;
    }

    auto & index = m_data.get<ByIndex>();
    const auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(
            "model::SavedSearchModel",
            "Can't find indexed reference to saved search item "
                << "which failed to be put to the local storage: " << localId);
        return;
    }

    const int rowIndex =
        static_cast<int>(std::distance(index.begin(), indexIt));

    beginRemoveRows(
        indexForItem(m_allSavedSearchesRootItem), rowIndex, rowIndex);

    m_data.erase(indexIt);
    endRemoveRows();
}

void SavedSearchModel::restoreSavedSearchItemFromLocalStorage(
    const QString & localId)
{
    auto findSavedSearchFuture =
        m_localStorage->findSavedSearchByLocalId(localId);

    auto findSavedSearchThenFuture = threading::then(
        std::move(findSavedSearchFuture), this,
        [this,
         localId](const std::optional<qevercloud::SavedSearch> & savedSearch) {
            if (Q_UNLIKELY(!savedSearch)) {
                QNWARNING(
                    "model::SavedSearchModel",
                    "Could not find saved search by local id in local storage");
                removeSavedSearchItem(localId);
                return;
            }

            onSavedSearchAddedOrUpdated(*savedSearch);
        });

    threading::onFailed(
        std::move(findSavedSearchThenFuture), this,
        [this, localId](const QException & e) {
            auto message = exceptionMessage(e);
            QNWARNING(
                "model::SavedSearchModel",
                "Failed to restore saved search from local storage: "
                    << message);
            Q_EMIT notifyError(std::move(message));
        });
}

bool SavedSearchModel::LessByName::operator()(
    const SavedSearchItem & lhs, const SavedSearchItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

bool SavedSearchModel::GreaterByName::operator()(
    const SavedSearchItem & lhs, const SavedSearchItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const SavedSearchModel::Column column)
{
    printSavedSearchModelColumn(column, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const SavedSearchModel::Column column)
{
    printSavedSearchModelColumn(column, strm);
    return strm;
}

} // namespace quentier
