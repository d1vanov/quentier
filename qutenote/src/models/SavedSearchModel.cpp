#include "SavedSearchModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <limits>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

#define NUM_SAVED_SEARCH_MODEL_COLUMNS (4)

namespace qute_note {

SavedSearchModel::SavedSearchModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                   QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_listSavedSearchesOffset(0),
    m_listSavedSearchesRequestId(),
    m_savedSearchItemsNotYetInLocalStorageUids(),
    m_addSavedSearchRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_expungeSavedSearchRequestIds(),
    m_findSavedSearchRequestIds()
{
    createConnections(localStorageManagerThreadWorker);
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel()
{}

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
        QVariant synchronizable = dataText(index.row(), Columns::Synchronizable);
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
        column = Columns::Name;
        break;
    case Columns::Query:
        column = Columns::Query;
        break;
    case Columns::Synchronizable:
        column = Columns::Synchronizable;
        break;
    case Columns::Dirty:
        column = Columns::Dirty;
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataText(rowIndex, column);
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
        return QVariant(QT_TR_NOOP("Name"));
    case Columns::Query:
        return QVariant(QT_TR_NOOP("Query"));
    case Columns::Synchronizable:
        return QVariant(QT_TR_NOOP("Synchronizable"));
    case Columns::Dirty:
        return QVariant(QT_TR_NOOP("Dirty"));
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

    if ((row < 0) || (column < 0)) {
        return QModelIndex();
    }

    if ((row >= static_cast<int>(m_data.size())) ||
        (column >= NUM_SAVED_SEARCH_MODEL_COLUMNS))
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
    if (!modelIndex.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int rowIndex = modelIndex.row();
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_data.size()))) {
        return false;
    }

    int columnIndex = modelIndex.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_SAVED_SEARCH_MODEL_COLUMNS)) {
        return false;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchModelItem item = index[static_cast<size_t>(rowIndex)];

    switch(columnIndex)
    {
    case Columns::Name:
        {
            QString name = value.toString();
            if (name.isEmpty()) {
                return false;
            }

            item.m_isDirty |= (name != item.m_name);
            item.m_name = name;
            break;
        }
    case Columns::Query:
        {
            QString query = value.toString();
            if (query.isEmpty()) {
                return false;
            }

            item.m_isDirty |= (query != item.m_query);
            item.m_query = query;
            break;
        }
    case Columns::Synchronizable:
        {
            if (item.m_isSynchronizable) {
                emit notifyError(QT_TR_NOOP("Can't make already synchronizable saved search not synchronizable"));
                return false;
            }

            item.m_isDirty |= (value.toBool() != item.m_isSynchronizable);
            item.m_isSynchronizable = value.toBool();
            break;
        }
    case Columns::Dirty:
        {
            emit notifyError(QT_TR_NOOP("The \"dirty\" flag can't be set manually"));
            return false;
        }
    default:
        return false;
    }

    index.replace(index.begin() + rowIndex, item);

    emit dataChanged(modelIndex, modelIndex);

    SavedSearch savedSearch;
    savedSearch.setLocalUid(item.m_localUid);
    savedSearch.setName(item.m_name);
    savedSearch.setQuery(item.m_query);
    savedSearch.setLocal(!item.m_isSynchronizable);
    savedSearch.setDirty(item.m_isDirty);

    QUuid requestId = QUuid::createUuid();

    auto notYetSavedItemIt = m_savedSearchItemsNotYetInLocalStorageUids.find(item.m_localUid);
    if (notYetSavedItemIt != m_savedSearchItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addSavedSearchRequestIds.insert(requestId));
        emit addSavedSearch(savedSearch, requestId);

        QNTRACE("Emitted the request to add the saved search to local storage: id = " << requestId
                << ", saved search: " << savedSearch);

        Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateSavedSearchRequestIds.insert(requestId));
        emit updateSavedSearch(savedSearch, requestId);

        QNTRACE("Emitted the request to update the saved search in the local storage: id = " << requestId
                << ", saved search: " << savedSearch);
    }

    return true;
}

bool SavedSearchModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE("SavedSearchModel::insertRows: row = " << row << ", count = " << count);

    Q_UNUSED(parent)
    beginInsertRows(QModelIndex(), row, row + count - 1);

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    for(int i = 0; i < count; ++i)
    {
        SavedSearchModelItem item;
        item.m_localUid = QUuid::createUuid().toString();
        item.m_localUid.chop(1);
        item.m_localUid.remove(0, 1);
        Q_UNUSED(m_savedSearchItemsNotYetInLocalStorageUids.insert(item.m_localUid));

        item.m_name = tr("New saved search");

        Q_UNUSED(index.insert(index.begin() + row, item));
    }

    endInsertRows();
    return false;
}

