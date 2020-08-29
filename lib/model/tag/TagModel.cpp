/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "AllTagsRootItem.h"
#include "InvisibleRootItem.h"
#include "NewItemNameGenerator.hpp"

#include <quentier/logging/QuentierLogger.h>

#include <QByteArray>
#include <QDataStream>
#include <QMimeData>

#include <algorithm>
#include <limits>
#include <vector>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT             (100)
#define LINKED_NOTEBOOK_LIST_LIMIT (40)

#define NUM_TAG_MODEL_COLUMNS (5)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription(error);                                       \
    QNWARNING("model:tag", errorDescription << "" __VA_ARGS__);                \
    Q_EMIT notifyError(errorDescription)

#define REPORT_INFO(info, ...)                                                 \
    ErrorString errorDescription(info);                                        \
    QNINFO("model:tag", errorDescription << "" __VA_ARGS__);                   \
    Q_EMIT notifyError(errorDescription)

namespace quentier {

TagModel::TagModel(
    const Account & account,
    LocalStorageManagerAsync & localStorageManagerAsync, TagCache & cache,
    QObject * parent) :
    ItemModel(account, parent),
    m_cache(cache)
{
    createConnections(localStorageManagerAsync);

    requestTagsList();
    requestLinkedNotebooksList();
}

TagModel::~TagModel()
{
    delete m_pAllTagsRootItem;
    delete m_pInvisibleRootItem;
}

bool TagModel::allTagsListed() const
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
}

void TagModel::favoriteTag(const QModelIndex & index)
{
    QNDEBUG(
        "model:tag",
        "TagModel::favoriteTag: index: is valid = "
            << (index.isValid() ? "true" : "false") << ", row = " << index.row()
            << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setTagFavorited(index, true);
}

void TagModel::unfavoriteTag(const QModelIndex & index)
{
    QNDEBUG(
        "model:tag",
        "TagModel::unfavoriteTag: index: is valid = "
            << (index.isValid() ? "true" : "false") << ", row = " << index.row()
            << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setTagFavorited(index, false);
}

bool TagModel::tagHasSynchronizedChildTags(const QString & tagLocalUid) const
{
    QNTRACE(
        "model:tag",
        "TagModel::tagHasSynchronizedChildTags: tag "
            << "local uid = " << tagLocalUid);

    const auto & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
    auto range = parentLocalUidIndex.equal_range(tagLocalUid);

    // Breadth-first search: first check each immediate child's guid
    for (auto it = range.first; it != range.second; ++it) {
        if (!it->guid().isEmpty()) {
            return true;
        }
    }

    // Now check each child's own child tags
    for (auto it = range.first; it != range.second; ++it) {
        if (tagHasSynchronizedChildTags(it->localUid())) {
            return true;
        }
    }

    return false;
}

QString TagModel::localUidForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:tag",
        "TagModel::localUidForItemName: name = "
            << itemName << ", linked notebook guid = " << linkedNotebookGuid);

    QModelIndex index = indexForTagName(itemName, linkedNotebookGuid);
    const auto * pItem = itemForIndex(index);
    if (!pItem) {
        QNTRACE("model:tag", "No tag with such name was found");
        return {};
    }

    const auto * pTagItem = pItem->cast<TagItem>();
    if (!pTagItem) {
        QNTRACE("model:tag", "Tag model item is not of tag type");
        return {};
    }

    return pTagItem->localUid();
}

QModelIndex TagModel::indexForLocalUid(const QString & localUid) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (it == localUidIndex.end()) {
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString TagModel::itemNameForLocalUid(const QString & localUid) const
{
    QNTRACE("model:tag", "TagModel::itemNameForLocalUid: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:tag", "No tag item with such local uid");
        return {};
    }

    return it->name();
}

ItemModel::ItemInfo TagModel::itemInfoForLocalUid(
    const QString & localUid) const
{
    QNTRACE("model:tag", "TagModel::itemInfoForLocalUid: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE("model:tag", "No tag item with such local uid");
        return {};
    }

    ItemModel::ItemInfo info;
    info.m_localUid = it->localUid();
    info.m_name = it->name();
    info.m_linkedNotebookGuid = it->linkedNotebookGuid();

    info.m_linkedNotebookUsername =
        linkedNotebookUsername(info.m_linkedNotebookGuid);

    return info;
}

QStringList TagModel::itemNames(const QString & linkedNotebookGuid) const
{
    return tagNames(linkedNotebookGuid);
}

QVector<ItemModel::LinkedNotebookInfo> TagModel::linkedNotebooksInfo() const
{
    QVector<LinkedNotebookInfo> infos;
    infos.reserve(m_linkedNotebookItems.size());

    for (const auto & it: qevercloud::toRange(m_linkedNotebookItems)) {
        infos.push_back(LinkedNotebookInfo(it.key(), it.value().username()));
    }

    return infos;
}

QString TagModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it != m_linkedNotebookItems.end()) {
        const auto & item = it.value();
        return item.username();
    }

    return {};
}

bool TagModel::allItemsListed() const
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
}

QModelIndex TagModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_pAllTagsRootItem)) {
        return {};
    }

    return indexForItem(m_pAllTagsRootItem);
}

QString TagModel::localUidForItemIndex(
    const QModelIndex & index) const
{
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    auto * pTagItem = pModelItem->cast<TagItem>();
    if (pTagItem) {
        return pTagItem->localUid();
    }

    return {};
}

QString TagModel::linkedNotebookGuidForItemIndex(
    const QModelIndex & index) const
{
    auto * pModelItem = itemForIndex(index);
    if (!pModelItem) {
        return {};
    }

    auto * pLinkedNotebookItem = pModelItem->cast<TagLinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        return pLinkedNotebookItem->linkedNotebookGuid();
    }

    return {};
}

Qt::ItemFlags TagModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;
    indexFlags |= Qt::ItemIsDragEnabled;
    indexFlags |= Qt::ItemIsDropEnabled;

    if ((index.column() == static_cast<int>(Column::Dirty)) ||
        (index.column() == static_cast<int>(Column::FromLinkedNotebook)))
    {
        return indexFlags;
    }

    const auto * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        return indexFlags;
    }

    const auto * pTagItem = pItem->cast<TagItem>();
    if (!pTagItem) {
        return indexFlags;
    }

    if (!canUpdateTagItem(*pTagItem)) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Synchronizable)) {
        QModelIndex parentIndex = index;

        while (true) {
            const auto * pParentItem = itemForIndex(parentIndex);
            if (Q_UNLIKELY(!pParentItem)) {
                break;
            }

            if ((pParentItem == m_pAllTagsRootItem) ||
                (pParentItem == m_pInvisibleRootItem))
            {
                break;
            }

            const auto * pParentTagItem = pParentItem->cast<TagItem>();
            if (!pParentTagItem) {
                return indexFlags;
            }

            if (pParentTagItem->isSynchronizable()) {
                return indexFlags;
            }

            if (!canUpdateTagItem(*pParentTagItem)) {
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
        return {};
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_TAG_MODEL_COLUMNS)) {
        return {};
    }

    const auto * pItem = itemForIndex(index);
    if (!pItem) {
        return {};
    }

    if (pItem == m_pInvisibleRootItem) {
        return {};
    }

    Column column;
    switch (static_cast<Column>(columnIndex)) {
    case Column::Name:
        column = Column::Name;
        break;
    case Column::Synchronizable:
        column = Column::Synchronizable;
        break;
    case Column::Dirty:
        column = Column::Dirty;
        break;
    case Column::FromLinkedNotebook:
        column = Column::FromLinkedNotebook;
        break;
    case Column::NoteCount:
        column = Column::NoteCount;
        break;
    default:
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataImpl(*pItem, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*pItem, column);
    default:
        return {};
    }
}

QVariant TagModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    return columnName(static_cast<Column>(section));
}

int TagModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    const auto * pParentItem = itemForIndex(parent);
    return (pParentItem ? pParentItem->childrenCount() : 0);
}

int TagModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    return NUM_TAG_MODEL_COLUMNS;
}

QModelIndex TagModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if ((row < 0) || (column < 0) || (column >= NUM_TAG_MODEL_COLUMNS) ||
        (parent.isValid() &&
         (parent.column() != static_cast<int>(Column::Name))))
    {
        return {};
    }

    const auto * pParentItem = itemForIndex(parent);
    if (!pParentItem) {
        return {};
    }

    const auto * pItem = pParentItem->childAtRow(row);
    if (!pItem) {
        return {};
    }

    IndexId id = idForItem(*pItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, column, id);
}

