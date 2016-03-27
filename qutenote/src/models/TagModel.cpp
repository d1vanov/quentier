#include "TagModel.h"
#include "NewItemNameGenerator.hpp"
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

    bool shouldMakeParentsSynchronizable = false;

    TagModelItem itemCopy = *item;
    bool dirty = itemCopy.isDirty();
    switch(modelIndex.column())
    {
    case Columns::Name:
        {
            dirty |= (value.toString() != itemCopy.name());
            itemCopy.setName(value.toString());
            break;
        }
    case Columns::Synchronizable:
        {
            if (itemCopy.isSynchronizable() && !value.toBool()) {
                QString error = QT_TR_NOOP("Can't make already synchronizable tag not synchronizable");
                QNINFO(error);
                emit notifyError(error);
                return false;
            }

            dirty |= (value.toBool() != itemCopy.isSynchronizable());
            itemCopy.setSynchronizable(value.toBool());
            shouldMakeParentsSynchronizable = true;
            break;
        }
    default:
        return false;
    }

    itemCopy.setDirty(dirty);

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();

    if (shouldMakeParentsSynchronizable)
    {
        const TagModelItem * processedItem = &itemCopy;
        TagModelItem dummy;
        while(processedItem->parent())
        {
            const TagModelItem * parentItem = processedItem->parent();
            if (parentItem == m_fakeRootItem) {
                break;
            }

            if (parentItem->isSynchronizable()) {
                break;
            }

            dummy = *parentItem;
            dummy.setSynchronizable(true);
            auto dummyIt = index.find(dummy.localUid());
            if (Q_UNLIKELY(dummyIt == index.end())) {
                QString error = QT_TR_NOOP("Internal error: can't find one of currently made synchronizable item's parents in TagModel by its local uid");
                QNWARNING(error << ", item: " << dummy);
                emit notifyError(error);
                return false;
            }

            index.replace(dummyIt, dummy);
            QModelIndex changedIndex = indexForLocalUid(dummy.localUid());
            if (Q_UNLIKELY(!changedIndex.isValid())) {
                QString error = QT_TR_NOOP("Can't get the valid tag item model index for one of currently made synchronizable item's parents in TagModel by its local uid");
                QNWARNING(error << ", item for which the index was requested: " << dummy);
                emit notifyError(error);
                return false;
            }

            emit dataChanged(changedIndex, changedIndex);
            processedItem = parentItem;
        }
    }

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
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

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
                            int row, int column, const QModelIndex & parentIndex)
{
    QNDEBUG("TagModel::dropMimeData: action = " << action << ", row = " << row
            << ", column = " << column << ", parent index: is valid = " << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row() << ", parent column = " << (parentIndex.column())
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

    const TagModelItem * parentItem = itemForIndex(parentIndex);
    if (!parentItem) {
        return false;
    }

    QByteArray data = qUncompress(mimeData->data(TAG_MODEL_MIME_TYPE));
    TagModelItem item;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> item;

    item.setParent(parentItem);
    item.setParentLocalUid(parentItem->localUid());
    item.setParentGuid(parentItem->guid());

    if (row == -1)
    {
        if (!parentIndex.isValid() && !m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        row = parentIndex.isValid() ? parentIndex.row() : m_fakeRootItem->numChildren();
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto originalItemIt = localUidIndex.find(item.localUid());
    if (originalItemIt != localUidIndex.end())
    {
        // Need to manually remove the item from its original parent

        const TagModelItem & originalItem = *originalItemIt;
        const TagModelItem * originalItemParent = originalItem.parent();
        if (!originalItemParent)
        {
            if (!m_fakeRootItem) {
                m_fakeRootItem = new TagModelItem;
            }

            originalItemParent = m_fakeRootItem;
            originalItem.setParent(originalItemParent);
        }

        Q_UNUSED(originalItemParent->takeChild(row));
    }

    beginInsertRows(parentIndex, row, row);
    if (originalItemIt != localUidIndex.end()) {
        localUidIndex.replace(originalItemIt, item);
        mapChildItems(*originalItemIt);
    }
    else {
        auto insertionResult = localUidIndex.insert(item);
        mapChildItems(*(insertionResult.first));
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
                                                                QString,QString,QUuid),
                     this, QNSLOT(TagModel,onListTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
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

    auto itemIt = localUidIndex.find(tag.localUid());
    bool newTag = (itemIt == localUidIndex.end());
    if (newTag)
    {
        onTagAdded(tag);
    }
    else {
        onTagUpdated(tag, itemIt);
    }
}

void TagModel::onTagAdded(const Tag & tag)
{
    QNDEBUG("TagModel::onTagAdded: tag local uid = " << tag.localUid());

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const TagModelItem * parentItem = Q_NULLPTR;

    if (tag.hasParentLocalUid())
    {
        auto parentIt = localUidIndex.find(tag.parentLocalUid());
        if (parentIt != localUidIndex.end()) {
            parentItem = &(*parentIt);
        }
        else {
            if (!m_fakeRootItem) {
                m_fakeRootItem = new TagModelItem;
            }

            parentItem = m_fakeRootItem;
        }
    }
    else
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        parentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(parentItem);

    TagModelItem item;
    tagToItem(tag, item);

    // The new item is going to be inserted into the last row of the parent item
    int row = parentItem->numChildren();

    beginInsertRows(parentIndex, row, row);

    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    it->setParent(parentItem);
    mapChildItems(*it);

    endInsertRows();
}

void TagModel::onTagUpdated(const Tag & tag, TagDataByLocalUid::iterator it)
{
    QNDEBUG("TagModel::onTagUpdated: tag local uid = " << tag.localUid());

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    TagModelItem itemCopy;
    tagToItem(tag, itemCopy);

    localUidIndex.replace(it, itemCopy);

    const TagModelItem * item = &(*it);

    const TagModelItem * parentItem = item->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QString error = QT_TR_NOOP("Tag item being updated does not have a parent item linked with it");
        QNWARNING(error << ", tag: " << tag << "\nTag item: " << *item);
        emit notifyError(error);
        return;
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QString error = QT_TR_NOOP("Can't find the row of tag item being updated within its parent");
        QNWARNING(error << ", tag: " << tag << "\nTag item: " << *item);
        emit notifyError(error);
        return;
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    QModelIndex modelIndexFrom = createIndex(row, 0, const_cast<TagModelItem*>(item));
    QModelIndex modelIndexTo = createIndex(row, NUM_TAG_MODEL_COLUMNS - 1, const_cast<TagModelItem*>(item));
    emit dataChanged(modelIndexFrom, modelIndexTo);
}

void TagModel::tagToItem(const Tag & tag, TagModelItem & item)
{
    item.setLocalUid(tag.localUid());

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

    QNTRACE("Created tag model item from tag; item: " << item << "\nTag: " << tag);
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

    if (item == m_fakeRootItem) {
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

QModelIndex TagModel::indexForLocalUid(const QString & localUid) const
{
    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(localUid);
    if (it == localUidIndex.end()) {
        return QModelIndex();
    }

    const TagModelItem & item = *it;
    return indexForItem(&item);
}

QModelIndex TagModel::promote(const QModelIndex & itemIndex)
{
    QNDEBUG("TagModel::promote: index: is valid = " << (itemIndex.isValid() ? "true" : "false")
            << ", row = " << itemIndex.row() << ", column = " << itemIndex.column());

    if (!itemIndex.isValid()) {
        return itemIndex;
    }

    const TagModelItem * item = itemForIndex(itemIndex);
    if (!item) {
        QNDEBUG("No item for given index");
        return itemIndex;
    }

    if (item == m_fakeRootItem) {
        QNDEBUG("Fake root item is not a real item");
        return itemIndex;
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * parentItem = item->parent();
    if (!parentItem) {
        parentItem = m_fakeRootItem;
        item->setParent(parentItem);
    }

    if (parentItem == m_fakeRootItem) {
        QNDEBUG("Already a top level item");
        return itemIndex;
    }

    int row = parentItem->rowForChild(item);
    if (row < 0) {
        QNDEBUG("Can't find row of promoted item within its parent item");
        return itemIndex;
    }

    const TagModelItem * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        grandParentItem = m_fakeRootItem;
        parentItem->setParent(grandParentItem);
    }

    int parentRow = grandParentItem->rowForChild(parentItem);
    if (parentRow < 0) {
        QNWARNING("Can't find parent item's row within its own parent item");
        return itemIndex;
    }

    QModelIndex grandParentIndex = indexForItem(grandParentItem);
    if (Q_UNLIKELY(!grandParentIndex.isValid())) {
        QNWARNING("Can't get the valid index for the grand parent item of the promoted item in tag model");
        return itemIndex;
    }

    const TagModelItem * takenItem = parentItem->takeChild(row);
    if (Q_UNLIKELY(takenItem != item)) {
        QNWARNING("Internal inconsistency in tag model: item to take out from parent doesn't match the original promoted item");
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    grandParentItem->insertChild(parentRow + 1, takenItem);

    QModelIndex newIndex = index(parentRow + 1, Columns::Name, grandParentIndex);
    if (!newIndex.isValid()) {
        QNWARNING("Can't get the valid index for the promoted item in tag model");
        Q_UNUSED(grandParentItem->takeChild(parentRow + 1))
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    TagModelItem copyItem = *takenItem;
    copyItem.setParentLocalUid(grandParentItem->localUid());
    copyItem.setParentGuid(grandParentItem->guid());

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(copyItem.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO("The promoted tag model item was not found in the underlying item which is odd. Adding it there");
        Q_UNUSED(localUidIndex.insert(copyItem))
    }
    else {
        localUidIndex.replace(it, copyItem);
    }

    emit dataChanged(newIndex, newIndex);
    return newIndex;
}

QModelIndex TagModel::demote(const QModelIndex & itemIndex)
{
    QNDEBUG("TagModel::demote: index: is valid = " << (itemIndex.isValid() ? "true" : "false")
            << ", row = " << itemIndex.row() << ", column = " << itemIndex.column());

    if (!itemIndex.isValid()) {
        return itemIndex;
    }

    const TagModelItem * item = itemForIndex(itemIndex);
    if (!item) {
        QNDEBUG("No item for given index");
        return itemIndex;
    }

    if (item == m_fakeRootItem) {
        QNDEBUG("Fake root item is not a real item");
        return itemIndex;
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * parentItem = item->parent();
    if (!parentItem) {
        parentItem = m_fakeRootItem;
        item->setParent(parentItem);
    }

    int row = parentItem->rowForChild(item);
    if (row < 0) {
        QNDEBUG("Can't find row of promoted item within its parent item");
        return itemIndex;
    }
    else if (row == 0) {
        QNDEBUG("No preceding sibling in parent to demote this item under");
        return itemIndex;
    }

    const TagModelItem * siblingItem = parentItem->childAtRow(row - 1);
    if (Q_UNLIKELY(!siblingItem)) {
        QNDEBUG("No sibling item was found");
        return itemIndex;
    }

    QModelIndex siblingItemIndex = indexForItem(siblingItem);
    if (Q_UNLIKELY(!siblingItemIndex.isValid())) {
        QNDEBUG("Can't get the valid index for sibling item");
        return itemIndex;
    }

    const TagModelItem * takenItem = parentItem->takeChild(row);
    if (Q_UNLIKELY(takenItem != item)) {
        QNWARNING("Internal inconsistency in tag model: item to take out from parent doesn't match the original demoted item");
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    siblingItem->addChild(takenItem);
    int newRow = siblingItem->numChildren() - 1;
    if (Q_UNLIKELY(newRow < 0)) {
        QNWARNING("Internal inconsistency in tag model: new row for demoted item is negative");
        Q_UNUSED(siblingItem->takeChild(newRow))
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    QModelIndex newIndex = index(newRow, Columns::Name, siblingItemIndex);
    if (!newIndex.isValid()) {
        QNWARNING("Can't get the valid index for the demoted item in tag model");
        Q_UNUSED(siblingItem->takeChild(newRow))
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    TagModelItem copyItem = *takenItem;
    copyItem.setParentLocalUid(siblingItem->localUid());
    copyItem.setParentGuid(siblingItem->guid());

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(copyItem.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO("The deletemoted tag model item was not found in the underlying item which is odd. Adding it there");
        Q_UNUSED(localUidIndex.insert(copyItem))
    }
    else {
        localUidIndex.replace(it, copyItem);
    }

    emit dataChanged(newIndex, newIndex);
    return newIndex;
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
            int row = parentItem.rowForChild(&item);
            if (row < 0) {
                parentItem.addChild(&item);
            }
        }
    }
}

QString TagModel::nameForNewTag() const
{
    QString baseName = tr("New tag");
    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    return newItemName<TagDataByNameUpper>(nameIndex, m_lastNewTagNameCounter, baseName);
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
