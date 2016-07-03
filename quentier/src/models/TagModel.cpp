#include "TagModel.h"
#include "NewItemNameGenerator.hpp"
#include <quentier/logging/QuentierLogger.h>
#include <QByteArray>
#include <QMimeData>
#include <limits>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT (100)

#define NUM_TAG_MODEL_COLUMNS (4)

namespace quentier {

TagModel::TagModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                   TagCache & cache, QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_cache(cache),
    m_lowerCaseTagNames(),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_tagItemsNotYetInLocalStorageUids(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_expungeTagRequestIds(),
    m_findTagToRestoreFailedUpdateRequestIds(),
    m_findTagToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_tagRestrictionsByLinkedNotebookGuid(),
    m_findNotebookRequestForLinkedNotebookGuid(),
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

    if ((index.column() == Columns::Dirty) ||
        (index.column() == Columns::FromLinkedNotebook))
    {
        return indexFlags;
    }

    const TagModelItem * item = itemForIndex(index);
    if (Q_UNLIKELY(!item)) {
        return indexFlags;
    }

    if (!canUpdateTagItem(*item)) {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        QModelIndex parentIndex = index;

        while(true) {
            const TagModelItem * parentItem = itemForIndex(parentIndex);
            if (Q_UNLIKELY(!parentItem)) {
                break;
            }

            if (parentItem == m_fakeRootItem) {
                break;
            }

            if (parentItem->isSynchronizable()) {
                return indexFlags;
            }

            if (!canUpdateTagItem(*parentItem)) {
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
    case Columns::FromLinkedNotebook:
        column = Columns::FromLinkedNotebook;
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(*item, column);
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
        return QVariant(tr("Name"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Dirty"));
    case Columns::FromLinkedNotebook:
        return QVariant(tr("From linked notebook"));
    default:
        return QVariant();
    }
}

int TagModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
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
    if (!m_fakeRootItem || (row < 0) || (column < 0) || (column >= NUM_TAG_MODEL_COLUMNS) ||
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
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Internal inconsistency detected in TagModel: parent of the item can't fint the item within its children: item = "
                  << *parentItem << "\nParent item: " << *grandParentItem);
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
    if (role != Qt::EditRole) {
        return false;
    }

    if (!modelIndex.isValid()) {
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        QNWARNING("The \"dirty\" flag can't be set manually in TagModel");
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        QNWARNING("The \"from linked notebook\" flag can't be set manually in TagModel");
        return false;
    }

    const TagModelItem * item = itemForIndex(modelIndex);
    if (!item) {
        return false;
    }

    if (item == m_fakeRootItem) {
        return false;
    }

    if (!canUpdateTagItem(*item)) {
        return false;
    }

    bool shouldMakeParentsSynchronizable = false;

    TagModelItem itemCopy = *item;
    bool dirty = itemCopy.isDirty();
    switch(modelIndex.column())
    {
    case Columns::Name:
        {
            QString newName = value.toString().trimmed();
            bool changed = (newName != itemCopy.name());
            if (!changed) {
                return true;
            }

            auto it = m_lowerCaseTagNames.find(newName.toLower());
            if (it != m_lowerCaseTagNames.end()) {
                QString error = QT_TR_NOOP("Can't change tag name: no two tags within the account are allowed "
                                           "to have the same name in a case-insensitive manner");
                QNINFO(error << ", suggested name = " << newName);
                emit notifyError(error);
                return false;
            }

            QString error;
            if (!Tag::validateName(newName, &error)) {
                error = QT_TR_NOOP("Can't change tag name") + QStringLiteral(": ") + error;
                QNINFO(error << ", suggested name = " << newName);
                emit notifyError(error);
                return false;
            }

            dirty = true;
            itemCopy.setName(newName);
            break;
        }
    case Columns::Synchronizable:
        {
            if (itemCopy.isSynchronizable() && !value.toBool()) {
                QString error = QT_TR_NOOP("Can't make already synchronizable tag not synchronizable");
                QNINFO(error << ", already synchronizable tag item: " << itemCopy);
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

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(itemCopy);
    emit layoutChanged();

    updateTagInLocalStorage(itemCopy);
    return true;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * parentItem = (parent.isValid()
                                       ? itemForIndex(parent)
                                       : m_fakeRootItem);
    if (!parentItem) {
        return false;
    }

    if ((parentItem != m_fakeRootItem) && !canCreateTagItem(*parentItem)) {
        return false;
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    std::vector<TagDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        TagModelItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewTag());
        item.setDirty(true);
        item.setParent(parentItem);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    emit layoutAboutToBeChanged();
    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const TagModelItem & item = *(*it);
        updateItemRowWithRespectToSorting(item);
        updateTagInLocalStorage(item);
    }
    emit layoutChanged();

    return true;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
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
            QNWARNING("Detected null pointer to child tag item on attempt to remove row " << row + i
                      << " from parent item: " << *parentItem);
            continue;
        }

        if (!item->linkedNotebookGuid().isEmpty()) {
            QString error = QT_TR_NOOP("Can't remove tag from linked notebook");
            QNINFO(error);
            emit notifyError(error);
            return false;
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

void TagModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG("TagModel::sort: column = " << column << ", order = " << order
            << " (" << (order == Qt::AscendingOrder ? "ascending" : "descending") << ")");

    if (column != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG("The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it) {
        updateItemRowWithRespectToSorting(*it);
    }

    updatePersistentModelIndices();
    emit layoutChanged();
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

    if ((parentItem != m_fakeRootItem) && !canCreateTagItem(*parentItem)) {
        return false;
    }

    QByteArray data = qUncompress(mimeData->data(TAG_MODEL_MIME_TYPE));
    TagModelItem item;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> item;

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

        if (!originalItemParent->linkedNotebookGuid().isEmpty()) {
            QString error = QT_TR_NOOP("Can't drag tag items from parent tags coming from linked notebook");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }

        QModelIndex originalParentIndex = indexForItem(originalItemParent);
        int originalItemRow = originalItemParent->rowForChild(&(*originalItemIt));

        if (originalItemRow >= 0) {
            beginRemoveRows(originalParentIndex, originalItemRow, originalItemRow);
            Q_UNUSED(originalItemParent->takeChild(originalItemRow));
            endRemoveRows();
        }
    }

    beginInsertRows(parentIndex, row, row);
    auto it = localUidIndex.end();
    if (originalItemIt != localUidIndex.end()) {
        localUidIndex.replace(originalItemIt, item);
        mapChildItems(*originalItemIt);
        it = originalItemIt;
    }
    else {
        auto insertionResult = localUidIndex.insert(item);
        it = insertionResult.first;
        mapChildItems(*it);
    }

    parentItem->insertChild(row, &(*it));
    endInsertRows();

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(*it);
    emit layoutChanged();

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
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE("Emitting the request to find a tag: local uid = " << tag.localUid()
            << ", request id = " << requestId);
    emit findTag(tag, requestId);
}

void TagModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("TagModel::onFindTagComplete: tag = " << tag << "\nRequest id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(tag.localUid(), tag);
        TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(tag.localUid());
        if (it != localUidIndex.end()) {
            updateTagInLocalStorage(*it);
        }
    }
}

void TagModel::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()))
    {
        return;
    }

    QNDEBUG("TagModel::onFindTagFailed: tag = " << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
    }

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
    }
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
            << ", request id = " << requestId);