QModelIndex TagModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto * pChildItem = itemForIndex(index);
    if (!pChildItem) {
        return {};
    }

    const auto * pParentItem = pChildItem->parent();
    if (!pParentItem) {
        return {};
    }

    if (pParentItem == m_pInvisibleRootItem) {
        return {};
    }

    if (pParentItem == m_pAllTagsRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allTagsRootItemIndexId);
    }

    const auto * pGrandParentItem = pParentItem->parent();
    if (!pGrandParentItem) {
        return {};
    }

    int row = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:tag",
            "Internal inconsistency detected in TagModel: "
                << "parent of the item can't find the item "
                << "within its children: item = " << *pParentItem
                << "\nParent item: " << *pGrandParentItem);
        return {};
    }

    IndexId id = idForItem(*pParentItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

bool TagModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool TagModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNTRACE(
        "model:tag",
        "TagModel::setData: row = "
            << modelIndex.row() << ", column = " << modelIndex.column()
            << ", internal id = " << modelIndex.internalId()
            << ", value = " << value << ", role = " << role);

    if (role != Qt::EditRole) {
        QNDEBUG("model:tag", "Non-edit role, skipping");
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG("model:tag", "The model index is invalid, skipping");
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::Dirty)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"dirty\" flag can't be set manually for a tag"));
        return false;
    }

    if (modelIndex.column() == static_cast<int>(Column::FromLinkedNotebook)) {
        REPORT_ERROR(
            QT_TR_NOOP("The \"from linked notebook\" flag can't be set "
                       "manually for a tag"));
        return false;
    }

    auto * pItem = itemForIndex(modelIndex);
    if (!pItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: no tag model item found for "
                       "model index"));
        return false;
    }

    if (Q_UNLIKELY(pItem == m_pInvisibleRootItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the invisible root item "
                       "within the tag model"));
        return false;
    }

    auto * pTagItem = pItem->cast<TagItem>();
    if (!pTagItem) {
        QNDEBUG("model:tag", "The model index points to a non-tag item");
        return false;
    }

    if (!canUpdateTagItem(*pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't update the tag, restrictions apply"));
        return false;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();

    bool shouldMakeParentsSynchronizable = false;

    TagItem tagItemCopy = *pTagItem;
    bool dirty = tagItemCopy.isDirty();
    switch (static_cast<Column>(modelIndex.column())) {
    case Column::Name:
    {
        QString newName = value.toString().trimmed();
        bool changed = (newName != tagItemCopy.name());
        if (!changed) {
            QNDEBUG("model:tag", "Tag name hasn't changed");
            return true;
        }

        auto nameIt = nameIndex.find(newName.toUpper());
        if (nameIt != nameIndex.end()) {
            ErrorString error(
                QT_TR_NOOP("Can't change tag name: no two tags within "
                           "the account are allowed to have the same name in "
                           "a case-insensitive manner"));
            QNINFO("model:tag", error << ", suggested name = " << newName);
            Q_EMIT notifyError(error);
            return false;
        }

        ErrorString errorDescription;
        if (!Tag::validateName(newName, &errorDescription)) {
            ErrorString error(QT_TR_NOOP("Can't change tag name"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNINFO("model:tag", error << "; suggested name = " << newName);
            Q_EMIT notifyError(error);
            return false;
        }

        dirty = true;
        tagItemCopy.setName(newName);
        break;
    }
    case Column::Synchronizable:
    {
        if (m_account.type() == Account::Type::Local) {
            ErrorString error(
                QT_TR_NOOP("Can't make tag synchronizable within a local "
                           "account"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (tagItemCopy.isSynchronizable() && !value.toBool()) {
            ErrorString error(
                QT_TR_NOOP("Can't make an already synchronizable "
                           "tag not synchronizable"));
            QNINFO(
                "model:tag",
                error << ", already synchronizable tag item: " << tagItemCopy);
            Q_EMIT notifyError(error);
            return false;
        }

        dirty |= (value.toBool() != tagItemCopy.isSynchronizable());
        tagItemCopy.setSynchronizable(value.toBool());
        shouldMakeParentsSynchronizable = true;
        break;
    }
    default:
        QNINFO(
            "model:tag",
            "Can't edit data for column " << modelIndex.column()
                                          << " in the tag model");
        return false;
    }

    tagItemCopy.setDirty(dirty);

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();

    if (shouldMakeParentsSynchronizable) {
        QNDEBUG(
            "model:tag",
            "Making the parents of the tag made "
                << "synchronizable also synchronizable");

        auto * pProcessedItem = pItem;
        TagItem dummy;
        while (pProcessedItem->parent()) {
            auto * pParentItem = pProcessedItem->parent();
            if (pParentItem == m_pInvisibleRootItem) {
                break;
            }

            if (pParentItem == m_pAllTagsRootItem) {
                break;
            }

            const TagItem * pParentTagItem = pParentItem->cast<TagItem>();
            if (!pParentTagItem) {
                break;
            }

            if (pParentTagItem->isSynchronizable()) {
                break;
            }

            dummy = *pParentTagItem;
            dummy.setSynchronizable(true);
            auto dummyIt = index.find(dummy.localUid());
            if (Q_UNLIKELY(dummyIt == index.end())) {
                ErrorString error(
                    QT_TR_NOOP("Can't find one of currently made "
                               "synchronizable tag's parent tags"));
                QNWARNING("model:tag", error << ", item: " << dummy);
                Q_EMIT notifyError(error);
                return false;
            }

            index.replace(dummyIt, dummy);
            QModelIndex changedIndex = indexForLocalUid(dummy.localUid());
            if (Q_UNLIKELY(!changedIndex.isValid())) {
                ErrorString error(
                    QT_TR_NOOP("Can't get valid model index for one of "
                               "currently made synchronizable tag's parent "
                               "tags"));
                QNWARNING(
                    "model:tag",
                    error << ", item for which the index "
                          << "was requested: " << dummy);
                Q_EMIT notifyError(error);
                return false;
            }

            changedIndex = this->index(
                changedIndex.row(), static_cast<int>(Column::Synchronizable),
                changedIndex.parent());

            Q_EMIT dataChanged(changedIndex, changedIndex);
            pProcessedItem = pParentItem;
        }
    }

    auto it = index.find(tagItemCopy.localUid());
    if (Q_UNLIKELY(it == index.end())) {
        ErrorString error(QT_TR_NOOP("Can't find the tag being modified"));
        QNWARNING(
            "model:tag", error << " by its local uid , item: " << tagItemCopy);
        Q_EMIT notifyError(error);
        return false;
    }

    index.replace(it, tagItemCopy);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    if (m_sortedColumn == Column::Name) {
        updateItemRowWithRespectToSorting(*pItem);
    }

    updateTagInLocalStorage(tagItemCopy);

    QNDEBUG("model:tag", "Successfully set the data");
    return true;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model:tag",
        "TagModel::insertRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    checkAndCreateModelRootItems();

    ITagModelItem * pParentItem =
        (parent.isValid() ? itemForIndex(parent) : m_pInvisibleRootItem);

    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(
            "model:tag",
            "Can't insert row into the tag model: can't "
                << "find parent item per model index");
        return false;
    }

    if ((pParentItem != m_pAllTagsRootItem) && !canCreateTagItem(*pParentItem))
    {
        QNINFO(
            "model:tag",
            "Can't insert row into under the tag model item: "
                << "restrictions apply: " << *pParentItem);
        return false;
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingTags = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingTags + count >= m_account.tagCountMax())) {
        ErrorString error(
            QT_TR_NOOP("Can't create tag(s): the account can "
                       "contain a limited number of tags"));
        error.details() = QString::number(m_account.tagCountMax());
        QNINFO("model:tag", error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<TagDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Adding tag item
        TagItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewTag(QString()));
        item.setDirty(true);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    if (m_sortedColumn == Column::Name) {
        Q_EMIT layoutAboutToBeChanged();

        for (const auto & it: addedItems) {
            auto tagModelItemIt = localUidIndex.find(it->localUid());
            if (tagModelItemIt != localUidIndex.end()) {
                updateItemRowWithRespectToSorting(const_cast<TagItem &>(*it));
            }
        }

        Q_EMIT layoutChanged();
    }

    for (const auto & it: addedItems) {
        updateTagInLocalStorage(*it);
    }

    QNDEBUG("model:tag", "Successfully inserted the rows");
    return true;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model:tag",
        "TagModel::removeRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    RemoveRowsScopeGuard removeRowsScopeGuard(*this);
    Q_UNUSED(removeRowsScopeGuard)

    checkAndCreateModelRootItems();

    auto * pParentItem =
        (parent.isValid() ? itemForIndex(parent) : m_pInvisibleRootItem);

    if (!pParentItem) {
        QNDEBUG("model:tag", "No item corresponding to parent index");
        return false;
    }

    /**
     * First need to check if the rows to be removed are allowed to be removed
     */
    for (int i = 0; i < count; ++i) {
        auto * pModelItem = pParentItem->childAtRow(row + i);
        if (!pModelItem) {
            QNWARNING(
                "model:tag",
                "Detected null pointer to child tag item "
                    << "on attempt to remove row " << (row + i)
                    << " from parent item: " << *pParentItem);
            continue;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (!pTagItem) {
            ErrorString error(
                QT_TR_NOOP("Can't remove a non-tag item from tag model"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (!pTagItem->linkedNotebookGuid().isEmpty()) {
            ErrorString error(
                QT_TR_NOOP("Can't remove tag from a linked notebook"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (!pTagItem->guid().isEmpty()) {
            ErrorString error(
                QT_TR_NOOP("Can't remove tag already synchronized with "
                           "Evernote"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (tagHasSynchronizedChildTags(pTagItem->localUid())) {
            ErrorString error(
                QT_TR_NOOP("Can't remove tag which has some child "
                           "tags already synchronized with Evernote"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();

    /**
     * Need to re-parent all children of each removed item to the parent of
     * the removed items i.e. to make the grand-parent of each child its new
     * parent. But before that will just take them away from the current parent
     * and ollect into a temporary list
     */
    QList<ITagModelItem *> removedItemsChildren;
    for (int i = 0; i < count; ++i) {
        auto * pModelItem = pParentItem->childAtRow(row + i);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(
                "model:tag",
                "Detected null pointer to tag model item "
                    << "within the items to be removed");
            continue;
        }

        auto modelItemIndex = indexForItem(pModelItem);
        while (pModelItem->hasChildren()) {
            beginRemoveRows(modelItemIndex, 0, 0);
            auto * pChildItem = pModelItem->takeChild(0);
            endRemoveRows();

            if (Q_UNLIKELY(!pChildItem)) {
                continue;
            }

            const auto * pChildTagItem = pChildItem->cast<TagItem>();
            if (Q_UNLIKELY(!pChildTagItem)) {
                continue;
            }

            TagItem childItemCopy(*pChildTagItem);

            auto * pParentTagItem = pParentItem->cast<TagItem>();
            if (pParentTagItem) {
                childItemCopy.setParentGuid(pParentTagItem->guid());
                childItemCopy.setParentLocalUid(pParentTagItem->localUid());
            }
            else {
                childItemCopy.setParentGuid(QString());
                childItemCopy.setParentLocalUid(QString());
            }

            childItemCopy.setDirty(true);

            auto tagItemIt = localUidIndex.find(childItemCopy.localUid());
            if (Q_UNLIKELY(tagItemIt == localUidIndex.end())) {
                QNINFO(
                    "model:tag",
                    "The tag item which parent is being "
                        << "removed was not found within the model. Adding it "
                        << "there");
                Q_UNUSED(localUidIndex.insert(childItemCopy))
            }
            else {
                localUidIndex.replace(tagItemIt, childItemCopy);
            }

            updateTagInLocalStorage(childItemCopy);

            /**
             * NOTE: no dataChanged signal here because the corresponding model
             * item is now parentless and hence is unavailable to the view
             */

            removedItemsChildren << pChildItem;
        }
    }

    /**
     * Actually remove the rows each of which has no children anymore
     */
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        auto * pModelItem = pParentItem->takeChild(row);
        if (Q_UNLIKELY(!pModelItem)) {
            continue;
        }

        TagItem * pTagItem = pModelItem->cast<TagItem>();
        if (Q_UNLIKELY(!pTagItem)) {
            continue;
        }

        Tag tag;
        tag.setLocalUid(pTagItem->localUid());

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeTagRequestIds.insert(requestId))
        Q_EMIT expungeTag(tag, requestId);

        QNTRACE(
            "model:tag",
            "Emitted the request to expunge the tag from "
                << "the local storage: request id = " << requestId
                << ", tag local uid: " << pTagItem->localUid());

        auto it = localUidIndex.find(pTagItem->localUid());
        if (it != localUidIndex.end()) {
            Q_UNUSED(localUidIndex.erase(it))
        }

        auto indexIt = m_indexIdToLocalUidBimap.right.find(tag.localUid());
        if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
            Q_UNUSED(m_indexIdToLocalUidBimap.right.erase(indexIt))
        }
    }
    endRemoveRows();

    /**
     * Insert previously collected children of the removed item under its parent
     */
    while (!removedItemsChildren.isEmpty()) {
        auto * pChildItem = removedItemsChildren.takeAt(0);
        if (Q_UNLIKELY(!pChildItem)) {
            continue;
        }

        int newRow = rowForNewItem(*pParentItem, *pChildItem);
        beginInsertRows(parent, newRow, newRow);
        pParentItem->insertChild(newRow, pChildItem);
        endInsertRows();
    }

    QNDEBUG("model:tag", "Successfully removed row(s)");
    return true;
}

void TagModel::sort(int column, Qt::SortOrder order)
{
    QNTRACE(
        "model:tag",
        "TagModel::sort: column = "
            << column << ", order = " << order << " ("
            << (order == Qt::AscendingOrder ? "ascending" : "descending")
            << ")");

    if (column != static_cast<int>(Column::Name)) {
        // Sorting by other columns is not yet implemented
        return;
    }

    if (order == m_sortOrder) {
        QNDEBUG(
            "model:tag",
            "The sort order already established, nothing to "
                << "do");
        return;
    }

    m_sortOrder = order;
    Q_EMIT sortingChanged();

    checkAndCreateModelRootItems();

    Q_EMIT layoutAboutToBeChanged();

    if (m_sortOrder == Qt::AscendingOrder) {
        auto & localUidIndex = m_data.get<ByLocalUid>();
        for (auto & item: localUidIndex) {
            const_cast<TagItem &>(item).sortChildren(LessByName());
        }

        for (auto it = m_linkedNotebookItems.begin();
             it != m_linkedNotebookItems.end(); ++it)
        {
            it.value().sortChildren(LessByName());
        }

        m_pAllTagsRootItem->sortChildren(LessByName());
    }
    else {
        auto & localUidIndex = m_data.get<ByLocalUid>();
        for (auto & item: localUidIndex) {
            const_cast<TagItem &>(item).sortChildren(GreaterByName());
        }

        for (auto it = m_linkedNotebookItems.begin();
             it != m_linkedNotebookItems.end(); ++it)
        {
            it.value().sortChildren(GreaterByName());
        }

        m_pAllTagsRootItem->sortChildren(GreaterByName());
    }

    updatePersistentModelIndices();
    Q_EMIT layoutChanged();

    QNDEBUG("model:tag", "Successfully sorted the tag model");
}

QStringList TagModel::mimeTypes() const
{
    QStringList list;
    list << TAG_MODEL_MIME_TYPE;
    return list;
}

QMimeData * TagModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    const auto * pModelItem = itemForIndex(indexes.at(0));
    if (!pModelItem) {
        return nullptr;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (!pTagItem) {
        return nullptr;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *pTagItem;

    auto * pMimeData = new QMimeData;
    pMimeData->setData(
        TAG_MODEL_MIME_TYPE,
        qCompress(encodedItem, TAG_MODEL_MIME_DATA_MAX_COMPRESSION));

    return pMimeData;
}

bool TagModel::dropMimeData(
    const QMimeData * pMimeData, Qt::DropAction action, int row, int column,
    const QModelIndex & parentIndex)
{
    QNTRACE(
        "model:tag",
        "TagModel::dropMimeData: action = "
            << action << ", row = " << row << ", column = " << column
            << ", parent index: is valid = "
            << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row()
            << ", parent column = " << (parentIndex.column())
            << ", parent internal id: " << parentIndex.internalId()
            << ", mime data formats: "
            << (pMimeData ? pMimeData->formats().join(QStringLiteral("; "))
                          : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!pMimeData || !pMimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        return false;
    }

    if (!parentIndex.isValid()) {
        // Invalid index corresponds to invisible root item and it's forbidden
        // to drop items under it. Tags without parent tags which don't belong
        // to linked notebooks need to be dropped under all tags root item.
        return false;
    }

    checkAndCreateModelRootItems();

    auto * pNewParentItem = itemForIndex(parentIndex);
    if (!pNewParentItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error, can't drop tag: no new parent "
                       "item was found"));
        return false;
    }

    if (pNewParentItem != m_pAllTagsRootItem &&
        !canCreateTagItem(*pNewParentItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move tag under the new parent: restrictions "
                       "apply or restrictions settings were not fetched yet"));
        return false;
    }

    QByteArray data = qUncompress(pMimeData->data(TAG_MODEL_MIME_TYPE));
    QDataStream in(&data, QIODevice::ReadOnly);

    qint32 type;
    in >> type;

    if (type != static_cast<qint32>(ITagModelItem::Type::Tag)) {
        QNDEBUG("model:tag", "Can only drag-drop tag model items of tag type");
        return false;
    }

    TagItem tagItem;
    in >> tagItem;

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(tagItem.localUid());
    if (it == localUidIndex.end()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find the notebook being "
                       "dropped in the notebook model"));
        return false;
    }

    auto * pTagItem = const_cast<TagItem *>(&(*it));

    if (!canUpdateTagItem(*pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Cannot reparent tag, restrictions apply"));
        return false;
    }

    QString parentLinkedNotebookGuid;

    auto * pParentLinkedNotebookItem =
        pNewParentItem->cast<TagLinkedNotebookRootItem>();
    if (pParentLinkedNotebookItem) {
        parentLinkedNotebookGuid =
            pParentLinkedNotebookItem->linkedNotebookGuid();
    }

    if (pTagItem->linkedNotebookGuid() != parentLinkedNotebookGuid) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move tags between different linked notebooks or "
                       "between user's tags and those from linked notebooks"));
        return false;
    }

    // Check that we aren't trying to move the tag under one of its children
    const auto * pTrackedParentItem = pNewParentItem;
    while (pTrackedParentItem && (pTrackedParentItem != m_pAllTagsRootItem)) {
        const auto * pTrackedParentTagItem =
            pTrackedParentItem->cast<TagItem>();

        if (pTrackedParentTagItem &&
            (pTrackedParentTagItem->localUid() != pTagItem->localUid()))
        {
            ErrorString error(
                QT_TR_NOOP("Can't move tag under one of its child tags"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return false;
        }

        pTrackedParentItem = pTrackedParentItem->parent();
    }

    if (pNewParentItem == pTagItem->parent()) {
        QNDEBUG(
            "model:tag",
            "Item is already under the chosen parent, nothing "
                << "to do");
        return true;
    }

    tagItem = *pTagItem;

    const auto * pNewParentTagItem = pNewParentItem->cast<TagItem>();
    if (pNewParentTagItem) {
        tagItem.setParentLocalUid(pNewParentTagItem->localUid());
        tagItem.setParentGuid(pNewParentTagItem->guid());
    }
    else {
        tagItem.setParentLocalUid(QString());
        tagItem.setParentGuid(QString());
    }

    tagItem.setDirty(true);

    if (row == -1) {
        row = parentIndex.row();
    }

    // Remove the tag model item from its original parent
    auto * pOriginalParentItem = pTagItem->parent();
    int originalRow = pOriginalParentItem->rowForChild(pTagItem);
    if (originalRow >= 0) {
        auto originalParentIndex = indexForItem(pOriginalParentItem);
        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(pOriginalParentItem->takeChild(originalRow));
        endRemoveRows();
        checkAndRemoveEmptyLinkedNotebookRootItem(*pOriginalParentItem);
    }

    beginInsertRows(parentIndex, row, row);
    localUidIndex.replace(it, tagItem);
    pNewParentItem->insertChild(row, pTagItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*pTagItem);
    updateTagInLocalStorage(*pTagItem);

    QModelIndex index = indexForLocalUid(tagItem.localUid());
    Q_EMIT notifyTagParentChanged(index);
    return true;
}

void TagModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onAddTagComplete: tag = " << tag
                                             << "\nRequest id = " << requestId);

    auto it = m_addTagRequestIds.find(requestId);
    if (it != m_addTagRequestIds.end()) {
        Q_UNUSED(m_addTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);
    requestNoteCountForTag(tag);
}

void TagModel::onAddTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addTagRequestIds.find(requestId);
    if (it == m_addTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onAddTagFailed: tag = " << tag << "\nError description = "
                                           << errorDescription
                                           << ", request id = " << requestId);

    Q_UNUSED(m_addTagRequestIds.erase(it))
    Q_EMIT notifyError(errorDescription);
    removeItemByLocalUid(tag.localUid());
}

void TagModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onUpdateTagComplete: tag = " << tag << "\nRequest id = "
                                                << requestId);

    auto it = m_updateTagRequestIds.find(requestId);
    if (it != m_updateTagRequestIds.end()) {
        Q_UNUSED(m_updateTagRequestIds.erase(it))
        return;
    }

    onTagAddedOrUpdated(tag);

    // NOTE: no need to re-request the number of notes per this tag -
    // the update of the tag itself doesn't change
    // anything about which notes use the tag
}

void TagModel::onUpdateTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onUpdateTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))
    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model:tag",
        "Emitting the request to find a tag: local uid = "
            << tag.localUid() << ", request id = " << requestId);

    Q_EMIT findTag(tag, requestId);
}

void TagModel::onFindTagComplete(Tag tag, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);

    auto checkAfterErasureIt =
        m_findTagAfterNotelessTagsErasureRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (checkAfterErasureIt ==
         m_findTagAfterNotelessTagsErasureRequestIds.end()))
    {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onFindTagComplete: tag = " << tag << "\nRequest id = "
                                              << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))

        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(tag.localUid(), tag);
        auto & localUidIndex = m_data.get<ByLocalUid>();
        auto it = localUidIndex.find(tag.localUid());
        if (it != localUidIndex.end()) {
            updateTagInLocalStorage(*it);
        }
    }
    else if (
        checkAfterErasureIt !=
        m_findTagAfterNotelessTagsErasureRequestIds.end())
    {
        QNDEBUG(
            "model:tag",
            "Tag still exists after expunging the noteless "
                << "tags from linked notebooks: " << tag);

        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.erase(
            checkAfterErasureIt))

        onTagAddedOrUpdated(tag);
    }
}

