#include "TagModel.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QByteArray>
#include <QMimeData>
#include <limits>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT (100)

#define NUM_TAG_MODEL_COLUMNS (3)

namespace qute_note {

TagModel::TagModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                   QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_ready(false),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_tagItemsNotYetInLocalStorageUids(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_deleteTagRequestIds(),
    m_expungeTagRequestIds(),
    m_findTagRequestIds(),
    m_lastNewTagNameCounter(0)
{
    createConnections(localStorageManagerThreadWorker);
    requestTagsList();
}

TagModel::~TagModel()
{
    delete m_fakeRootItem;
}

Qt::ItemFlags TagModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (!m_ready) {
        return indexFlags;
    }

    if (index.column() == Columns::Dirty) {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        QModelIndex parentIndex = index;

        while(true) {
            const TagModelItem * item = itemForIndex(parentIndex);
            if (!item) {
                break;
            }

            if (item == m_fakeRootItem) {
                break;
            }

            if (item->isSynchronizable()) {
                return indexFlags;
            }

            parentIndex = parentIndex.parent();
        }
    }

    indexFlags |= Qt::ItemIsEditable;

    return indexFlags;
}

QVariant TagModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (!m_ready) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_TAG_MODEL_COLUMNS)) {
        return QVariant();
    }

    const TagModelItem * item = itemForIndex(index);
    if (!item) {
        return QVariant();
    }

    if (item == m_fakeRootItem) {
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
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataText(*item, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*item, column);
    default:
        return QVariant();
    }
}

QVariant TagModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch(section)
    {
    case Columns::Name:
        return QVariant(QT_TR_NOOP("Name"));
    case Columns::Synchronizable:
        return QVariant(QT_TR_NOOP("Synchronizable"));
    case Columns::Dirty:
        return QVariant(QT_TR_NOOP("Dirty"));
    default:
        return QVariant();
    }
}

int TagModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    if (!m_ready) {
        return 0;
    }

    const TagModelItem * parentItem = itemForIndex(parent);
    return (parentItem ? parentItem->numChildren() : 0);
}

int TagModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    return NUM_TAG_MODEL_COLUMNS;
}

QModelIndex TagModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!m_fakeRootItem || !m_ready || (row < 0) || (column < 0) || (column >= NUM_TAG_MODEL_COLUMNS) ||
        (parent.isValid() && (parent.column() != Columns::Name)))
    {
        return QModelIndex();
    }

    const TagModelItem * parentItem = itemForIndex(parent);
    if (!parentItem) {
        return QModelIndex();
    }

    const TagModelItem * item = parentItem->childAtRow(row);
    if (!item) {
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, column, const_cast<TagModelItem*>(item));
}

