#include "SavedSearchModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

// Limit for the queries to the local storage
#define SAVED_SEARCH_LIST_LIMIT (100)

// Limit for the local cache of full saved search objects size
#define MAX_SAVED_SEARCH_CACHE_SIZE (20)

namespace qute_note {

SavedSearchModel::SavedSearchModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                   QObject * parent) :
    QAbstractItemModel(parent),
    m_container(),
    m_cache(),
    m_currentOffset(0),
    m_lastListSavedSearchesRequestId()
{
    createConnections(localStorageManagerThreadWorker);
}

SavedSearchModel::~SavedSearchModel()
{}

Qt::ItemFlags SavedSearchModel::flags(const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(index)
    return QAbstractItemModel::flags(index);
}

QVariant SavedSearchModel::data(const QModelIndex & index, int role) const
{
    // TODO: implement
    Q_UNUSED(index)
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

void SavedSearchModel::onGetSavedSearchCountComplete(int savedSearchCount, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(savedSearchCount)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onGetSavedSearchCountFailed(QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
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
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(foundSearches)
    Q_UNUSED(requestId)
}

void SavedSearchModel::onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListSavedSearchesOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
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

    // TODO: set up signal-slot connections with local storage manager thread worker
    Q_UNUSED(localStorageManagerThreadWorker)
}

QTextStream & SavedSearchModel::SavedSearchData::Print(QTextStream & strm) const
{
    strm << "Saved search data: local uid = " << m_localUid
         << ", name = " << m_name << ", query = " << m_query << "\n";
    return strm;
}

} // namespace qute_note