void TagModel::onFindTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto restoreUpdateIt =
        m_findTagToRestoreFailedUpdateRequestIds.find(requestId);

    auto performUpdateIt = m_findTagToPerformUpdateRequestIds.find(requestId);

    auto checkAfterErasureIt =
        m_findTagAfterNotelessTagsErasureRequestIds.find(requestId);

    if ((restoreUpdateIt == m_findTagToRestoreFailedUpdateRequestIds.end()) &&
        (performUpdateIt == m_findTagToPerformUpdateRequestIds.end()) &&
        (checkAfterErasureIt ==
         m_findTagAfterNotelessTagsErasureRequestIds.end()))
    {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onFindTagFailed: tag = " << tag << "\nError description = "
                                            << errorDescription
                                            << ", request id = " << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
    }
    else if (
        checkAfterErasureIt !=
        m_findTagAfterNotelessTagsErasureRequestIds.end())
    {
        QNDEBUG(
            "model:tag",
            "Tag no longer exists after the noteless tags "
                << "from linked notebooks erasure");

        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.erase(
            checkAfterErasureIt))

        removeItemByLocalUid(tag.localUid());
    }

    Q_EMIT notifyError(errorDescription);
}

void TagModel::onListTagsComplete(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, QList<Tag> tags, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onListTagsComplete: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", num found tags = " << tags.size()
            << ", request id = " << requestId);

    for (const auto & tag: qAsConst(tags)) {
        onTagAddedOrUpdated(tag);
    }

    m_listTagsRequestId = QUuid();

    if (!tags.isEmpty()) {
        QNTRACE(
            "model:tag",
            "The number of found tags is greater than zero, "
                << "requesting more tags from the local storage");
        m_listTagsOffset += static_cast<size_t>(tags.size());
        requestTagsList();
        return;
    }

    m_allTagsListed = true;
    requestNoteCountsPerAllTags();

    if (m_allLinkedNotebooksListed) {
        Q_EMIT notifyAllTagsListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void TagModel::onListTagsFailed(
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onListTagsFailed: flag = "
            << flag << ", limit = " << limit << ", offset = " << offset
            << ", order = " << order << ", direction = " << orderDirection
            << ", linked notebook guid = "
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>")
                                            : linkedNotebookGuid)
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listTagsRequestId = QUuid();
    Q_EMIT notifyError(errorDescription);
}

void TagModel::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onExpungeTagComplete: tag = "
            << tag << "\nExpunged child tag local uids: "
            << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    auto it = m_expungeTagRequestIds.find(requestId);
    if (it != m_expungeTagRequestIds.end()) {
        Q_UNUSED(m_expungeTagRequestIds.erase(it))
        return;
    }

    Q_EMIT aboutToRemoveTags();
    // NOTE: all child items would be removed from the model automatically
    removeItemByLocalUid(tag.localUid());
    Q_EMIT removedTags();
}

void TagModel::onExpungeTagFailed(
    Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_expungeTagRequestIds.find(requestId);
    if (it == m_expungeTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onExpungeTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_expungeTagRequestIds.erase(it))

    onTagAddedOrUpdated(tag);
}

void TagModel::onGetNoteCountPerTagComplete(
    int noteCount, Tag tag, LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onGetNoteCountPerTagComplete: tag = "
            << tag << "\nRequest id = " << requestId
            << ", note count = " << noteCount);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))
    setNoteCountForTag(tag.localUid(), noteCount);
}

void TagModel::onGetNoteCountPerTagFailed(
    ErrorString errorDescription, Tag tag,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onGetNoteCountPerTagFailed: "
            << "error description = " << errorDescription << ", tag = " << tag
            << ", request id = " << requestId);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))

    ErrorString error(QT_TR_NOOP("Failed to get note count for one of tags"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void TagModel::onGetNoteCountsPerAllTagsComplete(
    QHash<QString, int> noteCountsPerTagLocalUid,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    if (requestId != m_noteCountsPerAllTagsRequestId) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onGetNoteCountsPerAllTagsComplete: note "
            << "counts were received for " << noteCountsPerTagLocalUid.size()
            << " tag local uids; request id = " << requestId);

    m_noteCountsPerAllTagsRequestId = QUuid();

    auto & localUidIndex = m_data.get<ByLocalUid>();
    for (auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end;
         ++it)
    {
        TagItem item = *it;
        auto noteCountIt = noteCountsPerTagLocalUid.find(item.localUid());
        if (noteCountIt != noteCountsPerTagLocalUid.end()) {
            item.setNoteCount(noteCountIt.value());
        }
        else {
            item.setNoteCount(0);
        }

        localUidIndex.replace(it, item);

        const QString & parentLocalUid = item.parentLocalUid();
        const QString & linkedNotebookGuid = item.linkedNotebookGuid();
        if (parentLocalUid.isEmpty() && linkedNotebookGuid.isEmpty()) {
            continue;
        }

        // If tag item has either parent tag or linked notebook local uid,
        // we'll send dataChanged signal for it here; for all tags from user's
        // own account and without parent tags we'll send dataChanged signal
        // later, once for all such tags
        QModelIndex idx = indexForLocalUid(item.localUid());
        if (idx.isValid()) {
            idx = index(
                idx.row(), static_cast<int>(Column::NoteCount), idx.parent());

            Q_EMIT dataChanged(idx, idx);
        }
    }

    auto allTagsRootItemIndex = indexForItem(m_pAllTagsRootItem);

    QModelIndex startIndex =
        index(0, static_cast<int>(Column::NoteCount), allTagsRootItemIndex);

    QModelIndex endIndex = index(
        rowCount(allTagsRootItemIndex), static_cast<int>(Column::NoteCount),
        allTagsRootItemIndex);

    Q_EMIT dataChanged(startIndex, endIndex);
}

void TagModel::onGetNoteCountsPerAllTagsFailed(
    ErrorString errorDescription, LocalStorageManager::NoteCountOptions options,
    QUuid requestId)
{
    Q_UNUSED(options)

    if (requestId != m_noteCountsPerAllTagsRequestId) {
        return;
    }

    QNDEBUG(
        "model:tag",
        "TagModel::onGetNoteCountsPerAllTagsFailed: error "
            << "description = " << errorDescription
            << ", request id = " << requestId);

    m_noteCountsPerAllTagsRequestId = QUuid();

    ErrorString error(QT_TR_NOOP("Failed to get note counts for tags"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete: "
            << "request id = " << requestId);

    auto & localUidIndex = m_data.get<ByLocalUid>();

    for (const auto & item: localUidIndex) {
        if (item.linkedNotebookGuid().isEmpty()) {
            continue;
        }

        // The item's current note count per tag may be invalid due to
        // asynchronous events sequence, need to ask the database if such
        // an item actually exists
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.insert(requestId))

        Tag tag;
        tag.setLocalUid(item.localUid());

        QNTRACE(
            "model:tag",
            "Emitting the request to find tag from linked "
                << "notebook to check for its existence: " << item.localUid()
                << ", request id = " << requestId);

        Q_EMIT findTag(tag, requestId);
    }
}

void TagModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onFindNotebookComplete: notebook: "
            << notebook << "\nRequest id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))

    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onFindNotebookFailed(
    Notebook notebook, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestForLinkedNotebookGuid.right.find(requestId);
    if (it == m_findNotebookRequestForLinkedNotebookGuid.right.end()) {
        return;
    }

    QNWARNING(
        "model:tag",
        "TagModel::onFindNotebookFailed: notebook = "
            << notebook << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))
}

void TagModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onUpdateNotebookComplete: local uid = "
            << notebook.localUid());

    Q_UNUSED(requestId)
    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onExpungeNotebookComplete: local uid = "
            << notebook.localUid() << ", linked notebook guid = "
            << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid()
                                                 : QStringLiteral("<null>")));

    Q_UNUSED(requestId)

    // Notes from this notebook have been expunged along with it; need to
    // re-request the number of notes per tag for all tags
    requestNoteCountsPerAllTags();

    if (!notebook.hasLinkedNotebookGuid()) {
        return;
    }

    auto it = m_tagRestrictionsByLinkedNotebookGuid.find(
        notebook.linkedNotebookGuid());

    if (it == m_tagRestrictionsByLinkedNotebookGuid.end()) {
        Restrictions restrictions;
        restrictions.m_canCreateTags = false;
        restrictions.m_canUpdateTags = false;
        m_tagRestrictionsByLinkedNotebookGuid[notebook.linkedNotebookGuid()] =
            restrictions;
        return;
    }

    it->m_canCreateTags = false;
    it->m_canUpdateTags = false;
}

void TagModel::onAddNoteComplete(Note note, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onAddNoteComplete: note = " << note << "\nRequest id = "
                                               << requestId);

    if (Q_UNLIKELY(note.hasDeletionTimestamp())) {
        return;
    }

    if (!note.hasTagLocalUids()) {
        if (note.hasTagGuids()) {
            QNDEBUG(
                "model:tag",
                "The note has tag guids but not tag local "
                    << "uids, need to request the proper list of tags from "
                       "this "
                    << "note before their note counts can be updated");
            requestTagsPerNote(note);
        }
        else {
            QNDEBUG(
                "model:tag",
                "The note has no tags => no need to update "
                    << "the note count per any tag");
        }

        return;
    }

    const auto & tagLocalUids = note.tagLocalUids();
    for (const auto & tagLocalUid: qAsConst(tagLocalUids)) {
        Tag dummy;
        dummy.setLocalUid(tagLocalUid);
        requestNoteCountForTag(dummy);
    }
}

