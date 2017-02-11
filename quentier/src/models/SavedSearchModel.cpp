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

#include "SavedSearchModel.h"
#include "NewItemNameGenerator.hpp"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>
#include <limits>
#include <algorithm>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

#define NUM_SAVED_SEARCH_MODEL_COLUMNS (4)

#define REPORT_ERROR(error, ...) \
    ErrorString errorDescription(error); \
    QNWARNING(errorDescription << QStringLiteral("" __VA_ARGS__ "")); \
    emit notifyError(errorDescription)

namespace quentier {

SavedSearchModel::SavedSearchModel(const Account & account, LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
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
    createConnections(localStorageManagerThreadWorker);
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel()
{}

void SavedSearchModel::updateAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::updateAccount: ") << account);
    m_account = account;
}

const SavedSearchModelItem * SavedSearchModel::itemForIndex(const QModelIndex & modelIndex) const
{
    QNTRACE(QStringLiteral("SavedSearchModel::itemForIndex: row = ") << modelIndex.row());

    if (!modelIndex.isValid()) {
        QNTRACE(QStringLiteral("Index is invalid"));
        return Q_NULLPTR;
    }

    int row = modelIndex.row();

    const SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    if (row >= static_cast<int>(index.size())) {
        QNTRACE(QStringLiteral("Index's row is greater than the size of the row index"));
        return Q_NULLPTR;
    }

    return &(index[static_cast<size_t>(row)]);
}

QModelIndex SavedSearchModel::indexForItem(const SavedSearchModelItem * pItem) const
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

QModelIndex SavedSearchModel::indexForSavedSearchName(const QString & savedSearchName) const
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

QModelIndex SavedSearchModel::createSavedSearch(const QString & savedSearchName, const QString & searchQuery,
                                                ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::createSavedSearch: saved search name = ") << savedSearchName
            << QStringLiteral(", search query = ") << searchQuery);

    if (savedSearchName.isEmpty()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search name is empty");
        return QModelIndex();
    }

    int savedSearchNameSize = savedSearchName.size();

    if (savedSearchNameSize < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search name size is below the minimal acceptable length");
        errorDescription.details() = QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN);
        return QModelIndex();
    }

    if (savedSearchNameSize > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search name size is above the maximal acceptable length");
        errorDescription.details() = QString::number(qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX);
        return QModelIndex();
    }

    QModelIndex existingItemIndex = indexForSavedSearchName(savedSearchName);
    if (existingItemIndex.isValid()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search with such name already exists");
        return QModelIndex();
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingSavedSearches = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingSavedSearches + 1 >= m_account.savedSearchCountMax())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't create a new saved search: the account can "
                                                    "contain a limited number of saved searches");
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

    emit aboutToAddSavedSearch();

    int row = static_cast<int>(localUidIndex.size());

    beginInsertRows(QModelIndex(), row, row);
    Q_UNUSED(localUidIndex.insert(item))
    endInsertRows();

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QModelIndex addedSavedSearchIndex = indexForLocalUid(item.m_localUid);
    emit addedSavedSearch(addedSavedSearchIndex);

    return addedSavedSearchIndex;
}

void SavedSearchModel::favoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::favoriteSavedSearch: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row()
            << QStringLiteral(", column = ") << index.column());

    setSavedSearchFavorited(index, true);
}

void SavedSearchModel::unfavoriteSavedSearch(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::unfavoriteSavedSearch: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row()
            << QStringLiteral(", column = ") << index.column());

    setSavedSearchFavorited(index, false);
}

QString SavedSearchModel::localUidForItemName(const QString & itemName) const
{
    QNDEBUG(QStringLiteral("SavedSearchModel::localUidForItemName: ") << itemName);

    QModelIndex index = indexForSavedSearchName(itemName);
    const SavedSearchModelItem * pItem = itemForIndex(index);
    if (!pItem) {
        QNTRACE(QStringLiteral("No saved search with such name was found"));
        return QString();
    }

    return pItem->m_localUid;
}