bool SavedSearchModel::removeRows(int row, int count, const QModelIndex & parent)
{
    Q_UNUSED(parent)
    SavedSearchDataByIndex & index = m_data.get<ByIndex>();

    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row;
        if (it->m_isSynchronizable) {
            QString error = QT_TR_NOOP("Can't remove synchronizable saved search");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        SavedSearchDataByIndex::iterator it = index.begin() + row;

        SavedSearch savedSearch;
        savedSearch.setLocalUid(it->m_localUid);

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(requestId))
        emit expungeSavedSearch(savedSearch, requestId);
        QNTRACE("Emitted the request to expunge the saved search from the local storage: request id = "
                << requestId << ", saved search local uid: " << it->m_localUid);

        Q_UNUSED(index.erase(it))
    }
    endRemoveRows();

    return true;
}

void SavedSearchModel::onAddSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onAddSavedSearchComplete: " << search << "\nRequest id = " << requestId);

    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it != m_addSavedSearchRequestIds.end()) {
        Q_UNUSED(m_addSavedSearchRequestIds.erase(it));
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto it = m_addSavedSearchRequestIds.find(requestId);
    if (it == m_addSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onAddSavedSearchFailed: search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_addSavedSearchRequestIds.erase(it))

    emit notifyError(errorDescription);

    SavedSearchDataByLocalUid & orderedIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator itemIt = orderedIndex.find(search.localUid());
    if (Q_UNLIKELY(itemIt == orderedIndex.end())) {
        QNDEBUG("Can't find the saved search item failed to be added to the local storage within the model's items");
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(itemIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QNWARNING("Can't find the indexed reference to the saved search item failed to be added to the local storage: "
                  << search);
        return;
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(index.erase(indexIt))
    endRemoveRows();
}

void SavedSearchModel::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onUpdateSavedSearchComplete: " << search << "\nRequest id = " << requestId);

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it != m_updateSavedSearchRequestIds.end()) {
        Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))
        return;
    }

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onUpdateSavedSearchFailed: search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchRequestIds.insert(requestId))
    emit findSavedSearch(search, requestId);
    QNTRACE("Emitted the request to find the saved search: local uid = " << search.localUid()
            << ", request id = " << requestId);
}

void SavedSearchModel::onFindSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    auto it = m_findSavedSearchRequestIds.find(requestId);
    if (it == m_findSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onFindSavedSearchComplete: search = " << search << "\nRequest id = " << requestId);

    Q_UNUSED(m_findSavedSearchRequestIds.erase(it))

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto it = m_findSavedSearchRequestIds.find(requestId);
    if (it == m_findSavedSearchRequestIds.end()) {
        return;
    }

    QNWARNING("SavedSearchModel::onFindSavedSearchFailed: search = " << search << "\nError description = "
              << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_findSavedSearchRequestIds.erase(it))

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

    QNDEBUG("SavedSearchModel::onListSavedSearchesComplete: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", num found searches = " << foundSearches.size() << ", request id = " << requestId);

    if (foundSearches.isEmpty()) {
        return;
    }

    for(auto it = foundSearches.begin(), end = foundSearches.end(); it != end; ++it) {
        onSavedSearchAddedOrUpdated(*it);
    }

    m_listSavedSearchesRequestId = QUuid();
    if (foundSearches.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found saved searches matches the limit, requesting more saved searches from the local storage");
        m_listSavedSearchesOffset += limit;
        requestSavedSearchesList();
    }
}

void SavedSearchModel::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QString errorDescription, QUuid requestId)
{
    if (requestId != m_listSavedSearchesRequestId) {
        return;
    }

    QNDEBUG("SavedSearchModel::onListSavedSearchesFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = " << orderDirection
            << ", error: " << errorDescription << ", request id = " << requestId);

    m_listSavedSearchesRequestId = QUuid();

    emit notifyError(errorDescription);
}

void SavedSearchModel::onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    QNDEBUG("SavedSearchModel::onExpungeSavedSearchComplete: search = " << search << "\nRequest id = " << requestId);

    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it != m_expungeSavedSearchRequestIds.end()) {
        Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))
        return;
    }

    SavedSearchDataByLocalUid & orderedIndex = m_data.get<ByLocalUid>();
    SavedSearchDataByLocalUid::iterator orderedIt = orderedIndex.find(search.localUid());
    if (Q_UNLIKELY(orderedIt == orderedIndex.end())) {
        QNDEBUG("Expunged saved search was not found within the saved search model items: " << search);
        return;
    }

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByIndex::iterator indexIt = m_data.project<ByIndex>(orderedIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QString error = QT_TR_NOOP("can't convert index by local uid into sequential index in saved searches model");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int rowIndex = static_cast<int>(std::distance(index.begin(), indexIt));
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    Q_UNUSED(m_data.erase(indexIt))
    endRemoveRows();
}