void TagModel::onNoteTagListChanged(
    QString noteLocalUid, QStringList previousNoteTagLocalUids,
    QStringList newNoteTagLocalUids)
{
    QNDEBUG(
        "model:tag",
        "TagModel::onNoteTagListChanged: note local uid = "
            << noteLocalUid << ", previous note tag local uids = "
            << previousNoteTagLocalUids.join(QStringLiteral(","))
            << ", new note tag local uids = "
            << newNoteTagLocalUids.join(QStringLiteral(",")));

    std::sort(previousNoteTagLocalUids.begin(), previousNoteTagLocalUids.end());
    std::sort(newNoteTagLocalUids.begin(), newNoteTagLocalUids.end());

    std::vector<QString> commonTagLocalUids;

    std::set_intersection(
        previousNoteTagLocalUids.begin(), previousNoteTagLocalUids.end(),
        newNoteTagLocalUids.begin(), newNoteTagLocalUids.end(),
        std::back_inserter(commonTagLocalUids));

    auto & localUidIndex = m_data.get<ByLocalUid>();

    for (const auto & tagLocalUid: qAsConst(previousNoteTagLocalUids)) {
        auto commonIt = std::find(
            commonTagLocalUids.begin(), commonTagLocalUids.end(), tagLocalUid);

        if (commonIt != commonTagLocalUids.end()) {
            continue;
        }

        auto itemIt = localUidIndex.find(tagLocalUid);
        if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
            // Probably this tag was expunged
            QNDEBUG(
                "model:tag", "No tag was found in the model: " << tagLocalUid);
            continue;
        }

        int noteCount = itemIt->noteCount();
        --noteCount;
        noteCount = std::max(0, noteCount);
        setNoteCountForTag(tagLocalUid, noteCount);
    }

    for (const auto & tagLocalUid: qAsConst(newNoteTagLocalUids)) {
        auto commonIt = std::find(
            commonTagLocalUids.begin(), commonTagLocalUids.end(), tagLocalUid);

        if (commonIt != commonTagLocalUids.end()) {
            continue;
        }

        auto itemIt = localUidIndex.find(tagLocalUid);
        if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
            // Probably this tag was expunged
            QNDEBUG(
                "model:tag", "No tag was found in the model: " << tagLocalUid);
            continue;
        }

        int noteCount = itemIt->noteCount();
        ++noteCount;
        setNoteCountForTag(tagLocalUid, noteCount);
    }
}

void TagModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onExpungeNoteComplete: note = " << note << "\nRequest id = "
                                                   << requestId);

    if (note.hasTagLocalUids()) {
        const auto & tagLocalUids = note.tagLocalUids();
        for (const auto & tagLocalUid: qAsConst(tagLocalUids)) {
            Tag tag;
            tag.setLocalUid(tagLocalUid);
            requestNoteCountForTag(tag);
        }

        return;
    }

    QNDEBUG("model:tag", "Note has no tag local uids");
    requestNoteCountsPerAllTags();
}

void TagModel::onAddLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onAddLinkedNotebookComplete: request id = "
            << requestId << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onUpdateLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onUpdateLinkedNotebookComplete: request id = "
            << requestId << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onExpungeLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model:tag",
        "TagModel::onExpungeLinkedNotebookComplete: request "
            << "id = " << requestId << ", linked notebook: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model:tag",
            "Received linked notebook expunged event but "
                << "the linked notebook has no guid: " << linkedNotebook
                << ", request id = " << requestId);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    QStringList expungedTagLocalUids;
    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);

    expungedTagLocalUids.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedTagLocalUids << it->localUid();
    }

    for (const auto & tagLocalUid: qAsConst(expungedTagLocalUids)) {
        removeItemByLocalUid(tagLocalUid);
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        auto * pModelItem = &(linkedNotebookItemIt.value());
        auto * pParentItem = pModelItem->parent();
        if (pParentItem) {
            int row = pParentItem->rowForChild(pModelItem);
            if (row >= 0) {
                QModelIndex parentItemIndex = indexForItem(pParentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(pParentItem->takeChild(row))
                endRemoveRows();
            }
        }

        Q_UNUSED(m_linkedNotebookItems.erase(linkedNotebookItemIt))
    }

    auto indexIt =
        m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);

    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }
}

void TagModel::onListAllTagsPerNoteComplete(
    QList<Tag> foundTags, Note note,
    LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
    LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection, QUuid requestId)
{
    auto it = m_listTagsPerNoteRequestIds.find(requestId);
    if (it == m_listTagsPerNoteRequestIds.end()) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onListAllTagsPerNoteComplete: note = "
            << note << "\nFlag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order
            << ", order direction = " << orderDirection
            << ", request id = " << requestId);

    for (const auto & foundTag: qAsConst(foundTags)) {
        requestNoteCountForTag(foundTag);
    }
}

void TagModel::onListAllTagsPerNoteFailed(
    Note note, LocalStorageManager::ListObjectsOptions flag, size_t limit,
    size_t offset, LocalStorageManager::ListTagsOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    auto it = m_listTagsPerNoteRequestIds.find(requestId);
    if (it == m_listTagsPerNoteRequestIds.end()) {
        return;
    }

    QNWARNING(
        "model:tag",
        "TagModel::onListAllTagsPerNoteFailed: note = "
            << note << "\nFlag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order
            << ", order direction = " << orderDirection << ", request id = "
            << requestId << ", error description = " << errorDescription);

    // Trying to work around this problem by re-requesting the note count for
    // all tags
    requestNoteCountsPerAllTags();
}

void TagModel::onListAllLinkedNotebooksComplete(
    size_t limit, size_t offset,
    LocalStorageManager::ListLinkedNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    QList<LinkedNotebook> foundLinkedNotebooks, QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onListAllLinkedNotebooksComplete: limit = "
            << limit << ", offset = " << offset << ", order = " << order
            << ", order direction = " << orderDirection
            << ", request id = " << requestId);

    for (const auto & foundLinkedNotebook: qAsConst(foundLinkedNotebooks)) {
        onLinkedNotebookAddedOrUpdated(foundLinkedNotebook);
    }

    m_listLinkedNotebooksRequestId = QUuid();

    if (!foundLinkedNotebooks.isEmpty()) {
        QNTRACE(
            "model:tag",
            "The number of found linked notebooks is not "
                << "empty, requesting more linked notebooks from the local "
                << "storage");

        m_listLinkedNotebooksOffset +=
            static_cast<size_t>(foundLinkedNotebooks.size());

        requestLinkedNotebooksList();
        return;
    }

    m_allLinkedNotebooksListed = true;

    if (m_allTagsListed) {
        Q_EMIT notifyAllTagsListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void TagModel::onListAllLinkedNotebooksFailed(
    size_t limit, size_t offset,
    LocalStorageManager::ListLinkedNotebooksOrder order,
    LocalStorageManager::OrderDirection orderDirection,
    ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNTRACE(
        "model:tag",
        "TagModel::onListAllLinkedNotebooksFailed: limit = "
            << limit << ", offset = " << offset << ", order = " << order
            << ", order direction = " << orderDirection
            << ", error description = " << errorDescription
            << ", request id = " << requestId);

    m_listLinkedNotebooksRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void TagModel::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNTRACE("model:tag", "TagModel::createConnections");

    // Local signals to localStorageManagerAsync's slots

    QObject::connect(
        this, &TagModel::addTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onAddTagRequest);

    QObject::connect(
        this, &TagModel::updateTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onUpdateTagRequest);

    QObject::connect(
        this, &TagModel::findTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindTagRequest);

    QObject::connect(
        this, &TagModel::listTags, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListTagsRequest);

    QObject::connect(
        this, &TagModel::expungeTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onExpungeTagRequest);

    QObject::connect(
        this, &TagModel::findNotebook, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onFindNotebookRequest);

    QObject::connect(
        this, &TagModel::requestNoteCountPerTag, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onGetNoteCountPerTagRequest);

    QObject::connect(
        this, &TagModel::requestNoteCountsForAllTags, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onGetNoteCountsPerAllTagsRequest);

    QObject::connect(
        this, &TagModel::listAllTagsPerNote, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListAllTagsPerNoteRequest);

    QObject::connect(
        this, &TagModel::listAllLinkedNotebooks, &localStorageManagerAsync,
        &LocalStorageManagerAsync::onListAllLinkedNotebooksRequest);

    // localStorageManagerAsync's signals to local slots
    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addTagComplete,
        this, &TagModel::onAddTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addTagFailed,
        this, &TagModel::onAddTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateTagComplete,
        this, &TagModel::onUpdateTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateTagFailed,
        this, &TagModel::onUpdateTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagComplete,
        this, &TagModel::onFindTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::findTagFailed,
        this, &TagModel::onFindTagFailed);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::listTagsComplete,
        this, &TagModel::onListTagsComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listTagsWithNoteLocalUidsFailed, this,
        &TagModel::onListTagsFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &TagModel::onExpungeTagComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::expungeTagFailed,
        this, &TagModel::onExpungeTagFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerTagComplete, this,
        &TagModel::onGetNoteCountPerTagComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountPerTagFailed, this,
        &TagModel::onGetNoteCountPerTagFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountsPerAllTagsComplete, this,
        &TagModel::onGetNoteCountsPerAllTagsComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::getNoteCountsPerAllTagsFailed, this,
        &TagModel::onGetNoteCountsPerAllTagsFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::
            expungeNotelessTagsFromLinkedNotebooksComplete,
        this, &TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookComplete, this,
        &TagModel::onFindNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::findNotebookFailed, this,
        &TagModel::onFindNotebookFailed);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete, this,
        &TagModel::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &TagModel::onExpungeNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::addNoteComplete,
        this, &TagModel::onAddNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::noteTagListChanged, this,
        &TagModel::onNoteTagListChanged);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete, this,
        &TagModel::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::addLinkedNotebookComplete, this,
        &TagModel::onAddLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateLinkedNotebookComplete, this,
        &TagModel::onUpdateLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeLinkedNotebookComplete, this,
        &TagModel::onExpungeLinkedNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listAllTagsPerNoteComplete, this,
        &TagModel::onListAllTagsPerNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listAllLinkedNotebooksComplete, this,
        &TagModel::onListAllLinkedNotebooksComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::listAllLinkedNotebooksFailed, this,
        &TagModel::onListAllLinkedNotebooksFailed);
}

void TagModel::requestTagsList()
{
    QNTRACE(
        "model:tag",
        "TagModel::requestTagsList: offset = " << m_listTagsOffset);

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    auto order = LocalStorageManager::ListTagsOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();

    QNTRACE(
        "model:tag",
        "Emitting the request to list tags: offset = "
            << m_listTagsOffset << ", request id = " << m_listTagsRequestId);

    Q_EMIT listTags(
        flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, {},
        m_listTagsRequestId);
}

void TagModel::requestNoteCountForTag(const Tag & tag)
{
    QNTRACE("model:tag", "TagModel::requestNoteCountForTag: " << tag);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerTagRequestIds.insert(requestId))

    QNTRACE(
        "model:tag",
        "Emitting the request to compute the number of notes "
            << "per tag, request id = " << requestId);

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountPerTag(tag, options, requestId);
}

void TagModel::requestTagsPerNote(const Note & note)
{
    QNTRACE("model:tag", "TagModel::requestTagsPerNote: " << note);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_listTagsPerNoteRequestIds.insert(requestId))

    QNTRACE(
        "model:tag",
        "Emitting the request to list tags per note: request "
            << "id = " << requestId);

    Q_EMIT listAllTagsPerNote(
        note, LocalStorageManager::ListObjectsOption::ListAll,
        /* limit = */ 0,
        /* offset = */ 0, LocalStorageManager::ListTagsOrder::NoOrder,
        LocalStorageManager::OrderDirection::Ascending, requestId);
}

void TagModel::requestNoteCountsPerAllTags()
{
    QNTRACE("model:tag", "TagModel::requestNoteCountsPerAllTags");

    m_noteCountsPerAllTagsRequestId = QUuid::createUuid();

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountsForAllTags(
        options, m_noteCountsPerAllTagsRequestId);
}

void TagModel::requestLinkedNotebooksList()
{
    QNTRACE("model:tag", "TagModel::requestLinkedNotebooksList");

    auto order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;

    m_listLinkedNotebooksRequestId = QUuid::createUuid();

    QNTRACE(
        "model:tag",
        "Emitting the request to list linked notebooks: "
            << "offset = " << m_listLinkedNotebooksOffset
            << ", request id = " << m_listLinkedNotebooksRequestId);

    Q_EMIT listAllLinkedNotebooks(
        LINKED_NOTEBOOK_LIST_LIMIT, m_listLinkedNotebooksOffset, order,
        direction, m_listLinkedNotebooksRequestId);
}

void TagModel::onTagAddedOrUpdated(
    const Tag & tag, const QStringList * pTagNoteLocalUids)
{
    m_cache.put(tag.localUid(), tag);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(tag.localUid());
    bool newTag = (itemIt == localUidIndex.end());
    if (newTag) {
        Q_EMIT aboutToAddTag();

        onTagAdded(tag, pTagNoteLocalUids);

        QModelIndex addedTagIndex = indexForLocalUid(tag.localUid());
        Q_EMIT addedTag(addedTagIndex);
    }
    else {
        QModelIndex tagIndexBefore = indexForLocalUid(tag.localUid());
        Q_EMIT aboutToUpdateTag(tagIndexBefore);

        onTagUpdated(tag, itemIt, pTagNoteLocalUids);

        QModelIndex tagIndexAfter = indexForLocalUid(tag.localUid());
        Q_EMIT updatedTag(tagIndexAfter);
    }
}