QString SavedSearchModel::itemNameForLocalUid(const QString & localUid) const
{
    QNDEBUG(QStringLiteral("SavedSearchModel::itemNameForLocalUid: ") << localUid);

    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE(QStringLiteral("No saved search with such local uid"));
        return QString();
    }

    const SavedSearchModelItem & item = *it;
    return item.m_name;
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

QVariant SavedSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex SavedSearchModel::index(int row, int column, const QModelIndex & parent) const
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

bool SavedSearchModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool SavedSearchModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::setData: index: is valid = ")
            << (modelIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << modelIndex.row() << QStringLiteral(", column = ")
            << modelIndex.column() << QStringLiteral(", value = ") << value
            << QStringLiteral(", role = ") << role);

    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        QNDEBUG(QStringLiteral("Bad row"));
        return false;
    }

    int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
        QNDEBUG(QStringLiteral("Bad column"));
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
                QNDEBUG(QStringLiteral("The name has not changed"));
                return true;
            }

            auto nameIt = nameIndex.find(name.toUpper());
            if (nameIt != nameIndex.end()) {
                ErrorString error(QT_TRANSLATE_NOOP("", "Can't rename the saved search: no two saved searches within the account "
                                                    "are allowed to have the same name in case-insensitive manner"));
                error.details() = name;
                QNINFO(error);
                emit notifyError(error);
                return false;
            }

            ErrorString errorDescription;
            if (!SavedSearch::validateName(name, &errorDescription)) {
                ErrorString error(QT_TRANSLATE_NOOP("", "Can't rename the saved search"));
                error.additionalBases().append(errorDescription.base());
                error.additionalBases().append(errorDescription.additionalBases());
                error.details() = errorDescription.details();
                QNINFO(error << QStringLiteral("; suggested new name = ") << name);
                emit notifyError(error);
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
                QNDEBUG(QStringLiteral("Query is empty"));
                return false;
            }

            item.m_isDirty |= (query != item.m_query);
            item.m_query = query;
            break;
        }
    case Columns::Synchronizable:
        {
            if (m_account.type() == Account::Type::Local) {
                ErrorString error(QT_TRANSLATE_NOOP("", "Can't make the saved search synchronizable within the local account"));
                QNINFO(error);
                emit notifyError(error);
                return false;
            }

            if (item.m_isSynchronizable) {
                ErrorString error(QT_TRANSLATE_NOOP("", "Can't make already synchronizable saved search not synchronizable"));
                QNINFO(error << QStringLiteral(", already synchronizable saved search item: ") << item);
                emit notifyError(error);
                return false;
            }

            item.m_isDirty |= (value.toBool() != item.m_isSynchronizable);
            item.m_isSynchronizable = value.toBool();
            break;
        }
    default:
        QNWARNING(QStringLiteral("Unidentified column: ") << modelIndex.column());
        return false;
    }

    index.replace(index.begin() + rowIndex, item);
    emit dataChanged(modelIndex, modelIndex);

    updateRandomAccessIndexWithRespectToSorting(item);

    updateSavedSearchInLocalStorage(item);

    QNDEBUG(QStringLiteral("Successfully set the data"));
    return true;
}

bool SavedSearchModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(QStringLiteral("SavedSearchModel::insertRows: row = ") << row << QStringLiteral(", count = ") << count);

    Q_UNUSED(parent)

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    int numExistingSavedSearches = static_cast<int>(index.size());
    if (Q_UNLIKELY(numExistingSavedSearches + count >= m_account.savedSearchCountMax())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't create a new saved search): the account can contain "
                                            "a limited number of saved searches"));
        error.details() = QString::number(m_account.savedSearchCountMax());
        QNINFO(error);
        emit notifyError(error);
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

    emit layoutAboutToBeChanged();
    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const SavedSearchModelItem & item = *(*it);
        updateRandomAccessIndexWithRespectToSorting(item);
        updateSavedSearchInLocalStorage(item);
    }
    emit layoutChanged();

    return true;
}

