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

#include "TagModel.h"
#include "NewItemNameGenerator.hpp"
#include <quentier/logging/QuentierLogger.h>
#include <QByteArray>
#include <QMimeData>
#include <limits>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT (100)

#define NUM_TAG_MODEL_COLUMNS (5)

namespace quentier {

TagModel::TagModel(const Account & account, LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                   TagCache & cache, QObject * parent) :
    QAbstractItemModel(parent),
    m_account(account),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_cache(cache),
    m_indexIdToLocalUidBimap(),
    m_lastFreeIndexId(1),
    m_lowerCaseTagNames(),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_tagItemsNotYetInLocalStorageUids(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_expungeTagRequestIds(),
    m_noteCountPerTagRequestIds(),
    m_findTagToRestoreFailedUpdateRequestIds(),
    m_findTagToPerformUpdateRequestIds(),
    m_findTagAfterNotelessTagsErasureRequestIds(),
    m_listTagsPerNoteRequestIds(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_tagRestrictionsByLinkedNotebookGuid(),
    m_findNotebookRequestForLinkedNotebookGuid(),
    m_lastNewTagNameCounter(0),
    m_allTagsListed(false)
{
    createConnections(localStorageManagerThreadWorker);
    requestTagsList();
}

TagModel::~TagModel()
{
    delete m_fakeRootItem;
}

void TagModel::updateAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("TagModel::updateAccount: ") << account);
    m_account = account;
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

        while(true)
        {
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
    case Columns::NumNotesPerTag:
        column = Columns::NumNotesPerTag;
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

    return columnName(static_cast<Columns::type>(section));
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

    IndexId id = idForItem(*item);
    if (Q_UNLIKELY(id == 0)) {
        return QModelIndex();
    }

    return createIndex(row, column, id);
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
        QNWARNING(QStringLiteral("Internal inconsistency detected in TagModel: parent of the item can't find the item within its children: item = ")
                  << *parentItem << QStringLiteral("\nParent item: ") << *grandParentItem);
        return QModelIndex();
    }

    IndexId id = idForItem(*parentItem);
    if (Q_UNLIKELY(id == 0)) {
        return QModelIndex();
    }

    return createIndex(row, Columns::Name, id);
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
        QNWARNING(QStringLiteral("The \"dirty\" flag can't be set manually in TagModel"));
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        QNWARNING(QStringLiteral("The \"from linked notebook\" flag can't be set manually in TagModel"));
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
                QNLocalizedString error = QT_TR_NOOP("can't change tag name: no two tags within the account are allowed "
                                                     "to have the same name in a case-insensitive manner");
                QNINFO(error << QStringLiteral(", suggested name = ") << newName);
                emit notifyError(error);
                return false;
            }

            QNLocalizedString errorDescription;
            if (!Tag::validateName(newName, &errorDescription)) {
                QNLocalizedString error = QT_TR_NOOP("can't change tag name");
                error += QStringLiteral(": ");
                error += errorDescription;
                QNINFO(error << QStringLiteral("; suggested name = ") << newName);
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
                QNLocalizedString error = QT_TR_NOOP("can't make already synchronizable tag not synchronizable");
                QNINFO(error << QStringLiteral(", already synchronizable tag item: ") << itemCopy);
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
                QNLocalizedString error = QT_TR_NOOP("can't find one of currently made synchronizable tag's parent tags");
                QNWARNING(error << QStringLiteral(", item: ") << dummy);
                emit notifyError(error);
                return false;
            }

            index.replace(dummyIt, dummy);
            QModelIndex changedIndex = indexForLocalUid(dummy.localUid());
            if (Q_UNLIKELY(!changedIndex.isValid())) {
                QNLocalizedString error = QT_TR_NOOP("can't get the valid model index for one of currently made synchronizable tag's parent tags");
                QNWARNING(error << QStringLiteral(", item for which the index was requested: ") << dummy);
                emit notifyError(error);
                return false;
            }