void TagModel::onTagAdded(
    const Tag & tag, const QStringList * pTagNoteLocalUids)
{
    QNTRACE(
        "model:tag",
        "TagModel::onTagAdded: tag local uid = "
            << tag.localUid() << ", tag note local uids: "
            << (pTagNoteLocalUids
                    ? pTagNoteLocalUids->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    ITagModelItem * pParentItem = nullptr;
    auto & localUidIndex = m_data.get<ByLocalUid>();

    if (tag.hasParentLocalUid()) {
        auto it = localUidIndex.find(tag.parentLocalUid());
        if (it != localUidIndex.end()) {
            pParentItem = const_cast<TagItem *>(&(*it));
        }
    }
    else if (tag.hasLinkedNotebookGuid()) {
        const QString & linkedNotebookGuid = tag.linkedNotebookGuid();
        pParentItem =
            &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!pParentItem) {
        checkAndCreateModelRootItems();
        pParentItem = m_pAllTagsRootItem;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    TagItem item;
    tagToItem(tag, item);

    checkAndFindLinkedNotebookRestrictions(item);

    if (pTagNoteLocalUids) {
        item.setNoteCount(pTagNoteLocalUids->size());
    }

    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    auto * pItem = const_cast<TagItem *>(&(*it));

    int row = rowForNewItem(*pParentItem, *pItem);

    beginInsertRows(parentIndex, row, row);
    pParentItem->insertChild(row, pItem);
    endInsertRows();

    mapChildItems(*pItem);
}

void TagModel::onTagUpdated(
    const Tag & tag, TagDataByLocalUid::iterator it,
    const QStringList * pTagNoteLocalUids)
{
    QNTRACE(
        "model:tag",
        "TagModel::onTagUpdated: tag local uid = "
            << tag.localUid() << ", tag note local uids: "
            << (pTagNoteLocalUids
                    ? pTagNoteLocalUids->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    TagItem itemCopy;
    tagToItem(tag, itemCopy);

    if (pTagNoteLocalUids) {
        itemCopy.setNoteCount(pTagNoteLocalUids->size());
    }

    auto * pTagItem = const_cast<TagItem *>(&(*it));
    auto * pParentItem = pTagItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        // FIXME: should try to fix it automatically

        ErrorString error(
            QT_TR_NOOP("Tag model item being updated does not "
                       "have a parent item linked with it"));

        QNWARNING(
            "model:tag",
            error << ", tag: " << tag << "\nTag model item: " << *pTagItem);

        Q_EMIT notifyError(error);
        return;
    }

    int row = pParentItem->rowForChild(pTagItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(
            QT_TR_NOOP("Can't find the row of the updated tag "
                       "item within its parent"));

        QNWARNING(
            "model:tag",
            error << ", tag: " << tag << "\nTag model item: " << *pTagItem);

        Q_EMIT notifyError(error);
        return;
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();

    ITagModelItem * pNewParentItem = nullptr;
    if (tag.hasParentLocalUid()) {
        auto parentIt = localUidIndex.find(tag.parentLocalUid());
        if (parentIt != localUidIndex.end()) {
            pNewParentItem = const_cast<TagItem *>(&(*parentIt));
        }
    }
    else if (tag.hasLinkedNotebookGuid()) {
        pNewParentItem =
            &(findOrCreateLinkedNotebookModelItem(tag.linkedNotebookGuid()));
    }

    if (!pNewParentItem) {
        checkAndCreateModelRootItems();
        pNewParentItem = m_pAllTagsRootItem;
    }

    QModelIndex parentItemIndex = indexForItem(pParentItem);

    QModelIndex newParentItemIndex =
        ((pParentItem == pNewParentItem) ? parentItemIndex
                                         : indexForItem(pNewParentItem));

    // 1) Remove the original row from the parent
    beginRemoveRows(parentItemIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    // 2) Insert the replacement row
    if (pParentItem != pNewParentItem) {
        row = 0;
    }

    beginInsertRows(newParentItemIndex, row, row);

    int numNotesPerTag = it->noteCount();
    itemCopy.setNoteCount(numNotesPerTag);

    Q_UNUSED(localUidIndex.replace(it, itemCopy))
    pNewParentItem->insertChild(row, pTagItem);

    endInsertRows();

    QModelIndex modelIndexFrom = index(row, 0, newParentItemIndex);

    QModelIndex modelIndexTo =
        index(row, NUM_TAG_MODEL_COLUMNS - 1, newParentItemIndex);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    // 3) Ensure all the child tag model items are properly located under this
    // tag model item
    QModelIndex modelItemIndex = indexForItem(pTagItem);

    auto & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
    auto range = parentLocalUidIndex.equal_range(pTagItem->localUid());
    for (auto childIt = range.first; childIt != range.second; ++childIt) {
        const TagItem & childItem = *childIt;
        const QString & childItemLocalUid = childItem.localUid();

        auto childItemIt = localUidIndex.find(childItemLocalUid);
        if (childItemIt != localUidIndex.end()) {
            auto & childItem = const_cast<TagItem &>(*childItemIt);

            int row = pTagItem->rowForChild(&childItem);
            if (row >= 0) {
                continue;
            }

            row = rowForNewItem(*pTagItem, childItem);
            beginInsertRows(modelItemIndex, row, row);
            pTagItem->insertChild(row, &childItem);
            endInsertRows();
        }
    }

    // 4) Update the position of the updated item within its new parent
    updateItemRowWithRespectToSorting(*pTagItem);
}

void TagModel::tagToItem(const Tag & tag, TagItem & item)
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
    item.setFavorited(tag.isFavorited());

    QNTRACE(
        "model:tag",
        "Created tag model item from tag; item: " << item << "\nTag: " << tag);
}

bool TagModel::canUpdateTagItem(const TagItem & item) const
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

bool TagModel::canCreateTagItem(const ITagModelItem & parentItem) const
{
    if (parentItem.type() != ITagModelItem::Type::Tag) {
        return false;
    }

    const auto * pTagItem = parentItem.cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        return false;
    }

    const QString & linkedNotebookGuid = pTagItem->linkedNotebookGuid();
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
    QNTRACE(
        "model:tag",
        "TagModel::updateRestrictionsFromNotebook: "
            << "local uid = " << notebook.localUid()
            << ", linked notebook guid = "
            << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid()
                                                 : QStringLiteral("<null>")));

    if (!notebook.hasLinkedNotebookGuid()) {
        QNDEBUG("model:tag", "Not a linked notebook, ignoring it");
        return;
    }

    Restrictions restrictions;

    if (!notebook.hasRestrictions()) {
        restrictions.m_canCreateTags = true;
        restrictions.m_canUpdateTags = true;
    }
    else {
        const auto & notebookRestrictions = notebook.restrictions();

        restrictions.m_canCreateTags =
            (notebookRestrictions.noCreateTags.isSet()
                 ? (!notebookRestrictions.noCreateTags.ref())
                 : true);

        restrictions.m_canUpdateTags =
            (notebookRestrictions.noUpdateTags.isSet()
                 ? (!notebookRestrictions.noUpdateTags.ref())
                 : true);
    }

    m_tagRestrictionsByLinkedNotebookGuid[notebook.linkedNotebookGuid()] =
        restrictions;

    QNTRACE(
        "model:tag",
        "Set restrictions for tags from linked notebook with "
            << "guid " << notebook.linkedNotebookGuid()
            << ": can create tags = "
            << (restrictions.m_canCreateTags ? "true" : "false")
            << ", can update tags = "
            << (restrictions.m_canUpdateTags ? "true" : "false"));
}

void TagModel::onLinkedNotebookAddedOrUpdated(
    const LinkedNotebook & linkedNotebook)
{
    QNTRACE(
        "model:tag",
        "TagModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model:tag",
            "Can't process the addition or update of "
                << "a linked notebook without guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.hasUsername())) {
        QNWARNING(
            "model:tag",
            "Can't process the addition or update of "
                << "a linked notebook without username: " << linkedNotebook);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    auto it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(
        linkedNotebookGuid);

    if (it != m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end()) {
        if (it.value() == linkedNotebook.username()) {
            QNDEBUG("model:tag", "The username hasn't changed, nothing to do");
            return;
        }

        it.value() = linkedNotebook.username();

        QNDEBUG(
            "model:tag",
            "Updated the username corresponding to linked "
                << "notebook guid " << linkedNotebookGuid << " to "
                << linkedNotebook.username());
    }
    else {
        QNDEBUG(
            "model:tag",
            "Adding new username " << linkedNotebook.username()
                                   << " corresponding to linked notebook guid "
                                   << linkedNotebookGuid);

        it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(
            linkedNotebookGuid, linkedNotebook.username());
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt == m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model:tag",
            "Found no existing linked notebook item for "
                << "linked notebook guid " << linkedNotebookGuid
                << ", creating one");

        linkedNotebookItemIt = m_linkedNotebookItems.insert(
            linkedNotebookGuid,
            TagLinkedNotebookRootItem(
                linkedNotebook.username(), linkedNotebookGuid));

        checkAndCreateModelRootItems();

        auto * pLinkedNotebookItem = &(linkedNotebookItemIt.value());
        int row = rowForNewItem(*m_pAllTagsRootItem, *pLinkedNotebookItem);
        beginInsertRows(indexForItem(m_pAllTagsRootItem), row, row);
        m_pAllTagsRootItem->insertChild(row, pLinkedNotebookItem);
        endInsertRows();
    }
    else {
        linkedNotebookItemIt->setUsername(linkedNotebook.username());

        QNTRACE(
            "model:tag",
            "Updated the linked notebook username to "
                << linkedNotebook.username()
                << " for linked notebook item corresponding to "
                << "linked notebook guid " << linkedNotebookGuid);
    }

    auto linkedNotebookItemIndex =
        indexForLinkedNotebookGuid(linkedNotebookGuid);

    Q_EMIT dataChanged(linkedNotebookItemIndex, linkedNotebookItemIndex);
}

ITagModelItem * TagModel::itemForId(const IndexId id) const
{
    QNTRACE("model:tag", "TagModel::itemForId: " << id);

    if (id == m_allTagsRootItemIndexId) {
        return m_pAllTagsRootItem;
    }

    auto localUidIt = m_indexIdToLocalUidBimap.left.find(id);
    if (localUidIt == m_indexIdToLocalUidBimap.left.end()) {
        auto linkedNotebookGuidIt =
            m_indexIdToLinkedNotebookGuidBimap.left.find(id);

        if (linkedNotebookGuidIt ==
            m_indexIdToLinkedNotebookGuidBimap.left.end()) {
            QNDEBUG(
                "model:tag",
                "Found no tag model item corresponding to "
                    << "model index internal id");

            return nullptr;
        }

        const QString & linkedNotebookGuid = linkedNotebookGuidIt->second;
        auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
        if (it == m_linkedNotebookItems.end()) {
            QNDEBUG(
                "model:tag",
                "Found no tag linked notebook root model item "
                    << "corresponding to the linked notebook guid "
                    << "corresponding to model index internal id");

            return nullptr;
        }

        return const_cast<TagLinkedNotebookRootItem *>(&(it.value()));
    }

    const QString & localUid = localUidIt->second;

    QNTRACE(
        "model:tag",
        "Found tag local uid corresponding to model index "
            << "internal id: " << localUid);

    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (it != localUidIndex.end()) {
        return const_cast<TagItem *>(&(*it));
    }

    QNTRACE(
        "model:tag",
        "Found no tag item corresponding to local uid " << localUid);

    return nullptr;
}

TagModel::IndexId TagModel::idForItem(const ITagModelItem & item) const
{
    const auto * pTagItem = item.cast<TagItem>();
    if (pTagItem) {
        auto it = m_indexIdToLocalUidBimap.right.find(pTagItem->localUid());
        if (it == m_indexIdToLocalUidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLocalUidBimap.insert(
                IndexIdToLocalUidBimap::value_type(id, pTagItem->localUid())))
            return id;
        }

        return it->second;
    }

    const auto * pLinkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
            pLinkedNotebookItem->linkedNotebookGuid());

        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.insert(
                IndexIdToLinkedNotebookGuidBimap::value_type(
                    id, pLinkedNotebookItem->linkedNotebookGuid())))
            return id;
        }

        return it->second;
    }

    if (&item == m_pAllTagsRootItem) {
        return m_allTagsRootItemIndexId;
    }

    QNWARNING(
        "model:tag",
        "Detected attempt to assign id to unidentified "
            << "tag model item: " << item);
    return 0;
}

QVariant TagModel::dataImpl(
    const ITagModelItem & item, const Column column) const
{
    if (&item == m_pAllTagsRootItem) {
        if (column == Column::Name) {
            return tr("All tags");
        }

        return {};
    }

    const auto * pTagItem = item.cast<TagItem>();
    if (pTagItem) {
        switch (column) {
        case Column::Name:
            return QVariant(pTagItem->name());
        case Column::Synchronizable:
            return QVariant(pTagItem->isSynchronizable());
        case Column::Dirty:
            return QVariant(pTagItem->isDirty());
        case Column::FromLinkedNotebook:
            return QVariant(!pTagItem->linkedNotebookGuid().isEmpty());
        case Column::NoteCount:
            return QVariant(pTagItem->noteCount());
        default:
            return {};
        }
    }

    const auto * pLinkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        switch (column) {
        case Column::Name:
            return QVariant(pLinkedNotebookItem->username());
        case Column::FromLinkedNotebook:
            return QVariant(true);
        default:
            return {};
        }
    }

    return {};
}

QVariant TagModel::dataAccessibleText(
    const ITagModelItem & item, const Column column) const
{
    QVariant textData = dataImpl(item, column);
    if (textData.isNull()) {
        return QVariant();
    }

    QString accessibleText = tr("Tag") + QStringLiteral(": ");

    switch (column) {
    case Column::Name:
        accessibleText +=
            tr("name is") + QStringLiteral(" ") + textData.toString();
        break;
    case Column::Synchronizable:
        accessibleText +=
            (textData.toBool() ? tr("synchronizable")
                               : tr("not synchronizable"));
        break;
    case Column::Dirty:
        accessibleText += (textData.toBool() ? tr("dirty") : tr("not dirty"));
        break;
    case Column::FromLinkedNotebook:
        accessibleText +=
            (textData.toBool() ? tr("from linked notebook")
                               : tr("from own account"));
        break;
    case Column::NoteCount:
        accessibleText += tr("number of notes");
        break;
    default:
        return QVariant();
    }

    return QVariant(accessibleText);
}

ITagModelItem * TagModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_pInvisibleRootItem;
    }

    return itemForId(static_cast<IndexId>(index.internalId()));
}

ITagModelItem * TagModel::itemForLocalUid(const QString & localUid) const
{
    const auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (it != localUidIndex.end()) {
        return &(const_cast<TagItem &>(*it));
    }

    return nullptr;
}