bool SavedSearchModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (Q_UNLIKELY(parent.isValid())) {
        QNDEBUG(QStringLiteral("Ignoring the attempt to remove rows from saved search model for valid parent model index"));
        return false;
    }

    if (Q_UNLIKELY((row + count - 1) >= static_cast<int>(m_data.size())))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "Detected attempt to remove more rows than the saved search model contains"));
        QNINFO(error << QStringLiteral(", row = ") << row << QStringLiteral(", count = ") << count
               << QStringLiteral(", number of saved search model items = ") << m_data.size());
        emit notifyError(error);
        return false;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row + i;
        if (it->m_isSynchronizable) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't remove the synchronizable saved search"));
            QNINFO(error << QStringLiteral(", synchronizable note item: ") << *it);
            emit notifyError(error);
            return false;
        }
    }

    emit aboutToRemoveSavedSearches();

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row + i;

        SavedSearch savedSearch;
        savedSearch.setLocalUid(it->m_localUid);

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(requestId))
        QNTRACE(QStringLiteral("Emitting the request to expunge the saved search from the local storage: request id = ")
                << requestId << QStringLiteral(", saved search local uid: ") << it->m_localUid);
        emit expungeSavedSearch(savedSearch, requestId);
    }
    Q_UNUSED(index.erase(index.begin() + row, index.begin() + row + count))
    endRemoveRows();

    emit removedSavedSearches();

    return true;
}

void SavedSearchModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::sort: column = ") << column << QStringLiteral(", order = ") << order
            << QStringLiteral(" (") << (order == Qt::AscendingOrder ? QStringLiteral("ascending") : QStringLiteral("descending"))
            << QStringLiteral(")"));

    if (column != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(QStringLiteral("The sort order already established, nothing to do"));
        return;
    }

    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    QModelIndexList persistentIndices = persistentIndexList();
    QStringList localUidsToUpdate;
    for(auto it = persistentIndices.begin(), end = persistentIndices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        if (!index.isValid() || (index.column() != Columns::Name)) {
            localUidsToUpdate << QString();
            continue;
        }

        const SavedSearchModelItem * item = reinterpret_cast<const SavedSearchModelItem*>(index.internalPointer());
        if (!item) {
            localUidsToUpdate << QString();
            continue;
        }

        localUidsToUpdate << item->m_localUid;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    std::vector<boost::reference_wrapper<const SavedSearchModelItem> > items(index.begin(), index.end());

    if (m_sortOrder == Qt::AscendingOrder) {
        std::sort(items.begin(), items.end(), LessByName());
    }
    else {
        std::sort(items.begin(), items.end(), GreaterByName());
    }

    index.rearrange(items.begin());

    QModelIndexList replacementIndices;
    for(auto it = localUidsToUpdate.begin(), end = localUidsToUpdate.end(); it != end; ++it)
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

    emit layoutChanged();
}

void SavedSearchModel::onAddSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::onAddSavedSearchComplete: ") << search << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it != m_addSavedSearchRequestIds.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.erase(it));
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onAddSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it == m_addSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onAddSavedSearchFailed: search = ") << search << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_addSavedSearchRequestIds.erase(it))

    emit notifyError(errorDescription);

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Can't find the saved search item failed to be added to the local storage within the model's items"));
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(QStringLiteral("Can't find the indexed reference to the saved search item failed to be added to the local storage: ")
                  << search);
        return;
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();
}

void SavedSearchModel::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::onUpdateSavedSearchComplete: ") << search << QStringLiteral("\nRequest id = ")
            << requestId);

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onUpdateSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onUpdateSavedSearchFailed: search = ") << search << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to find the saved search: local uid = ") << search.localUid()
            << QStringLiteral(", request id = ") << requestId);
    emit findSavedSearch(search, requestId);
}