            changedIndex = this->index(changedIndex.row(), Columns::Synchronizable,
                                       changedIndex.parent());
            emit dataChanged(changedIndex, changedIndex);
            processedItem = parentItem;
        }
    }

    auto it = index.find(itemCopy.localUid());
    if (Q_UNLIKELY(it == index.end())) {
        QNLocalizedString error = QT_TR_NOOP("can't find the tag being modified");
        QNWARNING(error << QStringLiteral(" by its local uid , item: ") << itemCopy);
        emit notifyError(error);
        return false;
    }

    index.replace(it, itemCopy);
    emit dataChanged(modelIndex, modelIndex);

    if (m_sortedColumn == Columns::Name) {
        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(itemCopy);
        emit layoutChanged();
    }

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
    int numExistingTags = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingTags + count >= m_account.tagCountMax())) {
        QNLocalizedString error = QT_TR_NOOP("can't create tag(s): the account can contain a limited number of tags");
        error += QStringLiteral(": ");
        error += QString::number(m_account.tagCountMax());
        QNINFO(error);
        emit notifyError(error);
        return false;
    }

    QVector<TagDataByLocalUid::iterator> addedItems;
    addedItems.reserve(std::max(count, 0));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        TagModelItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewTag());
        item.setDirty(true);
        item.setParent(parentItem);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    if (m_sortedColumn == Columns::Name)
    {
        emit layoutAboutToBeChanged();

        for(auto it = addedItems.constBegin(), end = addedItems.constEnd(); it != end; ++it) {
            const TagModelItem & item = *(*it);
            updateItemRowWithRespectToSorting(item);
        }

        emit layoutChanged();
    }

    for(auto it = addedItems.constBegin(), end = addedItems.constEnd(); it != end; ++it) {
        updateTagInLocalStorage(*(*it));
    }

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
            QNWARNING(QStringLiteral("Detected null pointer to child tag item on attempt to remove row ") << row + i
                      << QStringLiteral(" from parent item: ") << *parentItem);
            continue;
        }

        if (!item->linkedNotebookGuid().isEmpty()) {
            QNLocalizedString error = QT_TR_NOOP("can't remove tag from linked notebook");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }

        if (item->isSynchronizable()) {
            QNLocalizedString error = QT_TR_NOOP("can't remove synchronizable tag");
            QNINFO(error);
            emit notifyError(error);
            return false;
        }

        if (hasSynchronizableChildren(item)) {
            QNLocalizedString error = QT_TR_NOOP("can't remove tag with synchronizable children");
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
        QNTRACE(QStringLiteral("Emitted the request to expunge the tag from the local storage: request id = ")
                << requestId << QStringLiteral(", tag local uid: ") << item->localUid());

        auto it = index.find(item->localUid());
        Q_UNUSED(index.erase(it))
    }
    endRemoveRows();

    return true;
}

void TagModel::sort(int column, Qt::SortOrder order)
{
    QNDEBUG(QStringLiteral("TagModel::sort: column = ") << column << QStringLiteral(", order = ") << order
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
    emit sortingChanged();

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
    QNDEBUG(QStringLiteral("TagModel::dropMimeData: action = ") << action << QStringLiteral(", row = ") << row
            << QStringLiteral(", column = ") << column << QStringLiteral(", parent index: is valid = ")
            << (parentIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", parent row = ") << parentIndex.row() << QStringLiteral(", parent column = ")
            << (parentIndex.column()) << QStringLiteral(", mime data formats: ")
            << (mimeData ? mimeData->formats().join(QStringLiteral("; ")) : QStringLiteral("<null>")));

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
            QNLocalizedString error = QT_TR_NOOP("can't drag tag items from parent tags coming from linked notebook");
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
    QNDEBUG(QStringLiteral("TagModel::onAddTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_addTagRequestIds.find(requestId);
    if (it != m_addTagRequestIds.end()) {
        Q_UNUSED(m_addTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
    requestNoteCountForTag(tag);
}

void TagModel::onAddTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_addTagRequestIds.find(requestId);
    if (it == m_addTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onAddTagFailed: tag = ") << tag << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_addTagRequestIds.erase(it))

    emit notifyError(errorDescription);

    removeItemByLocalUid(tag.localUid());
}

void TagModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onUpdateTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end()) {
        Q_UNUSED(m_updateTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
    // NOTE: no need to re-request the number of notes per this tag - the update of the tag itself doesn't change
    // anything about which notes use the tag
}

void TagModel::onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onUpdateTagFailed: tag = ") << tag << QStringLiteral("\nError description = ") << errorDescription
            << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))

    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to find a tag: local uid = ") << tag.localUid()
            << QStringLiteral(", request id = ") << requestId);
    emit findTag(tag, requestId);
}

void TagModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    auto checkAfterErasureIt = m_findTagAfterNotelessTagsErasureRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (checkAfterErasureIt == m_findTagAfterNotelessTagsErasureRequestIds.end()))
    {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onFindTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end())
    {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end())
    {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(tag.localUid(), tag);
        TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(tag.localUid());
        if (it != localUidIndex.end()) {
            updateTagInLocalStorage(*it);
        }
    }
    else if (checkAfterErasureIt != m_findTagAfterNotelessTagsErasureRequestIds.end())
    {
        QNDEBUG(QStringLiteral("Tag still exists after expunging the noteless tags from linked notebooks: ") << tag);
        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.erase(checkAfterErasureIt))
        onTagAddedOrUpdated(tag);
    }
}