QModelIndex TagModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    if (!m_ready) {
        return QModelIndex();
    }

    const TagModelItem * childItem = itemForIndex(index);
    if (!childItem) {
        return QModelIndex();
    }

    const TagModelItem * parentItem = childItem->parent();
    if (!parentItem) {
        return QModelIndex();
    }

    if (parentItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const TagModelItem * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        return QModelIndex();
    }

    int row = grandParentItem->rowForChild(parentItem);
    if (row < 0) {
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<TagModelItem*>(parentItem));
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
    if (!m_ready) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    if (!modelIndex.isValid()) {
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        emit notifyError(QT_TR_NOOP("The \"dirty\" flag can't be set manually"));
        return false;
    }

    const TagModelItem * item = itemForIndex(modelIndex);
    if (!item) {
        return false;
    }

    if (item == m_fakeRootItem) {
        return false;
    }

    TagModelItem itemCopy = *item;
    switch(modelIndex.column())
    {
    case Columns::Name:
        itemCopy.setName(value.toString());
        break;
    case Columns::Synchronizable:
        {
            if (itemCopy.isSynchronizable()) {
                QString error = QT_TR_NOOP("Can't make already synchronizable tag not synchronizable");
                QNINFO(error);
                emit notifyError(error);
                return false;
            }

            bool dirty = itemCopy.isDirty();
            dirty |= (value.toBool() != itemCopy.isSynchronizable());
            itemCopy.setSynchronizable(value.toBool());
            break;
        }
    default:
        return false;
    }

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();
    auto it = index.find(itemCopy.localUid());
    if (Q_UNLIKELY(it == index.end())) {
        QString error = QT_TR_NOOP("Internal error: can't find item in TagModel by its local uid");
        QNWARNING(error << ", item: " << itemCopy);
        emit notifyError(error);
        return false;
    }

    index.replace(it, itemCopy);

    emit dataChanged(modelIndex, modelIndex);

    Tag tag;
    tag.setLocalUid(itemCopy.localUid());
    tag.setName(itemCopy.name());
    tag.setLocal(!itemCopy.isSynchronizable());
    tag.setDirty(itemCopy.isDirty());

    QUuid requestId = QUuid::createUuid();

    auto notYetSavedItemIt = m_tagItemsNotYetInLocalStorageUids.find(itemCopy.localUid());
    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addTagRequestIds.insert(requestId));
        emit addTag(tag, requestId);

        QNTRACE("Emitted the request to add the tag to local storage: id = " << requestId
                << ", tag: " << tag);

        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));
        emit updateTag(tag, requestId);

        QNTRACE("Emitted the request to update the tag in the local storage: id = " << requestId
                << ", tag: " << tag);
    }

    return true;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    if (!m_ready) {
        return false;
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * parentItem = (parent.isValid()
                                       ? itemForIndex(parent)
                                       : m_fakeRootItem);
    if (!parentItem) {
        return false;
    }

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        TagModelItem item;
        QString localUid = QUuid::createUuid().toString();
        localUid.chop(1);
        localUid.remove(0, 1);
        item.setLocalUid(localUid);
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(localUid))

        item.setName(nameForNewTag());
        item.setDirty(true);
        item.setParent(parentItem);

        Q_UNUSED(index.insert(item))
    }
    endInsertRows();

    return true;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (!m_ready) {
        return false;
    }

    if (!m_fakeRootItem) {
        return false;
    }

    const TagModelItem * parentItem = (parent.isValid()
                                       ? itemForIndex(parent)
                                       : m_fakeRootItem);
    if (!parentItem) {
        return false;
    }

    for(int i = 0; i < count; ++i)
    {
        const TagModelItem * item = parentItem->childAtRow(row + i);
        if (!item) {
            continue;
        }

        if (item->isSynchronizable()) {
            QString error = QT_TR_NOOP("Can't remove synchronizable tag");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }

        if (hasSynchronizableChildren(item)) {
            QString error = QT_TR_NOOP("Can't remove tag with synchronizable children");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }
    }

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();

    beginRemoveRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        const TagModelItem * item = parentItem->takeChild(row);
        if (!item) {
            continue;
        }

        Tag tag;
        tag.setLocalUid(item->localUid());

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeTagRequestIds.insert(requestId))
        emit expungeTag(tag, requestId);
        QNTRACE("Emitted the request to expunge the tag from the local storage: request id = "
                << requestId << ", tag local uid: " << item->localUid());

        auto it = index.find(item->localUid());
        Q_UNUSED(index.erase(it))
    }
    endRemoveRows();

    return true;
}

QStringList TagModel::mimeTypes() const
{
    QStringList list;
    list << TAG_MODEL_MIME_TYPE;
    return list;
}

QMimeData * TagModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.count() != 1) {
        return Q_NULLPTR;
    }

    const TagModelItem * item = itemForIndex(indexes.at(0));
    if (!item) {
        return Q_NULLPTR;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *item;

    QMimeData * mimeData = new QMimeData;
    mimeData->setData(TAG_MODEL_MIME_TYPE, qCompress(encodedItem, TAG_MODEL_MIME_DATA_MAX_COMPRESSION));
    return mimeData;
}