QModelIndex TagModel::indexForItem(const ITagModelItem * pItem) const
{
    if (!pItem) {
        return {};
    }

    if (pItem == m_pInvisibleRootItem) {
        return {};
    }

    if (pItem == m_pAllTagsRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allTagsRootItemIndexId);
    }

    auto * pParentItem = pItem->parent();
    if (!pParentItem) {
        QNWARNING(
            "model:tag",
            "Tag model item has no parent, returning "
                << "invalid index: " << *pItem);
        return {};
    }

    int row = pParentItem->rowForChild(pItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:tag",
            "Internal error: can't get row of the child "
                << "item in parent in TagModel, child item: " << *pItem
                << "\nParent item: " << *pParentItem);
        return {};
    }

    IndexId itemId = idForItem(*pItem);
    return createIndex(row, static_cast<int>(Column::Name), itemId);
}

QModelIndex TagModel::indexForTagName(
    const QString & tagName, const QString & linkedNotebookGuid) const
{
    const auto & nameIndex = m_data.get<ByNameUpper>();

    auto range = nameIndex.equal_range(tagName.toUpper());
    for (auto it = range.first; it != range.second; ++it) {
        const auto & item = *it;
        if (item.linkedNotebookGuid() == linkedNotebookGuid) {
            return indexForLocalUid(item.localUid());
        }
    }

    return {};
}

QModelIndex TagModel::indexForLinkedNotebookGuid(
    const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:tag",
        "TagModel::indexForLinkedNotebookGuid: "
            << "linked notebook guid = " << linkedNotebookGuid);

    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it == m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model:tag",
            "Found no model item for linked notebook guid "
                << linkedNotebookGuid);
        return {};
    }

    const auto & item = it.value();
    return indexForItem(&item);
}

QModelIndex TagModel::promote(const QModelIndex & itemIndex)
{
    QNTRACE("model:tag", "TagModel::promote");

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote tag: invalid model index"));
        return {};
    }

    auto * pModelItem = itemForIndex(itemIndex);
    if (!pModelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote tag: no tag item was found"));
        return {};
    }

    auto * pTagItem = pModelItem->cast<TagItem>();
    if (!pTagItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote non-tag items"));
        return {};
    }

    checkAndCreateModelRootItems();

    fixupItemParent(*pModelItem);
    auto * pParentItem = pModelItem->parent();

    if (pParentItem == m_pAllTagsRootItem) {
        REPORT_INFO(QT_TR_NOOP("Can't promote tag: already at top level"));
        return {};
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (row < 0) {
        QNDEBUG(
            "model:tag",
            "Can't find row of promoted item within its "
                << "parent item");
        return {};
    }

    fixupItemParent(*pParentItem);
    auto * pGrandParentItem = pParentItem->parent();

    auto * pGrandParentTagItem = pGrandParentItem->cast<TagItem>();
    if (!canCreateTagItem(*pGrandParentItem) ||
        (pGrandParentTagItem && !canUpdateTagItem(*pGrandParentTagItem)))
    {
        REPORT_INFO(
            QT_TR_NOOP("Can't promote tag: can't create and/or update tags "
                       "for the grand parent tag due to restrictions"));
        return {};
    }

    int parentRow = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(parentRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't promote tag: can't find parent item's row within "
                       "the grand parent model item"));
        return {};
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * pTakenItem = pParentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, detected "
                       "internal inconsistency in the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original promoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    auto grandParentIndex = indexForItem(pGrandParentItem);
    int appropriateRow = rowForNewItem(*pGrandParentItem, *pTakenItem);

    beginInsertRows(grandParentIndex, appropriateRow, appropriateRow);
    pGrandParentItem->insertChild(appropriateRow, pTakenItem);
    endInsertRows();

    auto newIndex =
        index(appropriateRow, static_cast<int>(Column::Name), grandParentIndex);

    if (!newIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, invalid model index "
                       "was returned for the promoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(grandParentIndex, appropriateRow, appropriateRow);
        Q_UNUSED(pGrandParentItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy(*pTagItem);
    if (pGrandParentTagItem) {
        tagItemCopy.setParentLocalUid(pGrandParentTagItem->localUid());
        tagItemCopy.setParentGuid(pGrandParentTagItem->guid());
    }
    else {
        tagItemCopy.setParentLocalUid(QString());
        tagItemCopy.setParentGuid(QString());
    }

    bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(tagItemCopy.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO(
            "model:tag",
            "The promoted tag model item was not found in "
                << "the index which is odd. Adding it there");
        Q_UNUSED(localUidIndex.insert(tagItemCopy))
    }
    else {
        localUidIndex.replace(it, tagItemCopy);
    }

    if (!wasDirty) {
        QModelIndex dirtyColumnIndex = index(
            appropriateRow, static_cast<int>(Column::Dirty), grandParentIndex);

        Q_EMIT dataChanged(dirtyColumnIndex, dirtyColumnIndex);
    }

    updateTagInLocalStorage(tagItemCopy);

    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndex TagModel::demote(const QModelIndex & itemIndex)
{
    QNTRACE("model:tag", "TagModel::demote");

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote tag: model index is invalid"));
        return {};
    }

    auto * pModelItem = itemForIndex(itemIndex);
    if (!pModelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote tag: no tag item was found"));
        return {};
    }

    auto * pTagItem = pModelItem->cast<TagItem>();
    if (!pTagItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote non-tag items"));
        return {};
    }

    checkAndCreateModelRootItems();

    fixupItemParent(*pModelItem);
    auto * pParentItem = pModelItem->parent();

    auto * pParentTagItem = pParentItem->cast<TagItem>();
    if (pParentTagItem && !canUpdateTagItem(*pParentTagItem)) {
        REPORT_INFO(
            QT_TR_NOOP("Can't demote tag: can't update parent tag "
                       "due to insufficient permissions"));
        return {};
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (row < 0) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: can't find the row of "
                       "demoted tag within its parent"));
        return {};
    }
    else if (row == 0) {
        REPORT_INFO(
            QT_TR_NOOP("Can't demote tag: found no preceding sibling within "
                       "the parent model item to demote the tag under"));
        return {};
    }

    auto * pSiblingItem = pParentItem->childAtRow(row - 1);
    if (Q_UNLIKELY(!pSiblingItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: no sibling tag "
                       "appropriate for demoting was found"));
        return {};
    }

    auto * pSiblingTagItem = pSiblingItem->cast<TagItem>();
    if (Q_UNLIKELY(!pSiblingTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: the sibling model item "
                       "is not of a tag type"));
        return {};
    }

    const QString & itemLinkedNotebookGuid = pTagItem->linkedNotebookGuid();
    const QString & siblingItemLinkedNotebookGuid =
        pSiblingTagItem->linkedNotebookGuid();

    if ((pParentItem == m_pAllTagsRootItem) &&
        (siblingItemLinkedNotebookGuid != itemLinkedNotebookGuid))
    {
        ErrorString error;
        if (itemLinkedNotebookGuid.isEmpty() !=
            siblingItemLinkedNotebookGuid.isEmpty()) {
            error.setBase(
                QT_TR_NOOP("Can't demote tag: can't mix tags from linked "
                           "notebooks with tags from user's own account"));
        }
        else {
            error.setBase(
                QT_TR_NOOP("Can't demote tag: can't mix tags "
                           "from different linked notebooks"));
        }

        QNINFO(
            "model:tag",
            error << ", item attempted to be demoted: " << *pModelItem
                  << "\nSibling item: " << *pSiblingItem);
        Q_EMIT notifyError(error);
        return {};
    }

    if (!canCreateTagItem(*pSiblingItem)) {
        REPORT_INFO(
            QT_TR_NOOP("Can't demote tag: can't create tags within "
                       "the sibling tag due to restrictions"));
        return {};
    }

    auto siblingItemIndex = indexForItem(pSiblingItem);
    if (Q_UNLIKELY(!siblingItemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: can't get valid model index for "
                       "the sibling tag"));
        return {};
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * pTakenItem = pParentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, detected "
                       "internal inconsistency within the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original demoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    int appropriateRow = rowForNewItem(*pSiblingItem, *pTakenItem);

    // Need to update this index since its row within parent might have changed
    siblingItemIndex = indexForItem(pSiblingItem);
    beginInsertRows(siblingItemIndex, appropriateRow, appropriateRow);
    pSiblingItem->insertChild(appropriateRow, pTakenItem);
    endInsertRows();

    auto newIndex =
        index(appropriateRow, static_cast<int>(Column::Name), siblingItemIndex);

    if (!newIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, invalid model index "
                       "was returned for the demoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(siblingItemIndex, appropriateRow, appropriateRow);
        Q_UNUSED(pSiblingItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy(*pTagItem);
    tagItemCopy.setParentLocalUid(pSiblingTagItem->localUid());
    tagItemCopy.setParentGuid(pSiblingTagItem->guid());

    bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(tagItemCopy.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO(
            "model:tag",
            "The demoted tag model item was not found in "
                << "the index which is odd. Adding it there");
        Q_UNUSED(localUidIndex.insert(tagItemCopy))
    }
    else {
        localUidIndex.replace(it, tagItemCopy);
    }

    if (!wasDirty) {
        QModelIndex dirtyColumnIndex = index(
            appropriateRow, static_cast<int>(Column::Dirty), siblingItemIndex);

        Q_EMIT dataChanged(dirtyColumnIndex, dirtyColumnIndex);
    }

    updateTagInLocalStorage(tagItemCopy);

    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndex TagModel::moveToParent(
    const QModelIndex & index, const QString & parentTagName)
{
    QNTRACE(
        "model:tag",
        "TagModel::moveToParent: parent tag name = " << parentTagName);

    if (Q_UNLIKELY(parentTagName.isEmpty())) {
        return removeFromParent(index);
    }

    auto * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to move tag item to "
                       "another parent but the model index has no internal id "
                       "corresponding to the item"));
        return {};
    }

    if (Q_UNLIKELY(pModelItem == m_pAllTagsRootItem)) {
        QNDEBUG("model:tag", "Can't move all tags root item to a new parent");
        return {};
    }

    if (Q_UNLIKELY(pModelItem == m_pInvisibleRootItem)) {
        QNDEBUG("model:tag", "Can't move invisible root item to a new parent");
        return {};
    }

    const TagItem * pTagItem = pModelItem->cast<TagItem>();
    if (!pTagItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move non-tag model item to another parent"));
        return {};
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto tagItemIt = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(tagItemIt == localUidIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag item being "
                       "moved to another parent within the tag model"));
        return {};
    }

    fixupItemParent(*pModelItem);
    auto * pParentItem = pModelItem->parent();
    auto * pParentTagItem = pParentItem->cast<TagItem>();

    if (pParentTagItem &&
        (pParentTagItem->nameUpper() == parentTagName.toUpper())) {
        QNDEBUG(
            "model:tag",
            "The tag is already under the parent with "
                << "the correct name, nothing to do");
        return index;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    auto newParentItemsRange = nameIndex.equal_range(parentTagName.toUpper());
    auto newParentItemIt = nameIndex.end();
    for (auto it = newParentItemsRange.first; it != newParentItemsRange.second;
         ++it)
    {
        const auto & newParentTagItem = *it;
        if (newParentTagItem.linkedNotebookGuid() ==
            pTagItem->linkedNotebookGuid()) {
            newParentItemIt = it;
            break;
        }
    }

    if (Q_UNLIKELY(newParentItemIt == nameIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the parent tag "
                       "under which the current tag should be moved"));
        return {};
    }

    auto * pNewParentItem = const_cast<TagItem *>(&(*newParentItemIt));
    if (Q_UNLIKELY(pNewParentItem->type() != ITagModelItem::Type::Tag)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: the tag model item corresponding to "
                       "the parent tag under which the current tag should be "
                       "moved has wrong item type"));
        return {};
    }

    auto * pNewParentTagItem = pNewParentItem->cast<TagItem>();
    if (Q_UNLIKELY(!pNewParentTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: the tag model item corresponding to "
                       "the parent tag under which the current tag should be "
                       "moved has no tag item"));
        return {};
    }

    // If the new parent is actually one of the children of the original item,
    // reject the reparent attempt
    const int numMovedItemChildren = pModelItem->childrenCount();
    for (int i = 0; i < numMovedItemChildren; ++i) {
        const auto * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(
                "model:tag",
                "Found null child tag model item at row "
                    << i << ", parent item: " << *pModelItem);
            continue;
        }

        if (pChildItem == pNewParentItem) {
            ErrorString error(
                QT_TR_NOOP("Can't set the parent of the tag to "
                           "one of its child tags"));
            QNINFO("model:tag", error);
            Q_EMIT notifyError(error);
            return {};
        }
    }

    removeModelItemFromParent(*pModelItem);

    TagItem tagItemCopy(*pTagItem);
    tagItemCopy.setParentLocalUid(pNewParentTagItem->localUid());
    tagItemCopy.setParentGuid(pNewParentTagItem->guid());
    tagItemCopy.setDirty(true);
    localUidIndex.replace(tagItemIt, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    QModelIndex parentIndex = indexForItem(pNewParentItem);
    int newRow = rowForNewItem(*pNewParentItem, *pModelItem);

    beginInsertRows(parentIndex, newRow, newRow);
    pNewParentItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QModelIndex newIndex = indexForItem(pModelItem);
    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndex TagModel::removeFromParent(const QModelIndex & index)
{
    QNTRACE("model:tag", "TagModel::removeFromParent");

    auto * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to remove "
                       "the tag model item from its parent but "
                       "the model index has no internal id "
                       "corresponding to any tag model item"));
        return {};
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (!pTagItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can only remove tag items from their parent tags"));
        return {};
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't find the tag to be removed from its "
                       "parent within the tag model"));
        QNDEBUG("model:tag", "Tag item: " << *pTagItem);
        return {};
    }

    removeModelItemFromParent(*pModelItem);

    TagItem tagItemCopy(*pTagItem);
    tagItemCopy.setParentGuid(QString());
    tagItemCopy.setParentLocalUid(QString());
    tagItemCopy.setDirty(true);
    localUidIndex.replace(it, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    checkAndCreateModelRootItems();

    QNDEBUG(
        "model:tag",
        "Setting all tags root item as the new parent for "
            << "the tag");

    setItemParent(*pModelItem, *m_pAllTagsRootItem);

    auto newIndex = indexForItem(pModelItem);
    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QStringList TagModel::tagNames(const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model:tag",
        "TagModel::tagNames: linked notebook guid = "
            << linkedNotebookGuid
            << " (null = " << (linkedNotebookGuid.isNull() ? "true" : "false")
            << ", empty = " << (linkedNotebookGuid.isEmpty() ? "true" : "false")
            << ")");

    const auto & nameIndex = m_data.get<ByNameUpper>();

    QStringList result;
    result.reserve(static_cast<int>(nameIndex.size()));

    for (const auto & item: nameIndex) {
        if (!tagItemMatchesByLinkedNotebook(item, linkedNotebookGuid)) {
            continue;
        }

        result << item.name();
    }

    return result;
}