void SavedSearchModel::onFindSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    auto restoreUpdateIt = m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) )
    {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onFindSavedSearchComplete: search = ") << search
            << QStringLiteral("\nRequest id = ") << requestId);

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

void SavedSearchModel::onFindSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findSavedSearchToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findSavedSearchToPerformUpdateRequestIds.find(requestId);

    if ( (restoreUpdateIt == m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) &&
         (performUpdateIt == m_findSavedSearchToPerformUpdateRequestIds.end()) )
    {
        return;
    }

    QNWARNING(QStringLiteral("SavedSearchModel::onFindSavedSearchFailed: search = ") << search << QStringLiteral("\nError description = ")
              << errorDescription << QStringLiteral(", request id = ") << requestId);

    if (restoreUpdateIt != m_findSavedSearchToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findSavedSearchToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.erase(performUpdateIt))
    }

    emit notifyError(errorDescription);
}

void SavedSearchModel::onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                                   size_t limit, size_t offset,
                                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                                   LocalStorageManager::OrderDirection::type orderDirection,
                                                   QList<SavedSearch> foundSearches, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onListSavedSearchesComplete: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", num found searches = ") << foundSearches.size() << QStringLiteral(", request id = ")
            << requestId);

    for(auto it = foundSearches.constBegin(), end = foundSearches.constEnd(); it != end; ++it) {
        onSavedSearchAddedOrUpdated(*it);
    }

    m_listSavedSearchesRequestId = QUuid();
    if (foundSearches.size() == static_cast<int>(limit)) {
        QNTRACE(QStringLiteral("The number of found saved searches matches the limit, requesting more saved searches from the local storage"));
        m_listSavedSearchesOffset += limit;
        requestSavedSearchesList();
        return;
    }

    m_allSavedSearchesListed = true;
    emit notifyAllSavedSearchesListed();
    emit notifyAllItemsListed();
}

void SavedSearchModel::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onListSavedSearchesFailed: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", error: ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listSavedSearchesRequestId = QUuid();

    emit notifyError(errorDescription);
}

void SavedSearchModel::onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::onExpungeSavedSearchComplete: search = ") << search << QStringLiteral("\nRequest id = ")
            << requestId);

    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it != m_expungeSavedSearchRequestIds.end()) {
        Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
        return;
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator itemIt = localUidIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Expunged saved search was not found within the saved search model items: ") << search);
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: can't project the local uid index iterator "
                                            "to the random access index iterator within the saved searches model"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit aboutToRemoveSavedSearches();

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();

    emit removedSavedSearches();
}

void SavedSearchModel::onExpungeSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it == m_expungeSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("SavedSearchModel::onExpungeSavedSearchFailed: search = ") << search << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::createConnections"));

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(SavedSearchModel,addSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,updateSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,findSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,listSavedSearches,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                    LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListSavedSearchesRequest,
                                                              LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,expungeSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeSavedSearch,SavedSearch,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onAddSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchModel,onAddSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onUpdateSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchModel,onUpdateSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onFindSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchModel,onFindSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type, LocalStorageManager::OrderDirection::type,
                                  QList<SavedSearch>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  ErrorString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onExpungeSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchModel,onExpungeSavedSearchFailed,SavedSearch,ErrorString,QUuid));
}