    Q_UNUSED(m_expungeTagRequestIds.erase(it))

    onTagAddedOrUpdated(tag);
}

void TagModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNDEBUG("TagModel::onFindNotebookComplete: notebook: " << notebook << "\nRequest id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))

    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNWARNING("TagModel::onFindNotebookFailed: notebook = " << notebook << "\nError description = "
              << errorDescription << ", request id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))
}

void TagModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("TagModel::onUpdateNotebookComplete: local uid = " << notebook.localUid());
    Q_UNUSED(requestId)
    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("TagModel::onExpungeNotebookComplete: local uid = " << notebook.localUid()
            << ", linked notebook guid = " << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : QStringLiteral("<null>")));

    Q_UNUSED(requestId)

    if (!notebook.hasLinkedNotebookGuid()) {
        return;
    }

    auto it = m_tagRestrictionsByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
    if (it == m_tagRestrictionsByLinkedNotebookGuid.end()) {
        Restrictions restrictions;
        restrictions.m_canCreateTags = false;
        restrictions.m_canUpdateTags = false;
        m_tagRestrictionsByLinkedNotebookGuid[notebook.linkedNotebookGuid()] = restrictions;
        return;
    }

    it->m_canCreateTags = false;
    it->m_canUpdateTags = false;
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
    QObject::connect(this, QNSIGNAL(TagModel,expungeTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));

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
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QString,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagFailed,Tag,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onExpungeNotebookComplete,Notebook,QUuid));
}