void SavedSearchModel::onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    auto it = m_expungeSavedSearchRequestIds.find(requestId);
    if (it == m_expungeSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SavedSearchModel::onExpungeSavedSearchFailed: search = " << search << "\nError description = "
            << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_expungeSavedSearchRequestIds.erase(it))

    onSavedSearchAddedOrUpdated(search);
}

void SavedSearchModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("SavedSearchModel::createConnections");

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
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onAddSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onUpdateSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onUpdateSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onFindSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onFindSavedSearchFailed,SavedSearch,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type, LocalStorageManager::OrderDirection::type,
                                  QList<SavedSearch>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                                  QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchModel,onExpungeSavedSearchComplete,SavedSearch,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onExpungeSavedSearchFailed,SavedSearch,QString,QUuid));
}

void SavedSearchModel::requestSavedSearchesList()
{
    QNDEBUG("SavedSearchModel::requestSavedSearchesList: offset = " << m_listSavedSearchesOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListSavedSearchesOrder::type order = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listSavedSearchesRequestId = QUuid::createUuid();
    emit listSavedSearches(flags, SAVED_SEARCH_LIST_LIMIT, m_listSavedSearchesOffset,
                           order, direction, m_listSavedSearchesRequestId);
    QNTRACE("Emitted the request to list saved searches: offset = " << m_listSavedSearchesOffset
            << ", request id = " << m_listSavedSearchesRequestId);
}

void SavedSearchModel::onSavedSearchAddedOrUpdated(const SavedSearch & search)
{
    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByLocalUid & orderedIndex = m_data.get<ByLocalUid>();

    SavedSearchModelItem item(search.localUid());

    if (search.hasName()) {
        item.m_name = search.name();
    }

    if (search.hasQuery()) {
        item.m_query = search.query();
    }

    item.m_isSynchronizable = !search.isLocal();
    item.m_isDirty = search.isDirty();

    SavedSearchDataByLocalUid::iterator savedSearchIt = orderedIndex.find(search.localUid());
    bool newSavedSearch = (savedSearchIt == orderedIndex.end());
    if (newSavedSearch) {
        auto insertionResult = orderedIndex.insert(item);
        savedSearchIt = insertionResult.first;
    }
    else {
        orderedIndex.replace(savedSearchIt, item);
    }

    auto indexIt = m_data.project<ByIndex>(savedSearchIt);
    if (Q_UNLIKELY(indexIt == index.end())) {
        QString error = QT_TR_NOOP("can't convert index by local uid into sequential index in saved searches model");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    qint64 position = std::distance(index.begin(), indexIt);
    if (Q_UNLIKELY(position > static_cast<qint64>(std::numeric_limits<int>::max()))) {
        QString error = QT_TR_NOOP("too many stored elements to handle for saved searches model");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (newSavedSearch) {
        beginInsertRows(QModelIndex(), static_cast<int>(position), static_cast<int>(position));
        endInsertRows();
    }
    else {
        QModelIndex modelIndexFrom = createIndex(static_cast<int>(position), 0);
        QModelIndex modelIndexTo = createIndex(static_cast<int>(position), NUM_SAVED_SEARCH_MODEL_COLUMNS - 1);
        emit dataChanged(modelIndexFrom, modelIndexTo);
    }
}

QVariant SavedSearchModel::dataText(const int row, const Columns::type column) const
{
    QNTRACE("SavedSearchModel::dataText: row = " << row << ", column = " << column);

    if (Q_UNLIKELY((row < 0) || (row > static_cast<int>(m_data.size())))) {
        QNTRACE("Invalid row " << row << ", data size is " << m_data.size());
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
    QNTRACE("SavedSearchModel::dataAccessibleText: row = " << row << ", column = " << column);

    QVariant textData = dataText(row, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = QT_TR_NOOP("Saved search: ");

    switch(column)
    {
    case Columns::Name:
        accessibleText += QT_TR_NOOP("name is ") + textData.toString();
        break;
    case Columns::Query:
        accessibleText += QT_TR_NOOP("query is ") + textData.toString();
        break;
    case Columns::Synchronizable:
        accessibleText += (textData.toBool() ? "synchronizable" : "not synchronizable");
        break;
    case Columns::Dirty:
        accessibleText += (textData.toBool() ? "dirty" : "not dirty");
        break;
    default:
        return QVariant();
    }

    return QVariant(accessibleText);
}

} // namespace qute_note