void TagModel::onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt = m_findTagToRestoreFailedUpdateRequestIds.find(requestId);
    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);
    auto checkAfterErasureIt = m_findTagAfterNotelessTagsErasureRequestIds.find(requestId);
    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (checkAfterErasureIt == m_findTagAfterNotelessTagsErasureRequestIds.end()))
    {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onFindTagFailed: tag = ") << tag << QStringLiteral("\nError description = ") << errorDescription
            << QStringLiteral(", request id = ") << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (checkAfterErasureIt != m_findTagAfterNotelessTagsErasureRequestIds.end()) {
        QNDEBUG(QStringLiteral("Tag no longer exists after the noteless tags from linked notebooks erasure"));
        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.erase(checkAfterErasureIt))
        removeItemByLocalUid(tag.localUid());
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

    QNDEBUG(QStringLiteral("TagModel::onListTagsComplete: flag = ") << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid)
            << QStringLiteral(", num found tags = ") << foundTags.size() << QStringLiteral(", request id = ") << requestId);

    for(auto it = foundTags.constBegin(), end = foundTags.constEnd(); it != end; ++it) {
        onTagAddedOrUpdated(*it);
        requestNoteCountForTag(*it);
    }

    m_listTagsRequestId = QUuid();

    if (foundTags.size() == static_cast<int>(limit)) {
        QNTRACE(QStringLiteral("The number of found tags matches the limit, requesting more tags from the local storage"));
        m_listTagsOffset += limit;
        requestTagsList();
        return;
    }

    m_allTagsListed = true;
    emit notifyAllTagsListed();
}

void TagModel::onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                size_t limit, size_t offset,
                                LocalStorageManager::ListTagsOrder::type order,
                                LocalStorageManager::OrderDirection::type orderDirection,
                                QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListTagsFailed: flag = ") << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid)
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listTagsRequestId = QUuid();

    emit notifyError(errorDescription);
}

void TagModel::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    auto it = m_expungeTagRequestIds.find(requestId);
    if (it != m_expungeTagRequestIds.end()) {
        Q_UNUSED(m_expungeTagRequestIds.erase(it))
        return;
    }

    removeItemByLocalUid(tag.localUid());
}

void TagModel::onExpungeTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_expungeTagRequestIds.find(requestId);
    if (it == m_expungeTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onExpungeTagFailed: tag = ") << tag << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_expungeTagRequestIds.erase(it))

    onTagAddedOrUpdated(tag);
}

void TagModel::onNoteCountPerTagComplete(int noteCount, Tag tag, QUuid requestId)
{
    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onNoteCountPerTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ")
            << requestId << QStringLiteral(", note count = ") << noteCount);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto itemIt = localUidIndex.find(tag.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNLocalizedString error = QT_TR_NOOP("No tag receiving the note count update was found in the model");
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *itemIt);
        emit notifyError(error);
        return;
    }

    const TagModelItem * parentItem = itemIt->parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNLocalizedString error = QT_TR_NOOP("tag item being updated with the note count does not have a parent item linked with it");
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *itemIt);
        emit notifyError(error);
        return;
    }

    int row = parentItem->rowForChild(&(*itemIt));
    if (Q_UNLIKELY(row < 0)) {
        QNLocalizedString error = QT_TR_NOOP("can't find the row of tag item being updated with the note count within its parent");
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *itemIt);
        emit notifyError(error);
        return;
    }

    TagModelItem itemCopy = *itemIt;
    itemCopy.setNumNotesPerTag(noteCount);
    Q_UNUSED(localUidIndex.replace(itemIt, itemCopy))

    const TagModelItem * item = &(*itemIt);
    IndexId id = idForItem(*item);
    QModelIndex index = createIndex(row, Columns::NumNotesPerTag, id);
    emit dataChanged(index, index);

    // NOTE: in future, if/when sorting by note count is supported, will need to check if need to re-sort and emit the layout change signals
}