void TagModel::requestTagsList()
{
    QNDEBUG("TagModel::requestTagsList: offset = " << m_listTagsOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to list tags: offset = " << m_listTagsOffset << ", request id = " << m_listTagsRequestId);
    emit listTags(flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, QString(), m_listTagsRequestId);
}

void TagModel::onTagAddedOrUpdated(const Tag & tag)
{
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(tag.localUid(), tag);

    auto itemIt = localUidIndex.find(tag.localUid());
    bool newTag = (itemIt == localUidIndex.end());
    if (newTag) {
        onTagAdded(tag);
    }
    else {
        onTagUpdated(tag, itemIt);
    }
}

void TagModel::onTagAdded(const Tag & tag)
{
    QNDEBUG("TagModel::onTagAdded: tag local uid = " << tag.localUid());

    if (tag.hasName()) {
        Q_UNUSED(m_lowerCaseTagNames.insert(tag.name().toLower()))
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const TagModelItem * parentItem = Q_NULLPTR;

    if (tag.hasParentLocalUid()) {
        auto parentIt = localUidIndex.find(tag.parentLocalUid());
        if (parentIt != localUidIndex.end()) {
            parentItem = &(*parentIt);
        }
    }

    if (!parentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        parentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(parentItem);

    TagModelItem item;
    tagToItem(tag, item);

    const QString & linkedNotebookGuid = item.linkedNotebookGuid();
    if (!linkedNotebookGuid.isEmpty())
    {
        auto restrictionsIt = m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (restrictionsIt == m_tagRestrictionsByLinkedNotebookGuid.end())
        {
            auto it = m_findNotebookRequestForLinkedNotebookGuid.left.find(linkedNotebookGuid);
            if (it == m_findNotebookRequestForLinkedNotebookGuid.left.end())
            {
                QUuid requestId = QUuid::createUuid();
                m_findNotebookRequestForLinkedNotebookGuid.insert(LinkedNotebookGuidWithFindNotebookRequestIdBimap::value_type(linkedNotebookGuid, requestId));
                Notebook notebook;
                notebook.setLinkedNotebookGuid(linkedNotebookGuid);
                QNTRACE("Emitted the request to find notebook by linked notebook guid: " << linkedNotebookGuid
                        << ", request id = " << requestId);
                emit findNotebook(notebook, requestId);
            }
        }
    }

    // The new item is going to be inserted into the last row of the parent item
    int row = parentItem->numChildren();

    beginInsertRows(parentIndex, row, row);

    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    it->setParent(parentItem);
    mapChildItems(*it);

    endInsertRows();

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(*it);
    emit layoutChanged();
}

void TagModel::onTagUpdated(const Tag & tag, TagDataByLocalUid::iterator it)
{
    QNDEBUG("TagModel::onTagUpdated: tag local uid = " << tag.localUid());

    const TagModelItem & originalItem = *it;
    auto nameIt = m_lowerCaseTagNames.find(originalItem.name().toLower());
    if (nameIt != m_lowerCaseTagNames.end()) {
        Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
    }

    if (tag.hasName()) {
        Q_UNUSED(m_lowerCaseTagNames.insert(tag.name().toLower()))
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    TagModelItem itemCopy;
    tagToItem(tag, itemCopy);

    Q_UNUSED(localUidIndex.replace(it, itemCopy))

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

    emit layoutAboutToBeChanged();
    updateItemRowWithRespectToSorting(*item);
    emit layoutChanged();
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

    if (tag.hasLinkedNotebookGuid()) {
        item.setLinkedNotebookGuid(tag.linkedNotebookGuid());
    }

    item.setSynchronizable(!tag.isLocal());
    item.setDirty(tag.isDirty());

    QNTRACE("Created tag model item from tag; item: " << item << "\nTag: " << tag);
}

bool TagModel::canUpdateTagItem(const TagModelItem & item) const
{
    const QString & linkedNotebookGuid = item.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    auto it = m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (it == m_tagRestrictionsByLinkedNotebookGuid.end()) {
        return false;
    }

    const Restrictions & restrictions = it.value();
    if (!restrictions.m_canUpdateTags) {
        return false;
    }

    return true;
}

bool TagModel::canCreateTagItem(const TagModelItem & parentItem) const
{
    const QString & linkedNotebookGuid = parentItem.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    auto it = m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (it == m_tagRestrictionsByLinkedNotebookGuid.end()) {
        return false;
    }

    const Restrictions & restrictions = it.value();
    if (!restrictions.m_canCreateTags) {
        return false;
    }

    return true;
}

void TagModel::updateRestrictionsFromNotebook(const Notebook & notebook)
{
    QNDEBUG("TagModel::updateRestrictionsFromNotebook: local uid = " << notebook.localUid()
            << ", linked notebook guid = " << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : QStringLiteral("<null>")));

    if (!notebook.hasLinkedNotebookGuid()) {
        QNDEBUG("Not a linked notebook, ignoring it");
        return;
    }

    Restrictions restrictions;

    if (!notebook.hasRestrictions())
    {
        restrictions.m_canCreateTags = true;
        restrictions.m_canUpdateTags = true;
    }
    else
    {
        const qevercloud::NotebookRestrictions & notebookRestrictions = notebook.restrictions();
        restrictions.m_canCreateTags = (notebookRestrictions.noCreateTags.isSet()
                                        ? (!notebookRestrictions.noCreateTags.ref())
                                        : true);
        restrictions.m_canUpdateTags = (notebookRestrictions.noUpdateTags.isSet()
                                        ? (!notebookRestrictions.noUpdateTags.ref())
                                        : true);
    }

    m_tagRestrictionsByLinkedNotebookGuid[notebook.linkedNotebookGuid()] = restrictions;
    QNDEBUG("Set restrictions for tags from linked notebook with guid " << notebook.linkedNotebookGuid()
            << ": can create tags = " << (restrictions.m_canCreateTags ? "true" : "false")
            << ", can update tags = " << (restrictions.m_canUpdateTags ? "true" : "false"));
}

QVariant TagModel::dataImpl(const TagModelItem & item, const Columns::type column) const
{
    switch(column)
    {
    case Columns::Name:
        return QVariant(item.name());
    case Columns::Synchronizable:
        return QVariant(item.isSynchronizable());
    case Columns::Dirty:
        return QVariant(item.isDirty());
    case Columns::FromLinkedNotebook:
        return QVariant(!item.linkedNotebookGuid().isEmpty());
    default:
        return QVariant();
    }
}

QVariant TagModel::dataAccessibleText(const TagModelItem & item, const Columns::type column) const
{
    QVariant textData = dataImpl(item, column);
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
        accessibleText += (textData.toBool() ? QT_TR_NOOP("synchronizable") : QT_TR_NOOP("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (textData.toBool() ? QT_TR_NOOP("dirty") : QT_TR_NOOP("not dirty"));
        break;
    case Columns::FromLinkedNotebook:
        accessibleText += (textData.toBool() ? QT_TR_NOOP("from linked notebook") : QT_TR_NOOP("from own account"));
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

    if ((grandParentItem != m_fakeRootItem) &&
        (!canCreateTagItem(*grandParentItem) || !canUpdateTagItem(*grandParentItem)))
    {
        QNDEBUG("Can't create and/or update tags for the grand parent item");
        return itemIndex;
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

    int appropriateRow = rowForNewItem(*grandParentItem, *takenItem);
    grandParentItem->insertChild(appropriateRow, takenItem);

    QModelIndex newIndex = index(appropriateRow, Columns::Name, grandParentIndex);
    if (!newIndex.isValid()) {
        QNWARNING("Can't get the valid index for the promoted item in tag model");
        Q_UNUSED(grandParentItem->takeChild(appropriateRow))
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

    if ((parentItem != m_fakeRootItem) && !canUpdateTagItem(*parentItem)) {
        QNDEBUG("Can't update tags for the parent item");
        return itemIndex;
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

    if (!canCreateTagItem(*siblingItem)) {
        QNDEBUG("Can't create tags within the sibling tag item");
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

    int appropriateRow = rowForNewItem(*siblingItem, *takenItem);
    siblingItem->insertChild(appropriateRow, takenItem);

    QModelIndex newIndex = index(appropriateRow, Columns::Name, siblingItemIndex);
    if (!newIndex.isValid()) {
        QNWARNING("Can't get the valid index for the demoted item in tag model");
        Q_UNUSED(siblingItem->takeChild(appropriateRow))
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

QStringList TagModel::tagNames() const
{
    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    QStringList result;
    result.reserve(static_cast<int>(localUidIndex.size()));

    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it) {
        const QString tagName = it->name();
        result << tagName;
    }

    return result;
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
                int row = rowForNewItem(parentItem, item);
                parentItem.insertChild(row, &item);
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

    auto nameIt = m_lowerCaseTagNames.find(item.name().toLower());
    if (nameIt != m_lowerCaseTagNames.end()) {
        Q_UNUSED(m_lowerCaseTagNames.erase(nameIt))
    }

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

    // Need to recursively remove all the children of this tag and do this before the actual removal of their parent
    TagDataByParentLocalUid & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
    while(true)
    {
        auto childIt = parentLocalUidIndex.find(localUid);
        if (childIt == parentLocalUidIndex.end()) {
            break;
        }

        const TagModelItem & childItem = *childIt;
        removeItemByLocalUid(childItem.localUid());
    }

    QModelIndex parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    Q_UNUSED(localUidIndex.erase(itemIt))
    endRemoveRows();
}

int TagModel::rowForNewItem(const TagModelItem & parentItem, const TagModelItem & newItem) const
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return parentItem.numChildren();
    }

    QList<const TagModelItem*> children = parentItem.children();
    auto it = children.end();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(children.begin(), children.end(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(children.begin(), children.end(), &newItem, GreaterByName());
    }

    if (it == children.end()) {
        return parentItem.numChildren();
    }

    int row = static_cast<int>(std::distance(children.begin(), it));
    return row;
}

void TagModel::updateItemRowWithRespectToSorting(const TagModelItem & item)
{
    if (m_sortedColumn != Columns::Name) {
        // Sorting by other columns is not yet implemented
        return;
    }

    const TagModelItem * parentItem = item.parent();
    if (!parentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        parentItem = m_fakeRootItem;
        int row = rowForNewItem(*parentItem, item);
        parentItem->insertChild(row, &item);
        return;
    }

    int currentItemRow = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(QT_TR_NOOP("Can't update tag model item's row: can't find its original row within parent: ") << item);
        return;
    }

    Q_UNUSED(parentItem->takeChild(currentItemRow))
    int appropriateRow = rowForNewItem(*parentItem, item);
    parentItem->insertChild(appropriateRow, &item);
    QNTRACE("Moved item from row " << currentItemRow << " to row " << appropriateRow << "; item: " << item);
}

void TagModel::updatePersistentModelIndices()
{
    QNDEBUG("TagModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    QModelIndexList indices = persistentIndexList();
    for(auto it = indices.begin(), end = indices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        const TagModelItem * item = reinterpret_cast<const TagModelItem*>(index.internalPointer());
        QModelIndex replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
}

void TagModel::updateTagInLocalStorage(const TagModelItem & item)
{
    QNDEBUG("TagModel::updateTagInLocalStorage: local uid = " << item.localUid());

    Tag tag;

    auto notYetSavedItemIt = m_tagItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_tagItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG("Updating the tag");

        const Tag * pCachedTag = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedTag))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))
            Tag dummy;
            dummy.setLocalUid(item.localUid());
            QNDEBUG("Emitting the request to find tag: local uid = " << item.localUid()
                    << ", request id = " << requestId);
            emit findTag(dummy, requestId);
            return;
        }

        tag = *pCachedTag;
    }

    tag.setLocalUid(item.localUid());
    tag.setGuid(item.guid());
    tag.setLinkedNotebookGuid(item.linkedNotebookGuid());
    tag.setName(item.name());
    tag.setLocal(!item.isSynchronizable());
    tag.setDirty(item.isDirty());

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addTagRequestIds.insert(requestId));

        QNTRACE("Emitting the request to add the tag to local storage: id = " << requestId
                << ", tag: " << tag);
        emit addTag(tag, requestId);

        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));

        QNTRACE("Emitting the request to update the tag in the local storage: id = " << requestId
                << ", tag: " << tag);
        emit updateTag(tag, requestId);
    }
}

bool TagModel::LessByName::operator()(const TagModelItem & lhs, const TagModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

bool TagModel::LessByName::operator()(const TagModelItem * lhs, const TagModelItem * rhs) const
{
    if (!lhs) {
        return true;
    }
    else if (!rhs) {
        return false;
    }
    else {
        return this->operator()(*lhs, *rhs);
    }
}

bool TagModel::GreaterByName::operator()(const TagModelItem & lhs, const TagModelItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

bool TagModel::GreaterByName::operator()(const TagModelItem * lhs, const TagModelItem * rhs) const
{
    if (!lhs) {
        return true;
    }
    else if (!rhs) {
        return false;
    }
    else {
        return this->operator()(*lhs, *rhs);
    }
}

} // namespace quentier