QModelIndex TagModel::createTag(
    const QString & tagName, const QString & parentTagName,
    const QString & linkedNotebookGuid, ErrorString & errorDescription)
{
    QNTRACE(
        "model:tag",
        "TagModel::createTag: tag name = "
            << tagName << ", parent tag name = " << parentTagName
            << ", linked notebook guid = " << linkedNotebookGuid);

    if (tagName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Tag name is empty"));
        return {};
    }

    int tagNameSize = tagName.size();

    if (tagNameSize < qevercloud::EDAM_TAG_NAME_LEN_MIN) {
        errorDescription.setBase(
            QT_TR_NOOP("Tag name size is below the minimal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_TAG_NAME_LEN_MIN);

        return {};
    }

    if (tagNameSize > qevercloud::EDAM_TAG_NAME_LEN_MAX) {
        errorDescription.setBase(
            QT_TR_NOOP("Tag name size is above the maximal acceptable length"));

        errorDescription.details() =
            QString::number(qevercloud::EDAM_TAG_NAME_LEN_MAX);

        return {};
    }

    auto existingItemIndex = indexForTagName(tagName, linkedNotebookGuid);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Tag with such name already exists"));

        return {};
    }

    if (!linkedNotebookGuid.isEmpty()) {
        auto restrictionsIt =
            m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);

        if (restrictionsIt == m_tagRestrictionsByLinkedNotebookGuid.end()) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't find tag restrictions "
                           "for the specified linked notebook"));

            return {};
        }

        const Restrictions & restrictions = restrictionsIt.value();
        if (!restrictions.m_canCreateTags) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't create a new tag as  linked notebook "
                           "restrictions prohibit the creation of new tags"));

            return {};
        }
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingTags = static_cast<int>(localUidIndex.size());

    if (Q_UNLIKELY(numExistingTags + 1 >= m_account.tagCountMax())) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new tag: the account "
                       "can contain a limited number of tags"));

        errorDescription.details() = QString::number(m_account.tagCountMax());
        return {};
    }

    ITagModelItem * pParentItem = nullptr;

    if (!linkedNotebookGuid.isEmpty()) {
        pParentItem =
            &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!pParentItem) {
        checkAndCreateModelRootItems();
        pParentItem = m_pAllTagsRootItem;
    }

    if (!parentTagName.isEmpty()) {
        const auto & nameIndex = m_data.get<ByNameUpper>();
        auto parentTagRange = nameIndex.equal_range(parentTagName.toUpper());
        auto parentTagIt = nameIndex.end();

        for (auto it = parentTagRange.first; it != parentTagRange.second; ++it)
        {
            if (it->linkedNotebookGuid() == linkedNotebookGuid) {
                parentTagIt = it;
                break;
            }
        }

        if (Q_UNLIKELY(parentTagIt == nameIndex.end())) {
            errorDescription.setBase(
                QT_TR_NOOP("Can't create a new tag: the parent tag was not "
                           "found within the model"));

            errorDescription.details() = parentTagName;
            return {};
        }

        pParentItem = const_cast<TagItem *>(&(*parentTagIt));

        QNDEBUG(
            "model:tag",
            "Will put the new tag under parent item: " << *pParentItem);
    }

    TagItem item;
    item.setLocalUid(UidGenerator::Generate());
    Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

    item.setName(tagName);
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    auto * pParentTagItem = pParentItem->cast<TagItem>();
    if (pParentTagItem) {
        item.setParentLocalUid(pParentTagItem->localUid());
    }

    Q_EMIT aboutToAddTag();

    auto insertionResult = localUidIndex.insert(item);
    auto * pTagItem = const_cast<TagItem *>(&(*insertionResult.first));
    setItemParent(*pTagItem, *pParentItem);

    updateTagInLocalStorage(item);

    QModelIndex addedTagIndex = indexForLocalUid(item.localUid());
    Q_EMIT addedTag(addedTagIndex);

    return addedTagIndex;
}

QString TagModel::columnName(const TagModel::Column column) const
{
    switch (column) {
    case Column::Name:
        return tr("Name");
    case Column::Synchronizable:
        return tr("Synchronizable");
    case Column::Dirty:
        return tr("Changed");
    case Column::FromLinkedNotebook:
        return tr("From linked notebook");
    case Column::NoteCount:
        return tr("Note count");
    default:
        return QString();
    }
}

bool TagModel::hasSynchronizableChildren(const ITagModelItem * pModelItem) const
{
    const auto * pLinkedNotebookItem =
        pModelItem->cast<TagLinkedNotebookRootItem>();

    if (pLinkedNotebookItem) {
        return true;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (pTagItem && pTagItem->isSynchronizable()) {
        return true;
    }

    auto children = pModelItem->children();
    for (const auto * pChild: qAsConst(children)) {
        if (Q_UNLIKELY(!pChild)) {
            QNWARNING(
                "model:tag",
                "Found null child at tag model item: " << *pModelItem);
            continue;
        }

        if (hasSynchronizableChildren(pChild)) {
            return true;
        }
    }

    return false;
}

void TagModel::mapChildItems()
{
    QNTRACE("model:tag", "TagModel::mapChildItems");

    auto & localUidIndex = m_data.get<ByLocalUid>();
    for (auto & item: localUidIndex) {
        mapChildItems(const_cast<TagItem &>(item));
    }

    for (auto it = m_linkedNotebookItems.begin(),
              end = m_linkedNotebookItems.end();
         it != end; ++it)
    {
        auto & item = *it;
        mapChildItems(item);
    }
}

void TagModel::mapChildItems(ITagModelItem & item)
{
    QNTRACE("model:tag", "TagModel::mapChildItems: " << item);

    const auto * pTagItem = item.cast<TagItem>();
    const auto * pLinkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();

    if (Q_UNLIKELY(!pTagItem && !pLinkedNotebookItem)) {
        return;
    }

    auto parentIndex = indexForItem(&item);

    if (pTagItem) {
        auto & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
        auto range = parentLocalUidIndex.equal_range(pTagItem->localUid());
        for (auto it = range.first; it != range.second; ++it) {
            auto & currentTagItem = const_cast<TagItem &>(*it);

            int row = item.rowForChild(&currentTagItem);
            if (row >= 0) {
                continue;
            }

            removeModelItemFromParent(currentTagItem);

            row = rowForNewItem(item, currentTagItem);
            beginInsertRows(parentIndex, row, row);
            item.insertChild(row, &currentTagItem);
            endInsertRows();
        }

        return;
    }

    if (pLinkedNotebookItem) {
        auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();

        auto range = linkedNotebookGuidIndex.equal_range(
            pLinkedNotebookItem->linkedNotebookGuid());

        for (auto it = range.first; it != range.second; ++it) {
            auto & currentTagItem = const_cast<TagItem &>(*it);
            if (!currentTagItem.parentLocalUid().isEmpty()) {
                continue;
            }

            int row = item.rowForChild(&currentTagItem);
            if (row >= 0) {
                continue;
            }

            removeModelItemFromParent(currentTagItem);

            row = rowForNewItem(item, currentTagItem);
            beginInsertRows(parentIndex, row, row);
            item.insertChild(row, &currentTagItem);
            endInsertRows();
        }

        return;
    }
}

QString TagModel::nameForNewTag(const QString & linkedNotebookGuid) const
{
    QString baseName = tr("New tag");
    const auto & nameIndex = m_data.get<ByNameUpper>();
    QSet<QString> tagNames;
    for (const auto & item: nameIndex) {
        if (item.linkedNotebookGuid() != linkedNotebookGuid) {
            continue;
        }

        Q_UNUSED(tagNames.insert(item.nameUpper()))
    }

    int & lastNewTagNameCounter =
        (linkedNotebookGuid.isEmpty()
             ? m_lastNewTagNameCounter
             : m_lastNewTagNameCounterByLinkedNotebookGuid[linkedNotebookGuid]);

    return newItemName(tagNames, lastNewTagNameCounter, baseName);
}

void TagModel::removeItemByLocalUid(const QString & localUid)
{
    QNTRACE("model:tag", "TagModel::removeItemByLocalUid: " << localUid);

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto itemIt = localUidIndex.find(localUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        QNDEBUG("model:tag", "Can't find item to remove from the tag model");
        return;
    }

    auto * pTagItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*pTagItem);
    auto * pParentItem = pTagItem->parent();

    int row = pParentItem->rowForChild(pTagItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:tag",
            "Internal error: can't get the child item's row "
                << "within its parent in the tag model, child item: "
                << *pTagItem << "\nParent item: " << *pParentItem);
        return;
    }

    // Need to recursively remove all the children of this tag and do this
    // before the actual removal of their parent
    auto & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
    while (true) {
        auto childIt = parentLocalUidIndex.find(localUid);
        if (childIt == parentLocalUidIndex.end()) {
            break;
        }

        removeItemByLocalUid(childIt->localUid());
    }

    auto parentItemModelIndex = indexForItem(pParentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    auto indexIt = m_indexIdToLocalUidBimap.right.find(itemIt->localUid());
    if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLocalUidBimap.right.erase(indexIt))
    }

    Q_UNUSED(localUidIndex.erase(itemIt))

    checkAndRemoveEmptyLinkedNotebookRootItem(*pParentItem);
}

void TagModel::removeModelItemFromParent(ITagModelItem & item)
{
    QNTRACE("model:tag", "TagModel::removeModelItemFromParent: " << item);

    auto * pParentItem = item.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNDEBUG("model:tag", "No parent item, nothing to do");
        return;
    }

    QNTRACE("model:tag", "Parent item: " << *pParentItem);
    int row = pParentItem->rowForChild(&item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model:tag",
            "Can't find the child tag item's row within its "
                << "parent; child item = " << item
                << ", parent item = " << *pParentItem);
        return;
    }

    QNTRACE("model:tag", "Removing the child at row " << row);

    auto parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();
}

int TagModel::rowForNewItem(
    const ITagModelItem & parentItem, const ITagModelItem & newItem) const
{
    QNTRACE(
        "model:tag",
        "TagModel::rowForNewItem: new item = " << newItem << ", parent item = "
                                               << parentItem);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model:tag", "Won't sort on column " << m_sortedColumn);
        // Sorting by other columns is not yet implemented
        return parentItem.childrenCount();
    }

    auto children = parentItem.children();
    auto it = children.constEnd();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(
            children.constBegin(), children.constEnd(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(
            children.constBegin(), children.constEnd(), &newItem,
            GreaterByName());
    }

    int row = -1;
    if (it == children.constEnd()) {
        row = parentItem.childrenCount();
    }
    else {
        row = static_cast<int>(std::distance(children.constBegin(), it));
    }

    QNTRACE("model:tag", "Appropriate row = " << row);
    return row;
}

void TagModel::updateItemRowWithRespectToSorting(ITagModelItem & item)
{
    QNTRACE(
        "model:tag",
        "TagModel::updateItemRowWithRespectToSorting: item = " << item);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model:tag", "Won't sort on column " << m_sortedColumn);
        // Sorting by other columns is not yet implemented
        return;
    }

    fixupItemParent(item);
    auto * pParentItem = item.parent();

    int currentItemRow = pParentItem->rowForChild(&item);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(
            "model:tag",
            "Can't update tag model item's row: can't find "
                << "its original row within parent: " << item);
        return;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(pParentItem->takeChild(currentItemRow))
    endRemoveRows();

    int appropriateRow = rowForNewItem(*pParentItem, item);
    beginInsertRows(parentIndex, appropriateRow, appropriateRow);
    pParentItem->insertChild(appropriateRow, &item);
    endInsertRows();

    QNTRACE(
        "model:tag",
        "Moved item from row " << currentItemRow << " to row " << appropriateRow
                               << "; item: " << item);
}

void TagModel::updatePersistentModelIndices()
{
    QNTRACE("model:tag", "TagModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    auto indices = persistentIndexList();
    for (const auto & index: qAsConst(indices)) {
        auto * pItem = itemForId(static_cast<IndexId>(index.internalId()));
        QModelIndex replacementIndex = indexForItem(pItem);
        changePersistentIndex(index, replacementIndex);
    }
}

void TagModel::updateTagInLocalStorage(const TagItem & item)
{
    QNTRACE(
        "model:tag",
        "TagModel::updateTagInLocalStorage: local uid = " << item.localUid());

    Tag tag;

    auto notYetSavedItemIt =
        m_tagItemsNotYetInLocalStorageUids.find(item.localUid());

    if (notYetSavedItemIt == m_tagItemsNotYetInLocalStorageUids.end()) {
        QNDEBUG("model:tag", "Updating the tag");

        const auto * pCachedTag = m_cache.get(item.localUid());
        if (Q_UNLIKELY(!pCachedTag)) {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))

            Tag dummy;
            dummy.setLocalUid(item.localUid());

            QNDEBUG(
                "model:tag",
                "Emitting the request to find tag: "
                    << "local uid = " << item.localUid()
                    << ", request id = " << requestId);

            Q_EMIT findTag(dummy, requestId);
            return;
        }

        tag = *pCachedTag;
    }

    tagFromItem(item, tag);

    auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageUids.end()) {
        Q_UNUSED(m_addTagRequestIds.insert(requestId));

        QNTRACE(
            "model:tag",
            "Emitting the request to add the tag to the local "
                << "storage: id = " << requestId << ", tag: " << tag);

        Q_EMIT addTag(tag, requestId);
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));

        // While the tag is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(tag.localUid()))

        QNTRACE(
            "model:tag",
            "Emitting the request to update tag in the local "
                << "storage: id = " << requestId << ", tag: " << tag);

        Q_EMIT updateTag(tag, requestId);
    }
}