void TagModel::onNoteCountPerTagFailed(QNLocalizedString errorDescription, Tag tag, QUuid requestId)
{
    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onNoteCountPerTagFailed: error description = ") << errorDescription
            << QStringLiteral(", tag = ") << tag << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))
}

void TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete: request id = ") << requestId);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(auto it = localUidIndex.begin(); it != localUidIndex.end(); )
    {
        const TagModelItem & item = *it;

        if (item.linkedNotebookGuid().isEmpty()) {
            continue;
        }

        // The item's current note count per tag may be invalid due to asynchronous events sequence,
        // need to ask the database if such an item actually exists
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.insert(requestId))
        Tag tag;
        tag.setLocalUid(item.localUid());
        QNTRACE(QStringLiteral("Emitting the request to find tag from linked notebook to check for its existence: ")
                << item.localUid() << QStringLiteral(", request id = ") << requestId);
        emit findTag(tag, requestId);
    }
}

void TagModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onFindNotebookComplete: notebook: ") << notebook << QStringLiteral("\nRequest id = ")
            << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))

    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNWARNING(QStringLiteral("TagModel::onFindNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
              << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))
}

void TagModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onUpdateNotebookComplete: local uid = ") << notebook.localUid());
    Q_UNUSED(requestId)
    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeNotebookComplete: local uid = ") << notebook.localUid()
            << QStringLiteral(", linked notebook guid = ")
            << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : QStringLiteral("<null>")));

    Q_UNUSED(requestId)

    // Notes from this notebook have been expunged along with it; need to re-request the number of notes per tag
    // for all tags
    requestNoteCountForAllTags();

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

void TagModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onAddNoteComplete: note = ") << note
            << QStringLiteral("\nRequest id = ") << requestId);

    if (Q_UNLIKELY(note.hasDeletionTimestamp())) {
        return;
    }

    requestTagsPerNote(note);
}

void TagModel::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    if (!updateTags) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onUpdateNoteComplete: note = ") << note
            << QStringLiteral("\nUpdate resources = ")
            << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", update tags = ")
            << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", request id = ") << requestId);

    requestTagsPerNote(note);
}

void TagModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeNoteComplete: note = ") << note
            << QStringLiteral("\nRequest id = ") << requestId);

    if (note.hasTagLocalUids())
    {
        const QStringList & tagLocalUids = note.tagLocalUids();
        for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it)
        {
            const QString & tagLocalUid = *it;
            Tag tag;
            tag.setLocalUid(tagLocalUid);
            requestNoteCountForTag(tag);
        }

        return;
    }

    QNDEBUG(QStringLiteral("Note has no tag local uids, have to re-request the note count for all tag items"));

    requestNoteCountForAllTags();
}

void TagModel::onListAllTagsPerNoteComplete(QList<Tag> foundTags, Note note,
                                            LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListTagsOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QUuid requestId)
{
    auto it = m_listTagsPerNoteRequestIds.find(requestId);
    if (it == m_listTagsPerNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListAllTagsPerNoteComplete: note = ") << note
            << QStringLiteral("\nFlag = ") << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
            << QStringLiteral(", order direction = ") << orderDirection << QStringLiteral(", request id = ")
            << requestId);

    for(auto it = foundTags.constBegin(), end = foundTags.constEnd(); it != end; ++it) {
        requestNoteCountForTag(*it);
    }
}

void TagModel::onListAllTagsPerNoteFailed(Note note, LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListTagsOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_listTagsPerNoteRequestIds.find(requestId);
    if (it == m_listTagsPerNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("TagModel::onListAllTagsPerNoteFailed: note = ") << note
              << QStringLiteral("\nFlag = ") << flag << QStringLiteral(", limit = ") << limit
              << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
              << QStringLiteral(", order direction = ") << orderDirection << QStringLiteral(", request id = ")
              << requestId << QStringLiteral(", error description = ") << errorDescription);

    // Trying to work around this problem by re-requesting the note count for all tags
    requestNoteCountForAllTags();
}

void TagModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG(QStringLiteral("TagModel::createConnections"));

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
    QObject::connect(this, QNSIGNAL(TagModel,requestNoteCountPerTag,Tag,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onNoteCountPerTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,listAllTagsPerNote,Note,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                    LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListAllTagsPerNoteRequest,Note,LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onAddTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onFindTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsComplete,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QList<Tag>,QUuid),
                     this, QNSLOT(TagModel,onListTagsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<Tag>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listTagsFailed,LocalStorageManager::ListObjectsOptions,
                                                                size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                                QString,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onListTagsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerTagComplete,int,Tag,QUuid),
                     this, QNSLOT(TagModel,onNoteCountPerTagComplete,int,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,noteCountPerTagFailed,QNLocalizedString,Tag,QUuid),
                     this, QNSLOT(TagModel,onNoteCountPerTagFailed,QNLocalizedString,Tag,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                     this, QNSLOT(TagModel,onExpungeNotelessTagsFromLinkedNotebooksComplete,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(TagModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(TagModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(TagModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllTagsPerNoteComplete,QList<Tag>,Note,LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(TagModel,onListAllTagsPerNoteComplete,QList<Tag>,Note,LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid));
}

void TagModel::requestTagsList()
{
    QNDEBUG(QStringLiteral("TagModel::requestTagsList: offset = ") << m_listTagsOffset);

    LocalStorageManager::ListObjectsOptions flags = LocalStorageManager::ListAll;
    LocalStorageManager::ListTagsOrder::type order = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to list tags: offset = ") << m_listTagsOffset << QStringLiteral(", request id = ")
            << m_listTagsRequestId);
    emit listTags(flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, QString(), m_listTagsRequestId);
}

void TagModel::requestNoteCountForTag(const Tag & tag)
{
    QNDEBUG(QStringLiteral("TagModel::requestNoteCountForTag: ") << tag);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerTagRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to compute the number of notes per tag, request id = ") << requestId);
    emit requestNoteCountPerTag(tag, requestId);
}

void TagModel::requestTagsPerNote(const Note & note)
{
    QNDEBUG(QStringLiteral("TagModel::requestTagsPerNote: ") << note);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_listTagsPerNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to list tags per note: request id = ") << requestId);
    emit listAllTagsPerNote(note, LocalStorageManager::ListObjectsOption::ListAll,
                            /* limit = */ 0, /* offset = */ 0, LocalStorageManager::ListTagsOrder::NoOrder,
                            LocalStorageManager::OrderDirection::Ascending, requestId);
}

void TagModel::requestNoteCountForAllTags()
{
    QNDEBUG(QStringLiteral("TagModel::requestNoteCountForAllTags"));

    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it) {
        const TagModelItem & item = *it;
        Tag tag;
        tag.setLocalUid(item.localUid());
        requestNoteCountForTag(tag);
    }
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
    QNDEBUG(QStringLiteral("TagModel::onTagAdded: tag local uid = ") << tag.localUid());

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
                QNTRACE(QStringLiteral("Emitted the request to find notebook by linked notebook guid: ") << linkedNotebookGuid
                        << QStringLiteral(", request id = ") << requestId);
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

    if (m_sortedColumn == Columns::Name) {
        emit layoutAboutToBeChanged();
        updateItemRowWithRespectToSorting(*it);
        emit layoutChanged();
    }
}

void TagModel::onTagUpdated(const Tag & tag, TagDataByLocalUid::iterator it)
{
    QNDEBUG(QStringLiteral("TagModel::onTagUpdated: tag local uid = ") << tag.localUid());

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
        QNLocalizedString error = QT_TR_NOOP("tag item being updated does not have a parent item linked with it");
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *item);
        emit notifyError(error);
        return;
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QNLocalizedString error = QT_TR_NOOP("can't find the row of tag item being updated within its parent");
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *item);
        emit notifyError(error);
        return;
    }

    IndexId itemId = idForItem(*item);

    QModelIndex modelIndexFrom = createIndex(row, 0, itemId);
    QModelIndex modelIndexTo = createIndex(row, NUM_TAG_MODEL_COLUMNS - 1, itemId);
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

    QNTRACE(QStringLiteral("Created tag model item from tag; item: ") << item << QStringLiteral("\nTag: ") << tag);
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
    QNDEBUG(QStringLiteral("TagModel::updateRestrictionsFromNotebook: local uid = ") << notebook.localUid()
            << QStringLiteral(", linked notebook guid = ") << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : QStringLiteral("<null>")));

    if (!notebook.hasLinkedNotebookGuid()) {
        QNDEBUG(QStringLiteral("Not a linked notebook, ignoring it"));
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
    QNDEBUG(QStringLiteral("Set restrictions for tags from linked notebook with guid ") << notebook.linkedNotebookGuid()
            << QStringLiteral(": can create tags = ") << (restrictions.m_canCreateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", can update tags = ") << (restrictions.m_canUpdateTags ? QStringLiteral("true") : QStringLiteral("false")));
}