void SavedSearchModel::requestSavedSearchesList()
{
    QNDEBUG(QStringLiteral("SavedSearchModel::requestSavedSearchesList: offset = ") << m_listSavedSearchesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListSavedSearchesOrder::type order = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to list saved searches: offset = ") << m_listSavedSearchesOffset
            << QStringLiteral(", request id = ") << m_listSavedSearchesRequestId);
    emit listSavedSearches(flags, SAVED_SEARCH_LIST_LIMIT, m_listSavedSearchesOffset,
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
        emit aboutToAddSavedSearch();

        int row = rowForNewItem(item);
        beginInsertRows(QModelIndex(), row, row);
        auto insertionResult = localUidIndex.insert(item);
        itemIt = insertionResult.first;
        endInsertRows();

        updateRandomAccessIndexWithRespectToSorting(*itemIt);

        QModelIndex addedSavedSearchIndex = indexForLocalUid(search.localUid());
        emit addedSavedSearch(addedSavedSearchIndex);

        return;
    }

    QModelIndex savedSearchIndexBefore = indexForLocalUid(search.localUid());
    emit aboutToUpdateSavedSearch(savedSearchIndexBefore);

    localUidIndex.replace(itemIt, item);

    auto indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == rowIndex.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: can't project the local uid index iterator "
                                            "to the random access index iterator within the favorites model"));
        QNWARNING(error);
        emit notifyError(error);
        emit updatedSavedSearch(QModelIndex());
        return;
    }

    qint64 position = std::distance(rowIndex.begin(), indexIt);
    if (Q_UNLIKELY(position > static_cast<qint64>(std::numeric_limits<int>::max()))) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Too many stored elements to handle for saved searches model"));
        QNWARNING(error);
        emit notifyError(error);
        emit updatedSavedSearch(QModelIndex());
        return;
    }

    QModelIndex modelIndexFrom = createIndex(static_cast<int>(position), 0);
    QModelIndex modelIndexTo = createIndex(static_cast<int>(position), NUM_SAVED_SEARCH_MODEL_COLUMNS - 1);
    emit dataChanged(modelIndexFrom, modelIndexTo);

    updateRandomAccessIndexWithRespectToSorting(item);

    QModelIndex savedSearchIndexAfter = indexForLocalUid(search.localUid());
    emit updatedSavedSearch(savedSearchIndexAfter);
}