void TagModel::tagFromItem(const TagItem & item, Tag & tag) const
{
    tag.setLocalUid(item.localUid());
    tag.setGuid(item.guid());
    tag.setLinkedNotebookGuid(item.linkedNotebookGuid());
    tag.setName(item.name());
    tag.setLocal(!item.isSynchronizable());
    tag.setDirty(item.isDirty());
    tag.setFavorited(item.isFavorited());
    tag.setParentLocalUid(item.parentLocalUid());
    tag.setParentGuid(item.parentGuid());
}

void TagModel::setNoteCountForTag(
    const QString & tagLocalUid, const int noteCount)
{
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto itemIt = localUidIndex.find(tagLocalUid);
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        // Probably this tag was expunged
        QNDEBUG(
            "model:tag",
            "No tag receiving the note count update was found "
                << "in the model: " << tagLocalUid);
        return;
    }

    auto * pModelItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*pModelItem);
    auto * pParentItem = pModelItem->parent();

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(
            QT_TR_NOOP("Can't find the row of tag model item being updated "
                       "with the note count within its parent"));

        QNWARNING(
            "model:tag",
            error << ", tag local uid: " << tagLocalUid
                  << "\nTag model item: " << *pModelItem);

        Q_EMIT notifyError(error);
        return;
    }

    TagItem itemCopy(*itemIt);
    itemCopy.setNoteCount(noteCount);
    Q_UNUSED(localUidIndex.replace(itemIt, itemCopy))

    auto id = idForItem(*pModelItem);
    auto index = createIndex(row, static_cast<int>(Column::NoteCount), id);
    Q_EMIT dataChanged(index, index);

    // NOTE: in future, if/when sorting by note count is supported, will need to
    // check if need to re-sort and Q_EMIT the layout changed signal
}

void TagModel::setTagFavorited(const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: model "
                       "index is invalid"));
        return;
    }

    auto * pModelItem = itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: can't "
                       "find the model item corresponding to index"));
        return;
    }

    auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: "
                       "the target model item is not a tag item"));
        return;
    }

    if (favorited == pTagItem->isFavorited()) {
        QNDEBUG("model:tag", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: the modified tag "
                       "entry was not found within the model"));
        return;
    }

    TagItem itemCopy(*pTagItem);
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the tag as dirty as favorited property is not included
    // into the synchronization protocol

    localUidIndex.replace(it, itemCopy);
    updateTagInLocalStorage(itemCopy);
}

void TagModel::beginRemoveTags()
{
    Q_EMIT aboutToRemoveTags();
}

void TagModel::endRemoveTags()
{
    Q_EMIT removedTags();
}

ITagModelItem & TagModel::findOrCreateLinkedNotebookModelItem(
    const QString & linkedNotebookGuid)
{
    QNTRACE(
        "model:tag",
        "TagModel::findOrCreateLinkedNotebookModelItem: "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    if (Q_UNLIKELY(linkedNotebookGuid.isEmpty())) {
        QNWARNING(
            "model:tag",
            "Detected the request for finding of creation "
                << "of a linked notebook model item for empty linked notebook "
                << "guid");
        return *m_pAllTagsRootItem;
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);

    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model:tag",
            "Found existing linked notebook model item for "
                << "linked notebook guid " << linkedNotebookGuid);
        return linkedNotebookItemIt.value();
    }

    QNTRACE(
        "model:tag",
        "Found no existing linked notebook item corresponding "
            << "to linked notebook guid " << linkedNotebookGuid
            << ", will create "
            << "one");

    auto linkedNotebookOwnerUsernameIt =
        m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(
            linkedNotebookGuid);

    if (Q_UNLIKELY(
            linkedNotebookOwnerUsernameIt ==
            m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end()))
    {
        QNDEBUG(
            "model:tag",
            "Found no linked notebook owner's username "
                << "for linked notebook guid " << linkedNotebookGuid);

        linkedNotebookOwnerUsernameIt =
            m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(
                linkedNotebookGuid, QString());
    }

    const QString & linkedNotebookOwnerUsername =
        linkedNotebookOwnerUsernameIt.value();

    TagLinkedNotebookRootItem linkedNotebookItem(
        linkedNotebookOwnerUsername, linkedNotebookGuid);

    linkedNotebookItemIt =
        m_linkedNotebookItems.insert(linkedNotebookGuid, linkedNotebookItem);

    auto * pLinkedNotebookItem = &(linkedNotebookItemIt.value());
    QNTRACE("model:tag", "Linked notebook root item: " << *pLinkedNotebookItem);

    int row = rowForNewItem(*m_pAllTagsRootItem, *pLinkedNotebookItem);
    beginInsertRows(indexForItem(m_pAllTagsRootItem), row, row);
    m_pAllTagsRootItem->insertChild(row, pLinkedNotebookItem);
    endInsertRows();

    return *pLinkedNotebookItem;
}

void TagModel::checkAndRemoveEmptyLinkedNotebookRootItem(
    ITagModelItem & modelItem)
{
    if (modelItem.type() != ITagModelItem::Type::LinkedNotebook) {
        return;
    }

    auto * pLinkedNotebookItem = modelItem.cast<TagLinkedNotebookRootItem>();
    if (!pLinkedNotebookItem) {
        return;
    }

    if (modelItem.hasChildren()) {
        return;
    }

    QNTRACE(
        "model:tag",
        "Removed the last child from the linked notebook "
            << "root item, will remove that item as well");

    removeModelItemFromParent(modelItem);

    QString linkedNotebookGuid = pLinkedNotebookItem->linkedNotebookGuid();

    auto indexIt =
        m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);

    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        Q_UNUSED(m_linkedNotebookItems.erase(linkedNotebookItemIt))
    }
}

void TagModel::checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem)
{
    QNTRACE(
        "model:tag",
        "TagModel::checkAndFindLinkedNotebookRestrictions: " << tagItem);

    const QString & linkedNotebookGuid = tagItem.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        QNTRACE("model:tag", "No linked notebook guid");
        return;
    }

    auto restrictionsIt =
        m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (restrictionsIt != m_tagRestrictionsByLinkedNotebookGuid.end()) {
        QNTRACE(
            "model:tag",
            "Already have the tag restrictions for linked "
                << "notebook guid " << linkedNotebookGuid);
        return;
    }

    auto it = m_findNotebookRequestForLinkedNotebookGuid.left.find(
        linkedNotebookGuid);

    if (it != m_findNotebookRequestForLinkedNotebookGuid.left.end()) {
        QNTRACE(
            "model:tag",
            "Already emitted the request to find tag "
                << "restrictions for linked notebook guid "
                << linkedNotebookGuid);
        return;
    }

    auto requestId = QUuid::createUuid();

    m_findNotebookRequestForLinkedNotebookGuid.insert(
        LinkedNotebookGuidWithFindNotebookRequestIdBimap::value_type(
            linkedNotebookGuid, requestId));

    Notebook notebook;
    notebook.unsetLocalUid();
    notebook.setLinkedNotebookGuid(linkedNotebookGuid);

    QNTRACE(
        "model:tag",
        "Emitted the request to find notebook by linked "
            << "notebook guid: " << linkedNotebookGuid
            << ", for the purpose of finding the tag restrictions; "
            << "request id = " << requestId);

    Q_EMIT findNotebook(notebook, requestId);
}

bool TagModel::tagItemMatchesByLinkedNotebook(
    const TagItem & item, const QString & linkedNotebookGuid) const
{
    if (linkedNotebookGuid.isNull()) {
        return true;
    }

    if (item.linkedNotebookGuid().isEmpty() != linkedNotebookGuid.isEmpty()) {
        return false;
    }

    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    if (item.linkedNotebookGuid() != linkedNotebookGuid) {
        return false;
    }

    return true;
}

void TagModel::fixupItemParent(ITagModelItem & item)
{
    auto * pParentItem = item.parent();
    if (pParentItem) {
        // No fixup is needed
        return;
    }

    checkAndCreateModelRootItems();

    if ((&item == m_pAllTagsRootItem) || (&item == m_pInvisibleRootItem)) {
        // No fixup is needed for these special items
        return;
    }

    auto * pLinkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (pLinkedNotebookItem) {
        setItemParent(item, *m_pAllTagsRootItem);
        return;
    }

    auto * pTagItem = item.cast<TagItem>();
    if (pTagItem) {
        const QString & parentTagLocalUid = pTagItem->parentLocalUid();
        if (!parentTagLocalUid.isEmpty()) {
            auto & localUidIndex = m_data.get<ByLocalUid>();
            auto it = localUidIndex.find(parentTagLocalUid);
            if (it != localUidIndex.end()) {
                auto * pParentItem = const_cast<TagItem *>(&(*it));
                setItemParent(item, *pParentItem);
            }
            else {
                QNDEBUG(
                    "model:tag",
                    "No tag corresponding to parent local uid "
                        << parentTagLocalUid << ", setting all tags root item "
                        << "as parent");
                setItemParent(item, *m_pAllTagsRootItem);
            }

            return;
        }

        const QString & linkedNotebookGuid = pTagItem->linkedNotebookGuid();
        if (!linkedNotebookGuid.isEmpty()) {
            auto * pLinkedNotebookItem =
                &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));

            setItemParent(item, *pLinkedNotebookItem);
            return;
        }
    }

    setItemParent(item, *m_pAllTagsRootItem);
}

void TagModel::setItemParent(ITagModelItem & item, ITagModelItem & parent)
{
    int row = rowForNewItem(parent, item);
    auto parentIndex = indexForItem(&parent);

    beginInsertRows(parentIndex, row, row);
    parent.insertChild(row, &item);
    endInsertRows();
}

void TagModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_pInvisibleRootItem)) {
        m_pInvisibleRootItem = new InvisibleRootItem;
        QNDEBUG("model:tag", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_pAllTagsRootItem)) {
        m_pAllTagsRootItem = new AllTagsRootItem;
        beginInsertRows(QModelIndex(), 0, 0);
        m_pAllTagsRootItem->setParent(m_pInvisibleRootItem);
        endInsertRows();
        QNDEBUG("model:tag", "Created all tags root item");
    }
}

#define MODEL_ITEM_NAME(item, itemName)                                        \
    if (item.type() == ITagModelItem::Type::Tag) {                             \
        const auto * pTagItem = item.cast<TagItem>();                          \
        if (pTagItem) {                                                        \
            itemName = pTagItem->nameUpper();                                  \
        }                                                                      \
    }                                                                          \
    else if (item.type() == ITagModelItem::Type::LinkedNotebook) {             \
        const auto * pLinkedNotebookItem =                                     \
            item.cast<TagLinkedNotebookRootItem>();                            \
        if (pLinkedNotebookItem) {                                             \
            itemName = pLinkedNotebookItem->username().toUpper();              \
        }                                                                      \
    }

bool TagModel::LessByName::operator()(
    const ITagModelItem & lhs, const ITagModelItem & rhs) const
{
    if ((lhs.type() == ITagModelItem::Type::AllTagsRoot) &&
        (rhs.type() != ITagModelItem::Type::AllTagsRoot))
    {
        return false;
    }
    else if (
        (lhs.type() != ITagModelItem::Type::AllTagsRoot) &&
        (rhs.type() == ITagModelItem::Type::AllTagsRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs.type() == ITagModelItem::Type::LinkedNotebook) &&
        (rhs.type() != ITagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if (
        (lhs.type() != ITagModelItem::Type::LinkedNotebook) &&
        (rhs.type() == ITagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool TagModel::LessByName::operator()(
    const ITagModelItem * pLhs, const ITagModelItem * pRhs) const
{
    if (!pLhs) {
        return true;
    }
    else if (!pRhs) {
        return false;
    }
    else {
        return this->operator()(*pLhs, *pRhs);
    }
}

bool TagModel::GreaterByName::operator()(
    const ITagModelItem & lhs, const ITagModelItem & rhs) const
{
    if ((lhs.type() == ITagModelItem::Type::AllTagsRoot) &&
        (rhs.type() != ITagModelItem::Type::AllTagsRoot))
    {
        return false;
    }
    else if (
        (lhs.type() != ITagModelItem::Type::AllTagsRoot) &&
        (rhs.type() == ITagModelItem::Type::AllTagsRoot))
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if ((lhs.type() == ITagModelItem::Type::LinkedNotebook) &&
        (rhs.type() != ITagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if (
        (lhs.type() != ITagModelItem::Type::LinkedNotebook) &&
        (rhs.type() == ITagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool TagModel::GreaterByName::operator()(
    const ITagModelItem * pLhs, const ITagModelItem * pRhs) const
{
    if (!pLhs) {
        return true;
    }
    else if (!pRhs) {
        return false;
    }
    else {
        return this->operator()(*pLhs, *pRhs);
    }
}

TagModel::RemoveRowsScopeGuard::RemoveRowsScopeGuard(TagModel & model) :
    m_model(model)
{
    m_model.beginRemoveTags();
}

TagModel::RemoveRowsScopeGuard::~RemoveRowsScopeGuard()
{
    m_model.endRemoveTags();
}

////////////////////////////////////////////////////////////////////////////////

QDebug & operator<<(QDebug & dbg, const TagModel::Column column)
{
    using Column = TagModel::Column;

    switch (column) {
    case Column::Name:
        dbg << "name";
        break;
    case Column::Synchronizable:
        dbg << "synchronizable";
        break;
    case Column::Dirty:
        dbg << "dirty";
        break;
    case Column::FromLinkedNotebook:
        dbg << "from linked notebook";
        break;
    case Column::NoteCount:
        dbg << "note count";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(column) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