const TagModelItem * TagModel::itemForId(const IndexId id) const
{
    QNDEBUG(QStringLiteral("TagModel::itemForId: ") << id);

    auto localUidIt = m_indexIdToLocalUidBimap.left.find(id);
    if (Q_UNLIKELY(localUidIt == m_indexIdToLocalUidBimap.left.end())) {
        QNDEBUG(QStringLiteral("Found no tag model item corresponding to model index internal id"));
        return Q_NULLPTR;
    }

    const QString & localUid = localUidIt->second;
    QNTRACE(QStringLiteral("Found tag local uid corresponding to model index internal id: ") << localUid);

    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto itemIt = localUidIndex.find(localUid);
    if (itemIt != localUidIndex.end()) {
        QNTRACE(QStringLiteral("Found tag item corresponding to local uid: ") << *itemIt);
        return &(*itemIt);
    }

    QNTRACE(QStringLiteral("Found no tag item corresponding to local uid"));
    return Q_NULLPTR;
}

TagModel::IndexId TagModel::idForItem(const TagModelItem & item) const
{
    auto it = m_indexIdToLocalUidBimap.right.find(item.localUid());
    if (it == m_indexIdToLocalUidBimap.right.end()) {
        IndexId id = m_lastFreeIndexId++;
        Q_UNUSED(m_indexIdToLocalUidBimap.insert(IndexIdToLocalUidBimap::value_type(id, item.localUid())))
        return id;
    }

    return it->second;
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
    case Columns::NumNotesPerTag:
        return QVariant(item.numNotesPerTag());
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

    QString accessibleText = tr("Tag") + QStringLiteral(": ");

    switch(column)
    {
    case Columns::Name:
        accessibleText += tr("name is") + QStringLiteral(" ") + textData.toString();
        break;
    case Columns::Synchronizable:
        accessibleText += (textData.toBool() ? tr("synchronizable") : tr("not synchronizable"));
        break;
    case Columns::Dirty:
        accessibleText += (textData.toBool() ? tr("dirty") : tr("not dirty"));
        break;
    case Columns::FromLinkedNotebook:
        accessibleText += (textData.toBool() ? tr("from linked notebook") : tr("from own account"));
        break;
    case Columns::NumNotesPerTag:
        accessibleText += tr("number of notes");
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

    return itemForId(static_cast<IndexId>(index.internalId()));
}

const TagModelItem * TagModel::itemForLocalUid(const QString & localUid) const
{
    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(localUid);
    if (it == localUidIndex.end()) {
        return Q_NULLPTR;
    }

    const TagModelItem & item = *it;
    return &item;
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
        QNWARNING(QStringLiteral("Internal error: can't get the row of the child item in parent in TagModel, child item: ")
                  << *item << QStringLiteral("\nParent item: ") << *parentItem);
        return QModelIndex();
    }

    IndexId itemId = idForItem(*item);
    return createIndex(row, Columns::Name, itemId);
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

QModelIndex TagModel::indexForTagName(const QString & tagName) const
{
    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto it = nameIndex.find(tagName.toUpper());
    if (it == nameIndex.end()) {
        return QModelIndex();
    }

    const TagModelItem & item = *it;
    return indexForItem(&item);
}

QModelIndex TagModel::promote(const QModelIndex & itemIndex)
{
    QNDEBUG(QStringLiteral("TagModel::promote: index: is valid = ") << (itemIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << itemIndex.row() << QStringLiteral(", column = ") << itemIndex.column());

    if (!itemIndex.isValid()) {
        return itemIndex;
    }

    const TagModelItem * item = itemForIndex(itemIndex);
    if (!item) {
        QNDEBUG(QStringLiteral("No item for given index"));
        return itemIndex;
    }

    if (item == m_fakeRootItem) {
        QNDEBUG(QStringLiteral("Fake root item is not a real item"));
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
        QNDEBUG(QStringLiteral("Already a top level item"));
        return itemIndex;
    }

    int row = parentItem->rowForChild(item);
    if (row < 0) {
        QNDEBUG(QStringLiteral("Can't find row of promoted item within its parent item"));
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
        QNDEBUG(QStringLiteral("Can't create and/or update tags for the grand parent item"));
        return itemIndex;
    }

    int parentRow = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(parentRow < 0)) {
        QNWARNING(QStringLiteral("Can't find parent item's row within its own parent item"));
        return itemIndex;
    }

    QModelIndex grandParentIndex = indexForItem(grandParentItem);
    if (Q_UNLIKELY(!grandParentIndex.isValid())) {
        QNWARNING(QStringLiteral("Can't get the valid index for the grand parent item of the promoted item in tag model"));
        return itemIndex;
    }

    const TagModelItem * takenItem = parentItem->takeChild(row);
    if (Q_UNLIKELY(takenItem != item)) {
        QNWARNING(QStringLiteral("Internal inconsistency in tag model: item to take out from parent doesn't match the original promoted item"));
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    int appropriateRow = rowForNewItem(*grandParentItem, *takenItem);
    grandParentItem->insertChild(appropriateRow, takenItem);

    QModelIndex newIndex = index(appropriateRow, Columns::Name, grandParentIndex);
    if (!newIndex.isValid()) {
        QNWARNING(QStringLiteral("Can't get the valid index for the promoted item in tag model"));
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
        QNINFO(QStringLiteral("The promoted tag model item was not found in the underlying item which is odd. Adding it there"));
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
    QNDEBUG(QStringLiteral("TagModel::demote: index: is valid = ") << (itemIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << itemIndex.row() << QStringLiteral(", column = ") << itemIndex.column());

    if (!itemIndex.isValid()) {
        return itemIndex;
    }

    const TagModelItem * item = itemForIndex(itemIndex);
    if (!item) {
        QNDEBUG(QStringLiteral("No item for given index"));
        return itemIndex;
    }

    if (item == m_fakeRootItem) {
        QNDEBUG(QStringLiteral("Fake root item is not a real item"));
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
        QNDEBUG(QStringLiteral("Can't update tags for the parent item"));
        return itemIndex;
    }

    int row = parentItem->rowForChild(item);
    if (row < 0) {
        QNDEBUG(QStringLiteral("Can't find row of promoted item within its parent item"));
        return itemIndex;
    }
    else if (row == 0) {
        QNDEBUG(QStringLiteral("No preceding sibling in parent to demote this item under"));
        return itemIndex;
    }

    const TagModelItem * siblingItem = parentItem->childAtRow(row - 1);
    if (Q_UNLIKELY(!siblingItem)) {
        QNDEBUG(QStringLiteral("No sibling item was found"));
        return itemIndex;
    }

    const QString & itemLinkedNotebookGuid = item->linkedNotebookGuid();
    const QString & siblingItemLinkedNotebookGuid = siblingItem->linkedNotebookGuid();
    if ((parentItem == m_fakeRootItem) && (siblingItemLinkedNotebookGuid != itemLinkedNotebookGuid))
    {
        QNLocalizedString error;
        if (itemLinkedNotebookGuid.isEmpty() != siblingItemLinkedNotebookGuid.isEmpty()) {
            error = QT_TR_NOOP("Can't mix tags from linked notebooks with tags from the current account");
        }
        else {
            error = QT_TR_NOOP("Can't mix tags from different linked notebooks");
        }

        QNDEBUG(error << QStringLiteral(", item attempted to be demoted: ") << *item
                << QStringLiteral("\nSibling item: ") << *siblingItem);
        emit notifyError(error);
        return itemIndex;
    }

    if (!canCreateTagItem(*siblingItem)) {
        QNDEBUG(QStringLiteral("Can't create tags within the sibling tag item"));
        return itemIndex;
    }

    QModelIndex siblingItemIndex = indexForItem(siblingItem);
    if (Q_UNLIKELY(!siblingItemIndex.isValid())) {
        QNDEBUG(QStringLiteral("Can't get the valid index for sibling item"));
        return itemIndex;
    }

    const TagModelItem * takenItem = parentItem->takeChild(row);
    if (Q_UNLIKELY(takenItem != item)) {
        QNWARNING(QStringLiteral("Internal inconsistency in tag model: item to take out from parent doesn't match the original demoted item"));
        parentItem->insertChild(row, takenItem);
        return itemIndex;
    }

    int appropriateRow = rowForNewItem(*siblingItem, *takenItem);
    siblingItem->insertChild(appropriateRow, takenItem);

    QModelIndex newIndex = index(appropriateRow, Columns::Name, siblingItemIndex);
    if (!newIndex.isValid()) {
        QNWARNING(QStringLiteral("Can't get the valid index for the demoted item in tag model"));
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
        QNINFO(QStringLiteral("The deletemoted tag model item was not found in the underlying item which is odd. Adding it there"));
        Q_UNUSED(localUidIndex.insert(copyItem))
    }
    else {
        localUidIndex.replace(it, copyItem);
    }

    emit dataChanged(newIndex, newIndex);
    return newIndex;
}

QModelIndexList TagModel::persistentIndexes() const
{
    return persistentIndexList();
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

QString TagModel::columnName(const TagModel::Columns::type column) const
{
    switch(column)
    {
    case Columns::Name:
        return tr("Name");
    case Columns::Synchronizable:
        return tr("Synchronizable");
    case Columns::Dirty:
        return tr("Changed");
    case Columns::FromLinkedNotebook:
        return tr("From linked notebook");
    case Columns::NumNotesPerTag:
        return tr("Notes per tag");
    default:
        return QString();
    }
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
    QNDEBUG(QStringLiteral("TagModel::mapChildItems"));

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
            QNLocalizedString error = QT_TR_NOOP("can't find parent tag for tag");
            error += QStringLiteral(" \"");
            error += item.name();
            error += QStringLiteral("\"");
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
    QNDEBUG(QStringLiteral("TagModel::removeItemByLocalUid: ") << localUid);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG(QStringLiteral("Can't find item to remove from the tag model"));
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
        QNWARNING(QStringLiteral("Internal error: can't get the row of the child item in parent in TagModel, child item: ")
                  << item << QStringLiteral("\nParent item: ") << *parentItem);
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
        QNWARNING(QStringLiteral("Can't update tag model item's row: can't find its original row within parent: ") << item);
        return;
    }

    Q_UNUSED(parentItem->takeChild(currentItemRow))
    int appropriateRow = rowForNewItem(*parentItem, item);
    parentItem->insertChild(appropriateRow, &item);
    QNTRACE(QStringLiteral("Moved item from row ") << currentItemRow << QStringLiteral(" to row ") << appropriateRow
            << QStringLiteral("; item: ") << item);
}

void TagModel::updatePersistentModelIndices()
{
    QNDEBUG(QStringLiteral("TagModel::updatePersistentModelIndices"));

    // Ensure any persistent model indices would be updated appropriately
    QModelIndexList indices = persistentIndexList();
    for(auto it = indices.begin(), end = indices.end(); it != end; ++it)
    {
        const QModelIndex & index = *it;
        const TagModelItem * item = itemForId(static_cast<IndexId>(index.internalId()));
        QModelIndex replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
}


void TagModel::updateTagInLocalStorage(const TagModelItem & item)
{
    QNDEBUG(QStringLiteral("TagModel::updateTagInLocalStorage: local uid = ") << item.localUid());

    Tag tag;

    auto notYetSavedItemIt = m_tagItemsNotYetInLocalStorageUids.find(item.localUid());
    if (notYetSavedItemIt == m_tagItemsNotYetInLocalStorageUids.end())
    {
        QNDEBUG(QStringLiteral("Updating the tag"));

        const Tag * pCachedTag = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedTag))
        {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))
            Tag dummy;
            dummy.setLocalUid(item.localUid());
            QNDEBUG(QStringLiteral("Emitting the request to find tag: local uid = ") << item.localUid()
                    << QStringLiteral(", request id = ") << requestId);
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

        QNTRACE(QStringLiteral("Emitting the request to add the tag to local storage: id = ") << requestId
                << QStringLiteral(", tag: ") << tag);
        emit addTag(tag, requestId);

        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));

        QNTRACE(QStringLiteral("Emitting the request to update the tag in the local storage: id = ") << requestId
                << QStringLiteral(", tag: ") << tag);
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
