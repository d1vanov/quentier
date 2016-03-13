#include "TagModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <limits>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT (100)

#define NUM_TAG_MODEL_COLUMNS (2)

namespace qute_note {

TagModel::TagModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                   QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_tagItemsNotYetInLocalStorageUids(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_expungeTagRequestIds(),
    m_findTagRequestIds()
{
    createConnections(localStorageManagerThreadWorker);
    requestTagsList();
}

TagModel::~TagModel()
{}

Qt::ItemFlags TagModel::flags(const QModelIndex & index) const
{
    // TODO: implement
    return QAbstractItemModel::flags(index);
}

QVariant TagModel::data(const QModelIndex & index, int role) const
{
    // TODO: implement
    Q_UNUSED(index)
    Q_UNUSED(role)
    return QVariant();
}

QVariant TagModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // TODO: implement
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

int TagModel::rowCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

int TagModel::columnCount(const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(parent)
    return 0;
}

QModelIndex TagModel::index(int row, int column, const QModelIndex & parent) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)
    return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex & index) const
{
    // TODO: implement
    Q_UNUSED(index)
    return QModelIndex();
}

bool TagModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool TagModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    // TODO: implement
    Q_UNUSED(modelIndex)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(count)
    Q_UNUSED(parent)
    return false;
}

void TagModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void TagModel::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void TagModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void TagModel::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void TagModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void TagModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void TagModel::onListTagsComplete(LocalStorageManager::ListObjectsOptions flag,
                                  size_t limit, size_t offset,
                                  LocalStorageManager::ListTagsOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QList<Tag> foundTags, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(foundTags)
    Q_UNUSED(requestId)
}

void TagModel::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListTagsOrder::type order,
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

void TagModel::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void TagModel::onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void TagModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    // TODO: implement
    Q_UNUSED(localStorageManagerThreadWorker)
}

void TagModel::requestTagsList()
{
    // TODO: implement
}

void TagModel::onTagAddedOrUpdated(const Tag & tag, bool * pAdded)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(pAdded)
}

QVariant TagModel::dataText(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

QVariant TagModel::dataAccessibleText(const int row, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(row)
    Q_UNUSED(column)
    return QVariant();
}

} // namespace qute_note
