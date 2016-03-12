#include "SavedSearchModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <limits>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

// Limit for the local cache of full saved search objects size
#define MAX_SAVED_SEARCH_CACHE_SIZE (20)

#define NUM_SAVED_SEARCH_MODEL_COLUMNS (4)

namespace qute_note {

SavedSearchModel::SavedSearchModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                   QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_listSavedSearchesOffset(0),
    m_listSavedSearchesRequestId()
{
    createConnections(localStorageManagerThreadWorker);
    requestSavedSearchesList();
}

SavedSearchModel::~SavedSearchModel()
{}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        indexFlags |= Qt::ItemIsSelectable;
        indexFlags |= Qt::ItemIsEditable;
        indexFlags |= Qt::ItemIsEnabled;
    }

    return indexFlags;
}

QVariant SavedSearchModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() ||
        (index.row() < 0) || (index.row() >= static_cast<int>(m_data.size())) ||
        (index.column() < 0) || (index.column() > NUM_SAVED_SEARCH_MODEL_COLUMNS))
    {
        return QVariant();
    }

    // TODO: implement further
    Q_UNUSED(role)
    return QVariant();
}

QVariant SavedSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // TODO: implement
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

int SavedSearchModel::rowCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

int SavedSearchModel::columnCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

QModelIndex SavedSearchModel::index(int row, int column, const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)
    return QModelIndex();
}

QModelIndex SavedSearchModel::parent(const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(index)
    return QModelIndex();
}

bool SavedSearchModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    // TODO: implement
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool SavedSearchModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool SavedSearchModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool SavedSearchModel::removeRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

void SavedSearchModel::onAddSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
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

    int originalSize = static_cast<int>(m_data.size());
    int numAddedRows = 0;

    SavedSearchDataByIndex & index = m_data.get<ByIndex>();
    SavedSearchDataByLocalUid & orderedIndex = m_data.get<ByLocalUid>();
    for(auto it = foundSearches.begin(), end = foundSearches.end(); it != end; ++it)
    {
        SavedSearchModelItem item(it->localUid());

        if (it->hasName()) {
            item.m_name = it->name();
        }

        if (it->hasQuery()) {
            item.m_query = it->query();
        }

        SavedSearchDataByLocalUid::iterator savedSearchIt = orderedIndex.find(it->localUid());
        bool newSavedSearch = (savedSearchIt == orderedIndex.end());
        if (newSavedSearch)
        {
            auto insertionResult = orderedIndex.insert(item);
            savedSearchIt = insertionResult.first;
            ++numAddedRows;
        }
        else
        {
            orderedIndex.replace(savedSearchIt, item);

            auto indexIt = m_data.project<ByIndex>(savedSearchIt);
            if (Q_UNLIKELY(indexIt == index.end())) {
                QString error = QT_TR_NOOP("can't convert index by local uid into sequential index in saved searches model");
                QNWARNING(error);
                emit notifyError(error);
                continue;
            }

            qint64 position = std::distance(index.begin(), indexIt);
            if (Q_UNLIKELY(position > static_cast<qint64>(std::numeric_limits<int>::max()))) {
                QString error = QT_TR_NOOP("too many stored elements to handle for saved searches model");
                QNWARNING(error);
                emit notifyError(error);
                continue;
            }

            QModelIndex modelIndexFrom = createIndex(static_cast<int>(position), Columns::Name);
            QModelIndex modelIndexTo = createIndex(static_cast<int>(position), Columns::Query);

            emit dataChanged(modelIndexFrom, modelIndexTo);
        }
    }

    if (numAddedRows) {
        emit beginInsertRows(QModelIndex(), originalSize, originalSize + numAddedRows);
        emit endInsertRows();
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
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(search)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void SavedSearchModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("SavedSearchModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(SavedSearchModel,addSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,updateSavedSearch,SavedSearch,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchModel,listSavedSearches,LocalStorageManager::ListObjectsOptions::type,
                                    size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                    LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListSavedSearchesRequest,
                                                              LocalStorageManager::ListObjectsOptions::type,
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
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesComplete,LocalStorageManager::ListObjectsOptions::type,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesComplete,LocalStorageManager::ListObjectsOptions::type,
                                  size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type, LocalStorageManager::OrderDirection::type,
                                  QList<SavedSearch>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listSavedSearchesFailed,
                                                                LocalStorageManager::ListObjectsOptions::type,
                                                                size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid),
                     this, QNSLOT(SavedSearchModel,onListSavedSearchesFailed,LocalStorageManager::ListObjectsOptions::type,
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

} // namespace qute_note