QVariant SavedSearchModel::dataImpl(const int row, const Columns::type column) const
{
    QNTRACE(QStringLiteral("SavedSearchModel::dataImpl: row = ") << row << QStringLiteral(", column = ") << column);

    if (Q_UNLIKELY((row < 0) || (row > static_cast<int>(m_data.size())))) {
        QNTRACE(QStringLiteral("Invalid row ") << row << QStringLiteral(", data size is ") << m_data.size());
        return QVariant();
    }

    const SavedSearchModelItem & item = m_data.get<ByIndex>()[static_cast<size_t>(row)];
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

QVariant SavedSearchModel::dataAccessibleText(const int row, const Columns::type column) const
{
    QNTRACE(QStringLiteral("SavedSearchModel::dataAccessibleText: row = ") << row << QStringLiteral(", column = ") << column);

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
        accessibleText += (textData.toBool() ? tr("synchronizable") : tr("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (textData.toBool() ? tr("changed") : tr("not changed"));
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
    return newItemName<SavedSearchDataByNameUpper>(nameIndex, m_lastNewSavedSearchNameCounter, baseName);
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
        it = std::lower_bound(nameIndex.begin(), nameIndex.end(), newItem, LessByName());
    }
    else {
        it = std::lower_bound(nameIndex.begin(), nameIndex.end(), newItem, GreaterByName());
    }

    if (it == nameIndex.end()) {
        return static_cast<int>(m_data.size());
    }

    int row = static_cast<int>(std::distance(nameIndex.begin(), it));
    return row;
}

void SavedSearchModel::updateRandomAccessIndexWithRespectToSorting(const SavedSearchModelItem & item)
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(item.m_localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNWARNING(QStringLiteral("Can't find saved search item by local uid: ") << item);
        return;
    }

    const SavedSearchDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto appropriateNameIt = nameIndex.end();
    if (m_sortOrder == Qt::AscendingOrder) {
        appropriateNameIt = std::lower_bound(nameIndex.begin(), nameIndex.end(), item, LessByName());
    }
    else {
        appropriateNameIt = std::lower_bound(nameIndex.begin(), nameIndex.end(), item, GreaterByName());
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();

    SavedSearchDataByIndex::iterator originalRandomAccessIt = m_data.project<ByIndex>(itemIt);
    SavedSearchDataByIndex::iterator newRandomAccessIt = m_data.project<ByIndex>(appropriateNameIt);
    int oldRow = static_cast<int>(std::distance(index.begin(), originalRandomAccessIt));
    int newRow = static_cast<int>(std::distance(index.begin(), newRandomAccessIt));

    if (oldRow == newRow) {
        QNDEBUG(QStringLiteral("Already at the appropriate row"));
        return;
    }
    else if (oldRow + 1 == newRow) {
        QNDEBUG(QStringLiteral("Already before the appropriate row"));
        return;
    }

    bool res = beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow);
    if (!res) {
        QNWARNING(QStringLiteral("Internal error, can't move row within the saved search model for sorting purposes"));
        return;
    }

    index.relocate(newRandomAccessIt, originalRandomAccessIt);
    endMoveRows();
}

void SavedSearchModel::updateSavedSearchInLocalStorage(const SavedSearchModelItem & item)
{
    QNDEBUG(QStringLiteral("SavedSearchModel::updateSavedSearchInLocalStorage: local uid = ") << item);

    SavedSearch savedSearch;

    auto notYetSavedItemIt = m_savedSearchItemsNotYetInLocalStorageUids.find(item.m_localUid);
    if (notYetSavedItemIt == m_savedSearchItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG(QStringLiteral("Updating the saved search"));

        const SavedSearch * pCachedSearch = m_cache.get(item.m_localUid);
        if (Q_UNLIKELY(!pCachedSearch))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findSavedSearchToPerformUpdateRequestIds.insert(requestId))
                SavedSearch dummy;
            dummy.setLocalUid(item.m_localUid);
            emit findSavedSearch(dummy, requestId);
            QNDEBUG(QStringLiteral("Emitted the request to find the saved search: local uid = ") << item.m_localUid
                    << QStringLiteral(", request id = ") << requestId);
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
        emit addSavedSearch(savedSearch, requestId);

        QNTRACE(QStringLiteral("Emitted the request to add the saved search to local storage: id = ") << requestId
                << QStringLiteral(", saved search: ") << savedSearch);

        Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId));
        emit updateSavedSearch(savedSearch, requestId);

        QNTRACE(QStringLiteral("Emitted the request to update the saved search in the local storage: id = ") << requestId
                << QStringLiteral(", saved search: ") << savedSearch);
    }
}

void SavedSearchModel::setSavedSearchFavorited(const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR("Can't set favorited flag for the saved search: model index is invalid");
        return;
    }

    const SavedSearchModelItem * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        REPORT_ERROR("Can't set favorited flag for the saved search: can't find the model item corresponding to index");
        return;
    }

    if (favorited == pItem->m_isFavorited) {
        QNDEBUG(QStringLiteral("Favorited flag's value hasn't changed"));
        return;
    }

    SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(pItem->m_localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR("Can't set favorited flag for the saved search: the modified saved search entry was not found within the model");
        return;
    }

    SavedSearchModelItem itemCopy(*pItem);
    itemCopy.m_isFavorited = favorited;

    localUidIndex.replace(it, itemCopy);

    updateSavedSearchInLocalStorage(itemCopy);
}

QModelIndex SavedSearchModel::indexForLocalUidIndexIterator(const SavedSearchDataByLocalUid::const_iterator it) const
{
    const SavedSearchDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    if (it == localUidIndex.end()) {
        return QModelIndex();
    }

    const SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    auto indexIt = m_data.project<ByIndex>(it);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING(QStringLiteral("Can't find the indexed reference to the saved search item: ")
                  << *it);
        return QModelIndex();
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    return createIndex(rowIndex, Columns::Name);
}

bool SavedSearchModel::LessByName::operator()(const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

bool SavedSearchModel::GreaterByName::operator()(const SavedSearchModelItem & lhs, const SavedSearchModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

} // namespace quentier