bool TagModel::dropMimeData(const QMimeData * mimeData, Qt::DropAction action,
                            int row, int column, const QModelIndex & parent)
{
    QNDEBUG("TagModel::dropMimeData: action = " << action << ", row = " << row
            << ", column = " << column << ", parent: is valid = " << (parent.isValid() ? "true" : "false")
            << ", parent row = " << parent.row() << ", parent column = " << (parent.column())
            << ", mime data formats: " << (mimeData ? mimeData->formats().join("; ") : QString("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!mimeData || !mimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        return false;
    }

    const TagModelItem * item = itemForIndex(parent);
    if (!item) {
        return false;
    }

    QByteArray data = qUncompress(mimeData->data(TAG_MODEL_MIME_TYPE));
    TagModelItem localItem;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> localItem;

    localItem.setParent(item);
    localItem.setParentLocalUid(item->localUid());
    localItem.setParentGuid(item->guid());
    mapChildItems(localItem);

    if (row == -1)
    {
        if (!parent.isValid() && !m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        row = parent.isValid() ? parent.row() : m_fakeRootItem->numChildren();
    }

    beginInsertRows(parent, row, row);
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto originalItemIt = localUidIndex.find(localItem.localUid());
    if (originalItemIt != localUidIndex.end()) {
        localUidIndex.replace(originalItemIt, localItem);
    }
    else {
        Q_UNUSED(localUidIndex.insert(localItem))
    }
    endInsertRows();

    return true;
}

void TagModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("TagModel::onAddTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    auto it = m_addTagRequestIds.find(requestId);
    if (it != m_addTagRequestIds.end()) {
        Q_UNUSED(m_addTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
}

void TagModel::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_addTagRequestIds.find(requestId);
    if (it == m_addTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onAddTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_addTagRequestIds.erase(it))

    emit notifyError(errorDescription);

    removeItemByLocalUid(tag.localUid());
}

void TagModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("TagModel::onUpdateTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    auto it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end()) {
        Q_UNUSED(m_updateTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
}

void TagModel::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onUpdateTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagRequestIds.insert(requestId))
    emit findTag(tag, requestId);
    QNTRACE("Emitted the request to find the tag: local uid = " << tag.localUid()
            << ", request id = " << requestId);
}

void TagModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_findTagRequestIds.find(requestId);
    if (it == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onFindTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    Q_UNUSED(m_findTagRequestIds.erase(it))

    onTagAddedOrUpdated(tag);
}

void TagModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestIds.find(requestId);
    if (it == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onFindTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_findTagRequestIds.erase(it))

    emit notifyError(errorDescription);
}

void TagModel::onListTagsComplete(LocalStorageManager::ListObjectsOptions flag,
                                  size_t limit, size_t offset,
                                  LocalStorageManager::ListTagsOrder::type order,
                                  LocalStorageManager::OrderDirection::type orderDirection,
                                  QString linkedNotebookGuid, QList<Tag> foundTags, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG("TagModel::onListTagsComplete: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = "
            << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", num found tags = " << foundTags.size() << ", request id = " << requestId);

    for(auto it = foundTags.begin(), end = foundTags.end(); it != end; ++it) {
        onTagAddedOrUpdated(*it);
    }

    m_listTagsRequestId = QUuid();

    if (foundTags.size() == static_cast<int>(limit)) {
        QNTRACE("The number of found tags matches the limit, requesting more tags from the local storage");
        m_listTagsOffset += limit;
        requestTagsList();
        return;
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    mapChildItems();

    m_ready = true;
    emit ready();

    int dataSize = static_cast<int>(m_data.size());
    beginInsertRows(QModelIndex(), 0, dataSize - 1);
    endInsertRows();
}

void TagModel::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListTagsOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection,
                                QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG("TagModel::onListTagsFailed: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", direction = "
            << orderDirection << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QString("<null>") : linkedNotebookGuid)
            << ", error description = " << errorDescription << ", request id = " << requestId);

    m_listTagsRequestId = QUuid();

    emit notifyError(errorDescription);
}

void TagModel::onDeleteTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("TagModel::onDeleteTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    auto it = m_deleteTagRequestIds.find(requestId);
    if (it != m_deleteTagRequestIds.end()) {
        Q_UNUSED(m_deleteTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
}

void TagModel::onDeleteTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_deleteTagRequestIds.find(requestId);
    if (it == m_deleteTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onDeleteTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_deleteTagRequestIds.erase(it))

    tag.setDeleted(false);
    onTagAddedOrUpdated(tag);
}

void TagModel::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG("TagModel::onExpungeTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    auto it = m_expungeTagRequestIds.find(requestId);
    if (it != m_expungeTagRequestIds.end()) {
        Q_UNUSED(m_expungeTagRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(tag.localUid());
}

void TagModel::onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_expungeTagRequestIds.find(requestId);
    if (it == m_expungeTagRequestIds.end()) {
        return;
    }

    QNDEBUG("TagModel::onExpungeTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", rwquest id = " << requestId);

    Q_UNUSED(m_expungeTagRequestIds.erase(it))

    onTagAddedOrUpdated(tag);
}

void TagModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("TagModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(TagModel,addTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,updateTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,findTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,listTags,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                    LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListTagsRequest,LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,deleteTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onDeleteTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,expungeTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeTagRequest,Tag,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onAddTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onFindTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QList<Tag>,QUuid),
                     this, QNSLOT(TagModel,onListTagsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Tag>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QUuid),
                     this, QNSLOT(TagModel,onListTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onDeleteTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,deleteTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onDeleteTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagFailed,Tag,QString,QUuid));
}

void TagModel::requestTagsList()
{
    QNDEBUG("TagModel::requestTagsList: offset = " << m_listTagsOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();
    emit listTags(flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, QString(), m_listTagsRequestId);
    QNTRACE("Emitted the request to list tags: offset = " << m_listTagsOffset << ", request id = " << m_listTagsRequestId);
}

void TagModel::onTagAddedOrUpdated(const Tag & tag)
{
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    TagModelItem item(tag.localUid());

    if (tag.hasGuid()) {
        item.setGuid(tag.guid());
    }

    if (tag.hasName()) {
        item.setName(tag.name());
    }

    if (tag.hasParentLocalUid()) {
        item.setParentLocalUid(tag.parentLocalUid());
    }

    if (tag.hasParentGuid()) {
        item.setParentGuid(tag.parentGuid());
    }

    item.setSynchronizable(!tag.isLocal());
    item.setDirty(tag.isDirty());

    auto itemIt = localUidIndex.find(tag.localUid());
    bool newTag = (itemIt == localUidIndex.end());
    if (newTag) {
        auto insertionResult = localUidIndex.insert(item);
        itemIt = insertionResult.first;
        mapChildItems(*itemIt);
    }
    else {
        localUidIndex.replace(itemIt, item);
    }

    if (!m_ready) {
        QNTRACE("The tag model is not ready yet, won't emit data changed or rows insertion notification");
        return;
    }

    const TagModelItem * parentItem = itemIt->parent();
    if (!parentItem) {
        parentItem = m_fakeRootItem;
        itemIt->setParent(parentItem);
    }

    int itemIndex = parentItem->rowForChild(&(*itemIt));
    if (Q_UNLIKELY(itemIndex < 0)) {
        QString error = QT_TR_NOOP("Can't find the row of a child item just added or updated in TagModel");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QModelIndex parentItemModelIndex = indexForItem(parentItem);

    if (newTag) {
        beginInsertRows(parentItemModelIndex, itemIndex, itemIndex);
        endInsertRows();
    }
    else {
        // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
        QModelIndex modelIndexFrom = createIndex(itemIndex, 0, const_cast<TagModelItem*>(&(*itemIt)));
        QModelIndex modelIndexTo = createIndex(itemIndex, NUM_TAG_MODEL_COLUMNS - 1, const_cast<TagModelItem*>(&(*itemIt)));
        emit dataChanged(modelIndexFrom, modelIndexTo);
    }
}

QVariant TagModel::dataText(const TagModelItem & item, const Columns::type column) const
{
    switch(column)
    {
    case Columns::Name:
        return QVariant(item.name());
    case Columns::Synchronizable:
        return QVariant(item.isSynchronizable());
    case Columns::Dirty:
        return QVariant(item.isDirty());
    default:
        return QVariant();
    }
}

QVariant TagModel::dataAccessibleText(const TagModelItem & item, const Columns::type column) const
{
    QVariant textData = dataText(item, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = QT_TR_NOOP("Tag: ");

    switch(column)
    {
    case Columns::Name:
        accessibleText += QT_TR_NOOP("name is ") + textData.toString();
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

const TagModelItem * TagModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_fakeRootItem;
    }

    const TagModelItem * item = reinterpret_cast<const TagModelItem*>(index.internalPointer());
    if (item) {
        return item;
    }

    return m_fakeRootItem;
}

QModelIndex TagModel::indexForItem(const TagModelItem * item) const
{
    if (!item) {
        return QModelIndex();
    }

    const TagModelItem * parentItem = item->parent();
    if (!parentItem) {
        parentItem = m_fakeRootItem;
        item->setParent(parentItem);
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QString error = QT_TR_NOOP("Internal error: can't get the row of the child item in parent in TagModel");
        QNWARNING(error << ", child item: " << *item << "\nParent item: " << *parentItem);
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<TagModelItem*>(item));
}

bool TagModel::hasSynchronizableChildren(const TagModelItem * item) const
{
    if (item->isSynchronizable()) {
        return true;
    }

    QList<const TagModelItem*> children = item->children();
    for(auto it = children.begin(), end = children.end(); it != end; ++it) {
        if (hasSynchronizableChildren(*it)) {
            return true;
        }
    }

    return false;
}

void TagModel::mapChildItems()
{
    QNDEBUG("TagModel::mapChildItems");

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it) {
        const TagModelItem & item = *it;
        mapChildItems(item);
    }
}

void TagModel::mapChildItems(const TagModelItem & item)
{
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const QString & parentLocalUid = item.parentLocalUid();
    if (!parentLocalUid.isEmpty())
    {
        auto parentIt = localUidIndex.find(parentLocalUid);
        if (Q_UNLIKELY(parentIt == localUidIndex.end())) {
            QString error = tr("Can't find parent tag for tag ") + "\"" + item.name() + "\"";
            QNWARNING(error);
            emit notifyError(error);
        }
        else {
            const TagModelItem & parentItem = *parentIt;
            parentItem.addChild(&item);
        }
    }
}

QString TagModel::nameForNewTag() const
{
    QString baseName = QT_TR_NOOP("New tag");
    if (m_lastNewTagNameCounter != 0) {
        baseName += " (" + QString::number(m_lastNewTagNameCounter) + ")";
    }

    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    while(true)
    {
        auto it = nameIndex.find(baseName.toUpper());
        if (it == nameIndex.end()) {
            return baseName;
        }

        ++m_lastNewTagNameCounter;
        baseName += " (" + QString::number(m_lastNewTagNameCounter) + ")";
    }
}

void TagModel::removeItemByLocalUid(const QString & localUid)
{
    QNDEBUG("TagModel::removeItemByLocalUid: " << localUid);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("Can't find item to remove from the tag model");
        return;
    }

    const TagModelItem & item = *itemIt;

    const TagModelItem * parentItem = item.parent();
    if (!parentItem) {
        parentItem = m_fakeRootItem;
        item.setParent(parentItem);
    }

    int row = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(row < 0)) {
        QString error = QT_TR_NOOP("Internal error: can't get the row of the child item in parent in TagModel");
        QNWARNING(error << ", child item: " << item << "\nParent item: " << *parentItem);
        return;
    }

    QModelIndex parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

} // namespace qute_note
