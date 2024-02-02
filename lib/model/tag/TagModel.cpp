/*
 * Copyright 2016-2024 Dmitry Ivanov
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
#include "InvisibleTagRootItem.h"

#include <lib/exception/Utils.h>
#include <lib/model/common/NewItemNameGenerator.hpp>

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/SuppressWarnings.h>
#include <quentier/utility/UidGenerator.h>

#include <QByteArray>
#include <QDataStream>
#include <QMimeData>

#include <algorithm>
#include <limits>
#include <vector>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT             (100)
#define LINKED_NOTEBOOK_LIST_LIMIT (40)

#define REPORT_ERROR(error, ...)                                               \
    ErrorString errorDescription{error};                                       \
    QNWARNING("model::TagModel", errorDescription << "" __VA_ARGS__);          \
    Q_EMIT notifyError(std::move(errorDescription))

#define REPORT_INFO(info, ...)                                                 \
    ErrorString errorDescription(info);                                        \
    QNINFO("model::TagModel", errorDescription << "" __VA_ARGS__);             \
    Q_EMIT notifyError(std::move(errorDescription))

namespace quentier {

namespace {

constexpr int gTagModelColumnCount = 5;

const QString gMimeType =
    QStringLiteral("application/x-com.quentier.tagmodeldatalist");

} // namespace

TagModel::TagModel(
    Account account, local_storage::ILocalStoragePtr localStorage,
    TagCache & cache, QObject * parent) :
    AbstractItemModel{std::move(account), parent},
    m_localStorage{std::move(localStorage)}, m_cache{cache}
{
    connectToLocalStorageEvents(m_localStorage->notifier());

    requestTagsList();
    requestLinkedNotebooksList();
}

TagModel::~TagModel()
{
    delete m_allTagsRootItem;
    delete m_invisibleRootItem;
}

bool TagModel::allTagsListed() const noexcept
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
}

void TagModel::favoriteTag(const QModelIndex & index)
{
    QNDEBUG(
        "model::TagModel",
        "TagModel::favoriteTag: index: is valid = "
            << (index.isValid() ? "true" : "false") << ", row = " << index.row()
            << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setTagFavorited(index, true);
}

void TagModel::unfavoriteTag(const QModelIndex & index)
{
    QNDEBUG(
        "model::TagModel",
        "TagModel::unfavoriteTag: index: is valid = "
            << (index.isValid() ? "true" : "false") << ", row = " << index.row()
            << ", column = " << index.column()
            << ", internal id = " << index.internalId());

    setTagFavorited(index, false);
}

bool TagModel::tagHasSynchronizedChildTags(const QString & tagLocalId) const
{
    QNTRACE(
        "model::TagModel",
        "TagModel::tagHasSynchronizedChildTags: tag "
            << "local id = " << tagLocalId);

    const auto & parentLocalIdIndex = m_data.get<ByParentLocalId>();
    auto range = parentLocalIdIndex.equal_range(tagLocalId);

    // Breadth-first search: first check each immediate child's guid
    for (auto it = range.first; it != range.second; ++it) {
        if (!it->guid().isEmpty()) {
            return true;
        }
    }

    // Now check each child's own child tags
    for (auto it = range.first; it != range.second; ++it) {
        if (tagHasSynchronizedChildTags(it->localId())) {
            return true;
        }
    }

    return false;
}

QString TagModel::localIdForItemName(
    const QString & itemName, const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::TagModel",
        "TagModel::localIdForItemName: name = "
            << itemName << ", linked notebook guid = " << linkedNotebookGuid);

    QModelIndex index = indexForTagName(itemName, linkedNotebookGuid);
    const auto * item = itemForIndex(index);
    if (!item) {
        QNTRACE("model::TagModel", "No tag with such name was found");
        return {};
    }

    const auto * tagItem = item->cast<TagItem>();
    if (!tagItem) {
        QNTRACE("model::TagModel", "Tag model item is not of tag type");
        return {};
    }

    return tagItem->localId();
}

QModelIndex TagModel::indexForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (it == localIdIndex.end()) {
        return {};
    }

    const auto & item = *it;
    return indexForItem(&item);
}

QString TagModel::itemNameForLocalId(const QString & localId) const
{
    QNTRACE("model::TagModel", "TagModel::itemNameForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model::TagModel", "No tag item with such local id");
        return {};
    }

    return it->name();
}

AbstractItemModel::ItemInfo TagModel::itemInfoForLocalId(
    const QString & localId) const
{
    QNTRACE("model::TagModel", "TagModel::itemInfoForLocalId: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(localId);
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNTRACE("model::TagModel", "No tag item with such local id");
        return {};
    }

    AbstractItemModel::ItemInfo info;
    info.m_localId = it->localId();
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

QList<AbstractItemModel::LinkedNotebookInfo> TagModel::linkedNotebooksInfo()
    const
{
    QList<LinkedNotebookInfo> infos;
    infos.reserve(m_linkedNotebookItems.size());
    for (const auto it:
         qevercloud::toRange(m_linkedNotebookItems))
    {
        infos.push_back(LinkedNotebookInfo(it.key(), it.value().username()));
    }

    return infos;
}

QString TagModel::linkedNotebookUsername(
    const QString & linkedNotebookGuid) const
{
    if (const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
        it != m_linkedNotebookItems.end())
    {
        const auto & item = it.value();
        return item.username();
    }

    return {};
}

bool TagModel::allItemsListed() const noexcept
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
}

QModelIndex TagModel::allItemsRootItemIndex() const
{
    if (Q_UNLIKELY(!m_allTagsRootItem)) {
        return {};
    }

    return indexForItem(m_allTagsRootItem);
}

QString TagModel::localIdForItemIndex(const QModelIndex & index) const
{
    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        return {};
    }

    auto * tagItem = modelItem->cast<TagItem>();
    if (tagItem) {
        return tagItem->localId();
    }

    return {};
}

QString TagModel::linkedNotebookGuidForItemIndex(
    const QModelIndex & index) const
{
    auto * modelItem = itemForIndex(index);
    if (!modelItem) {
        return {};
    }

    auto * linkedNotebookItem = modelItem->cast<TagLinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        return linkedNotebookItem->linkedNotebookGuid();
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

    if (index.column() == static_cast<int>(Column::Dirty) ||
        index.column() == static_cast<int>(Column::FromLinkedNotebook))
    {
        return indexFlags;
    }

    const auto * item = itemForIndex(index);
    if (Q_UNLIKELY(!item)) {
        return indexFlags;
    }

    const auto * tagItem = item->cast<TagItem>();
    if (!tagItem) {
        return indexFlags;
    }

    if (!canUpdateTagItem(*tagItem)) {
        return indexFlags;
    }

    if (index.column() == static_cast<int>(Column::Synchronizable)) {
        QModelIndex parentIndex = index;

        while (true) {
            const auto * parentItem = itemForIndex(parentIndex);
            if (Q_UNLIKELY(!parentItem)) {
                break;
            }

            if ((parentItem == m_allTagsRootItem) ||
                (parentItem == m_invisibleRootItem))
            {
                break;
            }

            const auto * parentTagItem = parentItem->cast<TagItem>();
            if (!parentTagItem) {
                return indexFlags;
            }

            if (parentTagItem->isSynchronizable()) {
                return indexFlags;
            }

            if (!canUpdateTagItem(*parentTagItem)) {
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

    const int columnIndex = index.column();
    if (columnIndex < 0 || columnIndex >= gTagModelColumnCount) {
        return {};
    }

    const auto * item = itemForIndex(index);
    if (!item) {
        return {};
    }

    if (item == m_invisibleRootItem) {
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
        return dataImpl(*item, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*item, column);
    default:
        return {};
    }
}

QVariant TagModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation != Qt::Horizontal) {
        return {};
    }

    return columnName(static_cast<Column>(section));
}

int TagModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    const auto * parentItem = itemForIndex(parent);
    return (parentItem ? parentItem->childrenCount() : 0);
}

int TagModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != static_cast<int>(Column::Name)))
    {
        return 0;
    }

    return gTagModelColumnCount;
}

QModelIndex TagModel::index(
    int row, int column, const QModelIndex & parent) const
{
    if ((row < 0) || (column < 0) || (column >= gTagModelColumnCount) ||
        (parent.isValid() &&
         (parent.column() != static_cast<int>(Column::Name))))
    {
        return {};
    }

    const auto * parentItem = itemForIndex(parent);
    if (!parentItem) {
        return {};
    }

    const auto * item = parentItem->childAtRow(row);
    if (!item) {
        return {};
    }

    const auto id = idForItem(*item);
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

    const auto * childItem = itemForIndex(index);
    if (!childItem) {
        return {};
    }

    const auto * parentItem = childItem->parent();
    if (!parentItem) {
        return {};
    }

    if (parentItem == m_invisibleRootItem) {
        return {};
    }

    if (parentItem == m_allTagsRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allTagsRootItemIndexId);
    }

    const auto * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        return {};
    }

    const int row = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::TagModel",
            "Internal inconsistency detected in TagModel: "
                << "parent of the item can't find the item "
                << "within its children: item = " << *parentItem
                << "\nParent item: " << *grandParentItem);
        return {};
    }

    const auto id = idForItem(*parentItem);
    if (Q_UNLIKELY(id == 0)) {
        return {};
    }

    return createIndex(row, static_cast<int>(Column::Name), id);
}

bool TagModel::setHeaderData(
    [[maybe_unused]] int section, [[maybe_unused]] Qt::Orientation orientation,
    [[maybe_unused]] const QVariant & value, [[maybe_unused]] int role)
{
    return false;
}

bool TagModel::setData(
    const QModelIndex & modelIndex, const QVariant & value, int role)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::setData: row = "
            << modelIndex.row() << ", column = " << modelIndex.column()
            << ", internal id = " << modelIndex.internalId()
            << ", value = " << value << ", role = " << role);

    if (role != Qt::EditRole) {
        QNDEBUG("model::TagModel", "Non-edit role, skipping");
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG("model::TagModel", "The model index is invalid, skipping");
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

    auto * item = itemForIndex(modelIndex);
    if (!item) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: no tag model item found for "
                       "model index"));
        return false;
    }

    if (Q_UNLIKELY(item == m_invisibleRootItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set data for the invisible root item "
                       "within the tag model"));
        return false;
    }

    auto * tagItem = item->cast<TagItem>();
    if (!tagItem) {
        QNDEBUG("model::TagModel", "The model index points to a non-tag item");
        return false;
    }

    if (!canUpdateTagItem(*tagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't update the tag, restrictions apply"));
        return false;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();

    bool shouldMakeParentsSynchronizable = false;

    TagItem tagItemCopy{*tagItem};
    bool dirty = tagItemCopy.isDirty();
    switch (static_cast<Column>(modelIndex.column())) {
    case Column::Name:
    {
        const QString newName = value.toString().trimmed();
        const bool changed = (newName != tagItemCopy.name());
        if (!changed) {
            QNDEBUG("model::TagModel", "Tag name hasn't changed");
            return true;
        }

        const auto nameIt = nameIndex.find(newName.toUpper());
        if (nameIt != nameIndex.end()) {
            ErrorString error{
                QT_TR_NOOP("Can't change tag name: no two tags within "
                           "the account are allowed to have the same name in "
                           "a case-insensitive manner")};
            QNINFO(
                "model::TagModel", error << ", suggested name = " << newName);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        ErrorString errorDescription;
        if (!validateTagName(newName, &errorDescription)) {
            ErrorString error{QT_TR_NOOP("Can't change tag name")};
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNINFO("model::TagModel", error << "; suggested name = " <<
                   newName);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        dirty = true;
        tagItemCopy.setName(newName);
        break;
    }
    case Column::Synchronizable:
    {
        if (m_account.type() == Account::Type::Local) {
            ErrorString error{
                QT_TR_NOOP("Can't make tag synchronizable within a local "
                           "account")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        if (tagItemCopy.isSynchronizable() && !value.toBool()) {
            ErrorString error{
                QT_TR_NOOP("Can't make an already synchronizable "
                           "tag not synchronizable")};
            QNINFO(
                "model::TagModel",
                error << ", already synchronizable tag item: " << tagItemCopy);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        dirty |= (value.toBool() != tagItemCopy.isSynchronizable());
        tagItemCopy.setSynchronizable(value.toBool());
        shouldMakeParentsSynchronizable = true;
        break;
    }
    default:
        QNINFO(
            "model::TagModel",
            "Can't edit data for column " << modelIndex.column()
                                          << " in the tag model");
        return false;
    }

    tagItemCopy.setDirty(dirty);

    TagDataByLocalId & index = m_data.get<ByLocalId>();

    if (shouldMakeParentsSynchronizable) {
        QNDEBUG(
            "model::TagModel",
            "Making the parents of the tag made synchronizable also "
                << "synchronizable");

        auto * processedItem = item;
        TagItem dummy;
        while (processedItem->parent()) {
            auto * parentItem = processedItem->parent();
            if (parentItem == m_invisibleRootItem) {
                break;
            }

            if (parentItem == m_allTagsRootItem) {
                break;
            }

            const auto * parentTagItem = parentItem->cast<TagItem>();
            if (!parentTagItem) {
                break;
            }

            if (parentTagItem->isSynchronizable()) {
                break;
            }

            dummy = *parentTagItem;
            dummy.setSynchronizable(true);
            const auto dummyIt = index.find(dummy.localId());
            if (Q_UNLIKELY(dummyIt == index.end())) {
                ErrorString error{
                    QT_TR_NOOP("Can't find one of currently made "
                               "synchronizable tag's parent tags")};
                QNWARNING("model::TagModel", error << ", item: " << dummy);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            index.replace(dummyIt, dummy);
            auto changedIndex = indexForLocalId(dummy.localId());
            if (Q_UNLIKELY(!changedIndex.isValid())) {
                ErrorString error{
                    QT_TR_NOOP("Can't get valid model index for one of "
                               "currently made synchronizable tag's parent "
                               "tags")};
                QNWARNING(
                    "model::TagModel",
                    error << ", item for which the index "
                          << "was requested: " << dummy);
                Q_EMIT notifyError(std::move(error));
                return false;
            }

            changedIndex = this->index(
                changedIndex.row(), static_cast<int>(Column::Synchronizable),
                changedIndex.parent());

            Q_EMIT dataChanged(changedIndex, changedIndex);
            processedItem = parentItem;
        }
    }

    const auto it = index.find(tagItemCopy.localId());
    if (Q_UNLIKELY(it == index.end())) {
        ErrorString error{QT_TR_NOOP("Can't find the tag being modified")};
        QNWARNING(
            "model::TagModel", error << " by its local id , item: " << tagItemCopy);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    index.replace(it, tagItemCopy);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    if (m_sortedColumn == Column::Name) {
        updateItemRowWithRespectToSorting(*item);
    }

    updateTagInLocalStorage(tagItemCopy);

    QNDEBUG("model::TagModel", "Successfully set the data");
    return true;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::insertRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    checkAndCreateModelRootItems();

    auto * parentItem =
        (parent.isValid() ? itemForIndex(parent) : m_invisibleRootItem);

    if (Q_UNLIKELY(!parentItem)) {
        QNWARNING(
            "model::TagModel",
            "Can't insert row into the tag model: can't "
                << "find parent item per model index");
        return false;
    }

    if ((parentItem != m_allTagsRootItem) && !canCreateTagItem(*parentItem)) {
        QNINFO(
            "model::TagModel",
            "Can't insert row into under the tag model item: "
                << "restrictions apply: " << *parentItem);
        return false;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const int numExistingTags = static_cast<int>(localIdIndex.size());
    if (Q_UNLIKELY(numExistingTags + count >= m_account.tagCountMax())) {
        ErrorString error{
            QT_TR_NOOP("Can't create tag(s): the account can "
                       "contain a limited number of tags")};
        error.details() = QString::number(m_account.tagCountMax());
        QNINFO("model::TagModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
    }

    std::vector<TagDataByLocalId::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        // Adding tag item
        TagItem item;
        item.setLocalId(UidGenerator::Generate());
        m_tagItemsNotYetInLocalStorageIds.insert(item.localId());

        item.setName(nameForNewTag(QString{}));
        item.setDirty(true);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        const auto insertionResult = localIdIndex.insert(item);
        addedItems.push_back(insertionResult.first);
    }
    endInsertRows();

    if (m_sortedColumn == Column::Name) {
        Q_EMIT layoutAboutToBeChanged();

        for (const auto & it: addedItems) {
            if (const auto tagModelItemIt = localIdIndex.find(it->localId());
                tagModelItemIt != localIdIndex.end())
            {
                updateItemRowWithRespectToSorting(const_cast<TagItem &>(*it));
            }
        }

        Q_EMIT layoutChanged();
    }

    for (const auto & it: addedItems) {
        updateTagInLocalStorage(*it);
    }

    QNDEBUG("model::TagModel", "Successfully inserted the rows");
    return true;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::removeRows: row = "
            << row << ", count = " << count << ", parent index: row = "
            << parent.row() << ", column = " << parent.column()
            << ", internal id = " << parent.internalId());

    [[maybe_unused]] RemoveRowsScopeGuard removeRowsScopeGuard{*this};

    checkAndCreateModelRootItems();

    auto * parentItem =
        (parent.isValid() ? itemForIndex(parent) : m_invisibleRootItem);

    if (!parentItem) {
        QNDEBUG("model::TagModel", "No item corresponding to parent index");
        return false;
    }

    // First need to check if the rows to be removed are allowed to be removed
    for (int i = 0; i < count; ++i) {
        auto * modelItem = parentItem->childAtRow(row + i);
        if (!modelItem) {
            QNWARNING(
                "model::TagModel",
                "Detected null pointer to child tag item "
                    << "on attempt to remove row " << (row + i)
                    << " from parent item: " << *parentItem);
            continue;
        }

        const auto * tagItem = modelItem->cast<TagItem>();
        if (!tagItem) {
            ErrorString error{
                QT_TR_NOOP("Can't remove a non-tag item from tag model")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        if (!tagItem->linkedNotebookGuid().isEmpty()) {
            ErrorString error{
                QT_TR_NOOP("Can't remove tag from a linked notebook")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        if (!tagItem->guid().isEmpty()) {
            ErrorString error{
                QT_TR_NOOP("Can't remove tag already synchronized with "
                           "Evernote")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        if (tagHasSynchronizedChildTags(tagItem->localId())) {
            ErrorString error{
                QT_TR_NOOP("Can't remove tag which has some child "
                           "tags already synchronized with Evernote")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    /**
     * Need to re-parent all children of each removed item to the parent of
     * the removed items i.e. to make the grand-parent of each child its new
     * parent. But before that will just take them away from the current parent
     * and ollect into a temporary list
     */
    QList<ITagModelItem *> removedItemsChildren;
    for (int i = 0; i < count; ++i) {
        auto * modelItem = parentItem->childAtRow(row + i);
        if (Q_UNLIKELY(!modelItem)) {
            QNWARNING(
                "model::TagModel",
                "Detected null pointer to tag model item "
                    << "within the items to be removed");
            continue;
        }

        auto modelItemIndex = indexForItem(modelItem);
        while (modelItem->hasChildren()) {
            beginRemoveRows(modelItemIndex, 0, 0);
            auto * childItem = modelItem->takeChild(0);
            endRemoveRows();

            if (Q_UNLIKELY(!childItem)) {
                continue;
            }

            const auto * childTagItem = childItem->cast<TagItem>();
            if (Q_UNLIKELY(!childTagItem)) {
                continue;
            }

            TagItem childItemCopy(*childTagItem);

            auto * parentTagItem = parentItem->cast<TagItem>();
            if (parentTagItem) {
                childItemCopy.setParentGuid(parentTagItem->guid());
                childItemCopy.setParentLocalId(parentTagItem->localId());
            }
            else {
                childItemCopy.setParentGuid(QString());
                childItemCopy.setParentLocalId(QString());
            }

            childItemCopy.setDirty(true);

            const auto tagItemIt = localIdIndex.find(childItemCopy.localId());
            if (Q_UNLIKELY(tagItemIt == localIdIndex.end())) {
                QNINFO(
                    "model::TagModel",
                    "The tag item which parent is being removed was not found "
                        << "within the model. Adding it there");
                Q_UNUSED(localIdIndex.insert(childItemCopy))
            }
            else {
                localIdIndex.replace(tagItemIt, childItemCopy);
            }

            updateTagInLocalStorage(childItemCopy);

            // NOTE: no dataChanged signal here because the corresponding model
            // item is now parentless and hence is unavailable to the view
            removedItemsChildren << childItem;
        }
    }

    // Actually remove the rows each of which has no children anymore
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        auto * modelItem = parentItem->takeChild(row);
        if (Q_UNLIKELY(!modelItem)) {
            continue;
        }

        auto * tagItem = modelItem->cast<TagItem>();
        if (Q_UNLIKELY(!tagItem)) {
            continue;
        }

        QNTRACE(
            "model::TagModel",
            "Expunging tag from local storage: tag local id: "
                << tagItem->localId());

        auto expungeTagFuture = m_localStorage->expungeTagByLocalId(
            tagItem->localId());

        threading::onFailed(
            std::move(expungeTagFuture), this,
            [this, localId = tagItem->localId()](const QException & e) {
                auto message = exceptionMessage(e);
                ErrorString error{QT_TR_NOOP(
                    "Failed to expunge tag from local storage")};
                error.appendBase(message.base());
                error.appendBase(message.additionalBases());
                error.details() = message.details();
                QNWARNING(
                    "model::TagModel", error << "; tag local id = " << localId);
                Q_EMIT notifyError(std::move(error));
            });

        const auto tagLocalId = tagItem->localId();
        if (const auto it = localIdIndex.find(tagItem->localId());
            it != localIdIndex.end())
        {
            localIdIndex.erase(it);
        }

        if (const auto indexIt = m_indexIdToLocalIdBimap.right.find(tagLocalId);
            indexIt != m_indexIdToLocalIdBimap.right.end())
        {
            m_indexIdToLocalIdBimap.right.erase(indexIt);
        }
    }
    endRemoveRows();

    /**
     * Insert previously collected children of the removed item under its parent
     */
    while (!removedItemsChildren.isEmpty()) {
        auto * childItem = removedItemsChildren.takeAt(0);
        if (Q_UNLIKELY(!childItem)) {
            continue;
        }

        const int newRow = rowForNewItem(*parentItem, *childItem);
        beginInsertRows(parent, newRow, newRow);
        parentItem->insertChild(newRow, childItem);
        endInsertRows();
    }

    QNDEBUG("model::TagModel", "Successfully removed row(s)");
    return true;
}

void TagModel::sort(int column, Qt::SortOrder order)
{
    QNTRACE(
        "model::TagModel",
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
            "model::TagModel",
            "The sort order already established, nothing to do");
        return;
    }

    m_sortOrder = order;
    Q_EMIT sortingChanged();

    checkAndCreateModelRootItems();

    Q_EMIT layoutAboutToBeChanged();

    if (m_sortOrder == Qt::AscendingOrder) {
        auto & localIdIndex = m_data.get<ByLocalId>();
        for (auto & item: localIdIndex) {
            const_cast<TagItem &>(item).sortChildren(LessByName{});
        }

        for (const auto it: qevercloud::toRange(m_linkedNotebookItems)) {
            it.value().sortChildren(LessByName{});
        }

        m_allTagsRootItem->sortChildren(LessByName{});
    }
    else {
        auto & localIdIndex = m_data.get<ByLocalId>();
        for (auto & item: localIdIndex) {
            const_cast<TagItem &>(item).sortChildren(GreaterByName{});
        }

        for (auto it = m_linkedNotebookItems.begin();
             it != m_linkedNotebookItems.end(); ++it)
        {
            it.value().sortChildren(GreaterByName{});
        }

        m_allTagsRootItem->sortChildren(GreaterByName{});
    }

    updatePersistentModelIndices();
    Q_EMIT layoutChanged();

    QNDEBUG("model::TagModel", "Successfully sorted the tag model");
}

QStringList TagModel::mimeTypes() const
{
    QStringList list;
    list << gMimeType;
    return list;
}

QMimeData * TagModel::mimeData(const QModelIndexList & indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }

    const auto * modelItem = itemForIndex(indexes.at(0));
    if (!modelItem) {
        return nullptr;
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        return nullptr;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *tagItem;

    auto mimeData = std::make_unique<QMimeData>();
    mimeData->setData(
        gMimeType,
        qCompress(encodedItem, 9));

    return mimeData.release();
}

bool TagModel::dropMimeData(
    const QMimeData * mimeData, Qt::DropAction action, int row, int column,
    const QModelIndex & parentIndex)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::dropMimeData: action = "
            << action << ", row = " << row << ", column = " << column
            << ", parent index: is valid = "
            << (parentIndex.isValid() ? "true" : "false")
            << ", parent row = " << parentIndex.row()
            << ", parent column = " << (parentIndex.column())
            << ", parent internal id: " << parentIndex.internalId()
            << ", mime data formats: "
            << (mimeData ? mimeData->formats().join(QStringLiteral("; "))
                          : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!mimeData || !mimeData->hasFormat(gMimeType)) {
        return false;
    }

    if (!parentIndex.isValid()) {
        // Invalid index corresponds to invisible root item and it's forbidden
        // to drop items under it. Tags without parent tags which don't belong
        // to linked notebooks need to be dropped under all tags root item.
        return false;
    }

    checkAndCreateModelRootItems();

    auto * newParentItem = itemForIndex(parentIndex);
    if (!newParentItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error, can't drop tag: no new parent "
                       "item was found"));
        return false;
    }

    if (newParentItem != m_allTagsRootItem &&
        !canCreateTagItem(*newParentItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move tag under the new parent: restrictions "
                       "apply or restrictions settings were not fetched yet"));
        return false;
    }

    QByteArray data = qUncompress(mimeData->data(gMimeType));
    QDataStream in(&data, QIODevice::ReadOnly);

    qint32 type;
    in >> type;

    if (type != static_cast<qint32>(ITagModelItem::Type::Tag)) {
        QNDEBUG(
            "model::TagModel",
            "Can only drag-drop tag model items of tag type");
        return false;
    }

    TagItem droppedTagItem;
    in >> droppedTagItem;

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(droppedTagItem.localId());
    if (it == localIdIndex.end()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: failed to find the notebook being "
                       "dropped in the notebook model"));
        return false;
    }

    auto * tagItem = const_cast<TagItem *>(&(*it));

    if (!canUpdateTagItem(*tagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Cannot reparent tag, restrictions apply"));
        return false;
    }

    QString parentLinkedNotebookGuid;

    auto * parentLinkedNotebookItem =
        newParentItem->cast<TagLinkedNotebookRootItem>();
    if (parentLinkedNotebookItem) {
        parentLinkedNotebookGuid =
            parentLinkedNotebookItem->linkedNotebookGuid();
    }

    if (tagItem->linkedNotebookGuid() != parentLinkedNotebookGuid) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move tags between different linked notebooks or "
                       "between user's tags and those from linked notebooks"));
        return false;
    }

    // Check that we aren't trying to move the tag under one of its children
    const auto * trackedParentItem = newParentItem;
    while (trackedParentItem && trackedParentItem != m_allTagsRootItem) {
        const auto * trackedParentTagItem =
            trackedParentItem->cast<TagItem>();

        if (trackedParentTagItem &&
            (trackedParentTagItem->localId() != tagItem->localId()))
        {
            ErrorString error{
                QT_TR_NOOP("Can't move tag under one of its child tags")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return false;
        }

        trackedParentItem = trackedParentItem->parent();
    }

    if (newParentItem == tagItem->parent()) {
        QNDEBUG(
            "model::TagModel",
            "Item is already under the chosen parent, nothing to do");
        return true;
    }

    droppedTagItem = *tagItem;

    const auto * newParentTagItem = newParentItem->cast<TagItem>();
    if (newParentTagItem) {
        droppedTagItem.setParentLocalId(newParentTagItem->localId());
        droppedTagItem.setParentGuid(newParentTagItem->guid());
    }
    else {
        droppedTagItem.setParentLocalId(QString());
        droppedTagItem.setParentGuid(QString());
    }

    droppedTagItem.setDirty(true);

    if (row == -1) {
        row = parentIndex.row();
    }

    // Remove the tag model item from its original parent
    auto * originalParentItem = tagItem->parent();
    const int originalRow = originalParentItem->rowForChild(tagItem);
    if (originalRow >= 0) {
        auto originalParentIndex = indexForItem(originalParentItem);
        beginRemoveRows(originalParentIndex, originalRow, originalRow);
        Q_UNUSED(originalParentItem->takeChild(originalRow));
        endRemoveRows();
        checkAndRemoveEmptyLinkedNotebookRootItem(*originalParentItem);
    }

    beginInsertRows(parentIndex, row, row);
    localIdIndex.replace(it, droppedTagItem);
    newParentItem->insertChild(row, tagItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*tagItem);
    updateTagInLocalStorage(*tagItem);

    QModelIndex index = indexForLocalId(droppedTagItem.localId());
    Q_EMIT notifyTagParentChanged(index);
    return true;
}

void TagModel::onAddTagComplete(Tag tag, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onAddTagFailed: tag = " << tag << "\nError description = "
                                           << errorDescription
                                           << ", request id = " << requestId);

    Q_UNUSED(m_addTagRequestIds.erase(it))
    Q_EMIT notifyError(errorDescription);
    removeItemByLocalId(tag.localId());
}

void TagModel::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onUpdateTagFailed: tag = "
            << tag << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_updateTagRequestIds.erase(it))
    requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagToRestoreFailedUpdateRequestIds.insert(requestId))

    QNTRACE(
        "model::TagModel",
        "Emitting the request to find a tag: local id = "
            << tag.localId() << ", request id = " << requestId);

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
        "model::TagModel",
        "TagModel::onFindTagComplete: tag = " << tag << "\nRequest id = "
                                              << requestId);

    if (restoreUpdateIt != m_findTagToRestoreFailedUpdateRequestIds.end()) {
        Q_UNUSED(
            m_findTagToRestoreFailedUpdateRequestIds.erase(restoreUpdateIt))

        onTagAddedOrUpdated(tag);
    }
    else if (performUpdateIt != m_findTagToPerformUpdateRequestIds.end()) {
        Q_UNUSED(m_findTagToPerformUpdateRequestIds.erase(performUpdateIt))
        m_cache.put(tag.localId(), tag);
        auto & localIdIndex = m_data.get<ByLocalId>();
        auto it = localIdIndex.find(tag.localId());
        if (it != localIdIndex.end()) {
            updateTagInLocalStorage(*it);
        }
    }
    else if (
        checkAfterErasureIt !=
        m_findTagAfterNotelessTagsErasureRequestIds.end())
    {
        QNDEBUG(
            "model::TagModel",
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
        "model::TagModel",
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
            "model::TagModel",
            "Tag no longer exists after the noteless tags "
                << "from linked notebooks erasure");

        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.erase(
            checkAfterErasureIt))

        removeItemByLocalId(tag.localId());
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
        "model::TagModel",
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
            "model::TagModel",
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
        "model::TagModel",
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
    Tag tag, QStringList expungedChildTagLocalIds, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onExpungeTagComplete: tag = "
            << tag << "\nExpunged child tag local ids: "
            << expungedChildTagLocalIds.join(QStringLiteral(", "))
            << ", request id = " << requestId);

    auto it = m_expungeTagRequestIds.find(requestId);
    if (it != m_expungeTagRequestIds.end()) {
        Q_UNUSED(m_expungeTagRequestIds.erase(it))
        return;
    }

    Q_EMIT aboutToRemoveTags();
    // NOTE: all child items would be removed from the model automatically
    removeItemByLocalId(tag.localId());
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
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onGetNoteCountPerTagComplete: tag = "
            << tag << "\nRequest id = " << requestId
            << ", note count = " << noteCount);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))
    setNoteCountForTag(tag.localId(), noteCount);
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
        "model::TagModel",
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
    QHash<QString, int> noteCountsPerTagLocalId,
    LocalStorageManager::NoteCountOptions options, QUuid requestId)
{
    Q_UNUSED(options)

    if (requestId != m_noteCountsPerAllTagsRequestId) {
        return;
    }

    QNTRACE(
        "model::TagModel",
        "TagModel::onGetNoteCountsPerAllTagsComplete: note "
            << "counts were received for " << noteCountsPerTagLocalId.size()
            << " tag local ids; request id = " << requestId);

    m_noteCountsPerAllTagsRequestId = QUuid();

    auto & localIdIndex = m_data.get<ByLocalId>();
    for (auto it = localIdIndex.begin(), end = localIdIndex.end(); it != end;
         ++it)
    {
        TagItem item = *it;
        auto noteCountIt = noteCountsPerTagLocalId.find(item.localId());
        if (noteCountIt != noteCountsPerTagLocalId.end()) {
            item.setNoteCount(noteCountIt.value());
        }
        else {
            item.setNoteCount(0);
        }

        localIdIndex.replace(it, item);

        const QString & parentLocalId = item.parentLocalId();
        const QString & linkedNotebookGuid = item.linkedNotebookGuid();
        if (parentLocalId.isEmpty() && linkedNotebookGuid.isEmpty()) {
            continue;
        }

        // If tag item has either parent tag or linked notebook local id,
        // we'll send dataChanged signal for it here; for all tags from user's
        // own account and without parent tags we'll send dataChanged signal
        // later, once for all such tags
        QModelIndex idx = indexForLocalId(item.localId());
        if (idx.isValid()) {
            idx = index(
                idx.row(), static_cast<int>(Column::NoteCount), idx.parent());

            Q_EMIT dataChanged(idx, idx);
        }
    }

    auto allTagsRootItemIndex = indexForItem(m_allTagsRootItem);

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
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete: "
            << "request id = " << requestId);

    auto & localIdIndex = m_data.get<ByLocalId>();

    for (const auto & item: localIdIndex) {
        if (item.linkedNotebookGuid().isEmpty()) {
            continue;
        }

        // The item's current note count per tag may be invalid due to
        // asynchronous events sequence, need to ask the database if such
        // an item actually exists
        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_findTagAfterNotelessTagsErasureRequestIds.insert(requestId))

        Tag tag;
        tag.setLocalId(item.localId());

        QNTRACE(
            "model::TagModel",
            "Emitting the request to find tag from linked "
                << "notebook to check for its existence: " << item.localId()
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
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onFindNotebookFailed: notebook = "
            << notebook << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    Q_UNUSED(m_findNotebookRequestForLinkedNotebookGuid.right.erase(it))
}

void TagModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onUpdateNotebookComplete: local id = "
            << notebook.localId());

    Q_UNUSED(requestId)
    updateRestrictionsFromNotebook(notebook);
}

void TagModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onExpungeNotebookComplete: local id = "
            << notebook.localId() << ", linked notebook guid = "
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
        "model::TagModel",
        "TagModel::onAddNoteComplete: note = " << note << "\nRequest id = "
                                               << requestId);

    if (Q_UNLIKELY(note.hasDeletionTimestamp())) {
        return;
    }

    if (!note.hasTagLocalIds()) {
        if (note.hasTagGuids()) {
            QNDEBUG(
                "model::TagModel",
                "The note has tag guids but not tag local "
                    << "uids, need to request the proper list of tags from "
                       "this "
                    << "note before their note counts can be updated");
            requestTagsPerNote(note);
        }
        else {
            QNDEBUG(
                "model::TagModel",
                "The note has no tags => no need to update "
                    << "the note count per any tag");
        }

        return;
    }

    const auto & tagLocalIds = note.tagLocalIds();
    for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
        Tag dummy;
        dummy.setLocalId(tagLocalId);
        requestNoteCountForTag(dummy);
    }
}

void TagModel::onNoteTagListChanged(
    QString noteLocalId, QStringList previousNoteTagLocalIds,
    QStringList newNoteTagLocalIds)
{
    QNDEBUG(
        "model::TagModel",
        "TagModel::onNoteTagListChanged: note local id = "
            << noteLocalId << ", previous note tag local ids = "
            << previousNoteTagLocalIds.join(QStringLiteral(","))
            << ", new note tag local ids = "
            << newNoteTagLocalIds.join(QStringLiteral(",")));

    std::sort(previousNoteTagLocalIds.begin(), previousNoteTagLocalIds.end());
    std::sort(newNoteTagLocalIds.begin(), newNoteTagLocalIds.end());

    std::vector<QString> commonTagLocalIds;

    std::set_intersection(
        previousNoteTagLocalIds.begin(), previousNoteTagLocalIds.end(),
        newNoteTagLocalIds.begin(), newNoteTagLocalIds.end(),
        std::back_inserter(commonTagLocalIds));

    auto & localIdIndex = m_data.get<ByLocalId>();

    for (const auto & tagLocalId: qAsConst(previousNoteTagLocalIds)) {
        auto commonIt = std::find(
            commonTagLocalIds.begin(), commonTagLocalIds.end(), tagLocalId);

        if (commonIt != commonTagLocalIds.end()) {
            continue;
        }

        auto itemIt = localIdIndex.find(tagLocalId);
        if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
            // Probably this tag was expunged
            QNDEBUG(
                "model::TagModel", "No tag was found in the model: " << tagLocalId);
            continue;
        }

        int noteCount = itemIt->noteCount();
        --noteCount;
        noteCount = std::max(0, noteCount);
        setNoteCountForTag(tagLocalId, noteCount);
    }

    for (const auto & tagLocalId: qAsConst(newNoteTagLocalIds)) {
        auto commonIt = std::find(
            commonTagLocalIds.begin(), commonTagLocalIds.end(), tagLocalId);

        if (commonIt != commonTagLocalIds.end()) {
            continue;
        }

        auto itemIt = localIdIndex.find(tagLocalId);
        if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
            // Probably this tag was expunged
            QNDEBUG(
                "model::TagModel", "No tag was found in the model: " << tagLocalId);
            continue;
        }

        int noteCount = itemIt->noteCount();
        ++noteCount;
        setNoteCountForTag(tagLocalId, noteCount);
    }
}

void TagModel::onExpungeNoteComplete(Note note, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onExpungeNoteComplete: note = " << note << "\nRequest id = "
                                                   << requestId);

    if (note.hasTagLocalIds()) {
        const auto & tagLocalIds = note.tagLocalIds();
        for (const auto & tagLocalId: qAsConst(tagLocalIds)) {
            Tag tag;
            tag.setLocalId(tagLocalId);
            requestNoteCountForTag(tag);
        }

        return;
    }

    QNDEBUG("model::TagModel", "Note has no tag local ids");
    requestNoteCountsPerAllTags();
}

void TagModel::onAddLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onAddLinkedNotebookComplete: request id = "
            << requestId << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onUpdateLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onUpdateLinkedNotebookComplete: request id = "
            << requestId << ", linked notebook: " << linkedNotebook);

    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onExpungeLinkedNotebookComplete(
    LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onExpungeLinkedNotebookComplete: request "
            << "id = " << requestId << ", linked notebook: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model::TagModel",
            "Received linked notebook expunged event but "
                << "the linked notebook has no guid: " << linkedNotebook
                << ", request id = " << requestId);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    QStringList expungedTagLocalIds;
    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);

    expungedTagLocalIds.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedTagLocalIds << it->localId();
    }

    for (const auto & tagLocalId: qAsConst(expungedTagLocalIds)) {
        removeItemByLocalId(tagLocalId);
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        auto * modelItem = &(linkedNotebookItemIt.value());
        auto * parentItem = modelItem->parent();
        if (parentItem) {
            int row = parentItem->rowForChild(modelItem);
            if (row >= 0) {
                QModelIndex parentItemIndex = indexForItem(parentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(parentItem->takeChild(row))
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
        "model::TagModel",
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
        "model::TagModel",
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
        "model::TagModel",
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
            "model::TagModel",
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
        "model::TagModel",
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
    QNTRACE("model::TagModel", "TagModel::createConnections");

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
        &LocalStorageManagerAsync::listTagsWithNoteLocalIdsFailed, this,
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
        "model::TagModel",
        "TagModel::requestTagsList: offset = " << m_listTagsOffset);

    LocalStorageManager::ListObjectsOptions flags =
        LocalStorageManager::ListObjectsOption::ListAll;

    auto order = LocalStorageManager::ListTagsOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;

    m_listTagsRequestId = QUuid::createUuid();

    QNTRACE(
        "model::TagModel",
        "Emitting the request to list tags: offset = "
            << m_listTagsOffset << ", request id = " << m_listTagsRequestId);

    Q_EMIT listTags(
        flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, {},
        m_listTagsRequestId);
}

void TagModel::requestNoteCountForTag(const Tag & tag)
{
    QNTRACE("model::TagModel", "TagModel::requestNoteCountForTag: " << tag);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerTagRequestIds.insert(requestId))

    QNTRACE(
        "model::TagModel",
        "Emitting the request to compute the number of notes "
            << "per tag, request id = " << requestId);

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountPerTag(tag, options, requestId);
}

void TagModel::requestTagsPerNote(const Note & note)
{
    QNTRACE("model::TagModel", "TagModel::requestTagsPerNote: " << note);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_listTagsPerNoteRequestIds.insert(requestId))

    QNTRACE(
        "model::TagModel",
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
    QNTRACE("model::TagModel", "TagModel::requestNoteCountsPerAllTags");

    m_noteCountsPerAllTagsRequestId = QUuid::createUuid();

    LocalStorageManager::NoteCountOptions options(
        LocalStorageManager::NoteCountOption::IncludeNonDeletedNotes);

    Q_EMIT requestNoteCountsForAllTags(
        options, m_noteCountsPerAllTagsRequestId);
}

void TagModel::requestLinkedNotebooksList()
{
    QNTRACE("model::TagModel", "TagModel::requestLinkedNotebooksList");

    auto order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    auto direction = LocalStorageManager::OrderDirection::Ascending;

    m_listLinkedNotebooksRequestId = QUuid::createUuid();

    QNTRACE(
        "model::TagModel",
        "Emitting the request to list linked notebooks: "
            << "offset = " << m_listLinkedNotebooksOffset
            << ", request id = " << m_listLinkedNotebooksRequestId);

    Q_EMIT listAllLinkedNotebooks(
        LINKED_NOTEBOOK_LIST_LIMIT, m_listLinkedNotebooksOffset, order,
        direction, m_listLinkedNotebooksRequestId);
}

void TagModel::onTagAddedOrUpdated(
    const Tag & tag, const QStringList * pTagNoteLocalIds)
{
    m_cache.put(tag.localId(), tag);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(tag.localId());
    bool newTag = (itemIt == localIdIndex.end());
    if (newTag) {
        Q_EMIT aboutToAddTag();

        onTagAdded(tag, pTagNoteLocalIds);

        QModelIndex addedTagIndex = indexForLocalId(tag.localId());
        Q_EMIT addedTag(addedTagIndex);
    }
    else {
        QModelIndex tagIndexBefore = indexForLocalId(tag.localId());
        Q_EMIT aboutToUpdateTag(tagIndexBefore);

        onTagUpdated(tag, itemIt, pTagNoteLocalIds);

        QModelIndex tagIndexAfter = indexForLocalId(tag.localId());
        Q_EMIT updatedTag(tagIndexAfter);
    }
}

void TagModel::onTagAdded(
    const Tag & tag, const QStringList * pTagNoteLocalIds)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onTagAdded: tag local id = "
            << tag.localId() << ", tag note local ids: "
            << (pTagNoteLocalIds
                    ? pTagNoteLocalIds->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    ITagModelItem * parentItem = nullptr;
    auto & localIdIndex = m_data.get<ByLocalId>();

    if (tag.hasParentLocalId()) {
        auto it = localIdIndex.find(tag.parentLocalId());
        if (it != localIdIndex.end()) {
            parentItem = const_cast<TagItem *>(&(*it));
        }
    }
    else if (tag.hasLinkedNotebookGuid()) {
        const QString & linkedNotebookGuid = tag.linkedNotebookGuid();
        parentItem =
            &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!parentItem) {
        checkAndCreateModelRootItems();
        parentItem = m_allTagsRootItem;
    }

    QModelIndex parentIndex = indexForItem(parentItem);

    TagItem item;
    tagToItem(tag, item);

    checkAndFindLinkedNotebookRestrictions(item);

    if (pTagNoteLocalIds) {
        item.setNoteCount(pTagNoteLocalIds->size());
    }

    auto insertionResult = localIdIndex.insert(item);
    auto it = insertionResult.first;
    auto * item = const_cast<TagItem *>(&(*it));

    int row = rowForNewItem(*parentItem, *item);

    beginInsertRows(parentIndex, row, row);
    parentItem->insertChild(row, item);
    endInsertRows();

    mapChildItems(*item);
}

void TagModel::onTagUpdated(
    const Tag & tag, TagDataByLocalId::iterator it,
    const QStringList * pTagNoteLocalIds)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onTagUpdated: tag local id = "
            << tag.localId() << ", tag note local ids: "
            << (pTagNoteLocalIds
                    ? pTagNoteLocalIds->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    TagItem itemCopy;
    tagToItem(tag, itemCopy);

    if (pTagNoteLocalIds) {
        itemCopy.setNoteCount(pTagNoteLocalIds->size());
    }

    auto * tagItem = const_cast<TagItem *>(&(*it));
    auto * parentItem = tagItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        // FIXME: should try to fix it automatically

        ErrorString error(
            QT_TR_NOOP("Tag model item being updated does not "
                       "have a parent item linked with it"));

        QNWARNING(
            "model::TagModel",
            error << ", tag: " << tag << "\nTag model item: " << *tagItem);

        Q_EMIT notifyError(error);
        return;
    }

    int row = parentItem->rowForChild(tagItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(
            QT_TR_NOOP("Can't find the row of the updated tag "
                       "item within its parent"));

        QNWARNING(
            "model::TagModel",
            error << ", tag: " << tag << "\nTag model item: " << *tagItem);

        Q_EMIT notifyError(error);
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();

    ITagModelItem * newParentItem = nullptr;
    if (tag.hasParentLocalId()) {
        auto parentIt = localIdIndex.find(tag.parentLocalId());
        if (parentIt != localIdIndex.end()) {
            newParentItem = const_cast<TagItem *>(&(*parentIt));
        }
    }
    else if (tag.hasLinkedNotebookGuid()) {
        newParentItem =
            &(findOrCreateLinkedNotebookModelItem(tag.linkedNotebookGuid()));
    }

    if (!newParentItem) {
        checkAndCreateModelRootItems();
        newParentItem = m_allTagsRootItem;
    }

    QModelIndex parentItemIndex = indexForItem(parentItem);

    QModelIndex newParentItemIndex =
        ((parentItem == newParentItem) ? parentItemIndex
                                         : indexForItem(newParentItem));

    // 1) Remove the original row from the parent
    beginRemoveRows(parentItemIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();

    // 2) Insert the replacement row
    if (parentItem != newParentItem) {
        row = 0;
    }

    beginInsertRows(newParentItemIndex, row, row);

    int numNotesPerTag = it->noteCount();
    itemCopy.setNoteCount(numNotesPerTag);

    Q_UNUSED(localIdIndex.replace(it, itemCopy))
    newParentItem->insertChild(row, tagItem);

    endInsertRows();

    QModelIndex modelIndexFrom = index(row, 0, newParentItemIndex);

    QModelIndex modelIndexTo =
        index(row, gTagModelColumnCount - 1, newParentItemIndex);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    // 3) Ensure all the child tag model items are properly located under this
    // tag model item
    QModelIndex modelItemIndex = indexForItem(tagItem);

    auto & parentLocalIdIndex = m_data.get<ByParentLocalId>();
    auto range = parentLocalIdIndex.equal_range(tagItem->localId());
    for (auto childIt = range.first; childIt != range.second; ++childIt) {
        const TagItem & childItem = *childIt;
        const QString & childItemLocalId = childItem.localId();

        auto childItemIt = localIdIndex.find(childItemLocalId);
        if (childItemIt != localIdIndex.end()) {
            auto & childItem = const_cast<TagItem &>(*childItemIt);

            int row = tagItem->rowForChild(&childItem);
            if (row >= 0) {
                continue;
            }

            row = rowForNewItem(*tagItem, childItem);
            beginInsertRows(modelItemIndex, row, row);
            tagItem->insertChild(row, &childItem);
            endInsertRows();
        }
    }

    // 4) Update the position of the updated item within its new parent
    updateItemRowWithRespectToSorting(*tagItem);
}

void TagModel::tagToItem(const Tag & tag, TagItem & item)
{
    item.setLocalId(tag.localId());

    if (tag.hasGuid()) {
        item.setGuid(tag.guid());
    }

    if (tag.hasName()) {
        item.setName(tag.name());
    }

    if (tag.hasParentLocalId()) {
        item.setParentLocalId(tag.parentLocalId());
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
        "model::TagModel",
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

    const auto * tagItem = parentItem.cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        return false;
    }

    const QString & linkedNotebookGuid = tagItem->linkedNotebookGuid();
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
        "model::TagModel",
        "TagModel::updateRestrictionsFromNotebook: "
            << "local id = " << notebook.localId()
            << ", linked notebook guid = "
            << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid()
                                                 : QStringLiteral("<null>")));

    if (!notebook.hasLinkedNotebookGuid()) {
        QNDEBUG("model::TagModel", "Not a linked notebook, ignoring it");
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
        "model::TagModel",
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
        "model::TagModel",
        "TagModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(
            "model::TagModel",
            "Can't process the addition or update of "
                << "a linked notebook without guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.hasUsername())) {
        QNWARNING(
            "model::TagModel",
            "Can't process the addition or update of "
                << "a linked notebook without username: " << linkedNotebook);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    auto it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(
        linkedNotebookGuid);

    if (it != m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end()) {
        if (it.value() == linkedNotebook.username()) {
            QNDEBUG("model::TagModel", "The username hasn't changed, nothing to do");
            return;
        }

        it.value() = linkedNotebook.username();

        QNDEBUG(
            "model::TagModel",
            "Updated the username corresponding to linked "
                << "notebook guid " << linkedNotebookGuid << " to "
                << linkedNotebook.username());
    }
    else {
        QNDEBUG(
            "model::TagModel",
            "Adding new username " << linkedNotebook.username()
                                   << " corresponding to linked notebook guid "
                                   << linkedNotebookGuid);

        it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(
            linkedNotebookGuid, linkedNotebook.username());
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt == m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model::TagModel",
            "Found no existing linked notebook item for "
                << "linked notebook guid " << linkedNotebookGuid
                << ", creating one");

        linkedNotebookItemIt = m_linkedNotebookItems.insert(
            linkedNotebookGuid,
            TagLinkedNotebookRootItem(
                linkedNotebook.username(), linkedNotebookGuid));

        checkAndCreateModelRootItems();

        auto * linkedNotebookItem = &(linkedNotebookItemIt.value());
        int row = rowForNewItem(*m_allTagsRootItem, *linkedNotebookItem);
        beginInsertRows(indexForItem(m_allTagsRootItem), row, row);
        m_allTagsRootItem->insertChild(row, linkedNotebookItem);
        endInsertRows();
    }
    else {
        linkedNotebookItemIt->setUsername(linkedNotebook.username());

        QNTRACE(
            "model::TagModel",
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
    QNTRACE("model::TagModel", "TagModel::itemForId: " << id);

    if (id == m_allTagsRootItemIndexId) {
        return m_allTagsRootItem;
    }

    auto localIdIt = m_indexIdToLocalIdBimap.left.find(id);
    if (localIdIt == m_indexIdToLocalIdBimap.left.end()) {
        auto linkedNotebookGuidIt =
            m_indexIdToLinkedNotebookGuidBimap.left.find(id);

        if (linkedNotebookGuidIt ==
            m_indexIdToLinkedNotebookGuidBimap.left.end()) {
            QNDEBUG(
                "model::TagModel",
                "Found no tag model item corresponding to "
                    << "model index internal id");

            return nullptr;
        }

        const QString & linkedNotebookGuid = linkedNotebookGuidIt->second;
        auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
        if (it == m_linkedNotebookItems.end()) {
            QNDEBUG(
                "model::TagModel",
                "Found no tag linked notebook root model item "
                    << "corresponding to the linked notebook guid "
                    << "corresponding to model index internal id");

            return nullptr;
        }

        return const_cast<TagLinkedNotebookRootItem *>(&(it.value()));
    }

    const QString & localId = localIdIt->second;

    QNTRACE(
        "model::TagModel",
        "Found tag local id corresponding to model index "
            << "internal id: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(localId);
    if (it != localIdIndex.end()) {
        return const_cast<TagItem *>(&(*it));
    }

    QNTRACE(
        "model::TagModel",
        "Found no tag item corresponding to local id " << localId);

    return nullptr;
}

TagModel::IndexId TagModel::idForItem(const ITagModelItem & item) const
{
    const auto * tagItem = item.cast<TagItem>();
    if (tagItem) {
        auto it = m_indexIdToLocalIdBimap.right.find(tagItem->localId());
        if (it == m_indexIdToLocalIdBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLocalIdBimap.insert(
                IndexIdToLocalIdBimap::value_type(id, tagItem->localId())))
            return id;
        }

        return it->second;
    }

    const auto * linkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
            linkedNotebookItem->linkedNotebookGuid());

        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.insert(
                IndexIdToLinkedNotebookGuidBimap::value_type(
                    id, linkedNotebookItem->linkedNotebookGuid())))
            return id;
        }

        return it->second;
    }

    if (&item == m_allTagsRootItem) {
        return m_allTagsRootItemIndexId;
    }

    QNWARNING(
        "model::TagModel",
        "Detected attempt to assign id to unidentified "
            << "tag model item: " << item);
    return 0;
}

QVariant TagModel::dataImpl(
    const ITagModelItem & item, const Column column) const
{
    if (&item == m_allTagsRootItem) {
        if (column == Column::Name) {
            return tr("All tags");
        }

        return {};
    }

    const auto * tagItem = item.cast<TagItem>();
    if (tagItem) {
        switch (column) {
        case Column::Name:
            return QVariant(tagItem->name());
        case Column::Synchronizable:
            return QVariant(tagItem->isSynchronizable());
        case Column::Dirty:
            return QVariant(tagItem->isDirty());
        case Column::FromLinkedNotebook:
            return QVariant(!tagItem->linkedNotebookGuid().isEmpty());
        case Column::NoteCount:
            return QVariant(tagItem->noteCount());
        default:
            return {};
        }
    }

    const auto * linkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        switch (column) {
        case Column::Name:
            return QVariant(linkedNotebookItem->username());
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
        return m_invisibleRootItem;
    }

    return itemForId(static_cast<IndexId>(index.internalId()));
}

ITagModelItem * TagModel::itemForLocalId(const QString & localId) const
{
    const auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(localId);
    if (it != localIdIndex.end()) {
        return &(const_cast<TagItem &>(*it));
    }

    return nullptr;
}

QModelIndex TagModel::indexForItem(const ITagModelItem * item) const
{
    if (!item) {
        return {};
    }

    if (item == m_invisibleRootItem) {
        return {};
    }

    if (item == m_allTagsRootItem) {
        return createIndex(
            0, static_cast<int>(Column::Name), m_allTagsRootItemIndexId);
    }

    auto * parentItem = item->parent();
    if (!parentItem) {
        QNWARNING(
            "model::TagModel",
            "Tag model item has no parent, returning "
                << "invalid index: " << *item);
        return {};
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::TagModel",
            "Internal error: can't get row of the child "
                << "item in parent in TagModel, child item: " << *item
                << "\nParent item: " << *parentItem);
        return {};
    }

    IndexId itemId = idForItem(*item);
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
            return indexForLocalId(item.localId());
        }
    }

    return {};
}

QModelIndex TagModel::indexForLinkedNotebookGuid(
    const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::TagModel",
        "TagModel::indexForLinkedNotebookGuid: "
            << "linked notebook guid = " << linkedNotebookGuid);

    auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (it == m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model::TagModel",
            "Found no model item for linked notebook guid "
                << linkedNotebookGuid);
        return {};
    }

    const auto & item = it.value();
    return indexForItem(&item);
}

QModelIndex TagModel::promote(const QModelIndex & itemIndex)
{
    QNTRACE("model::TagModel", "TagModel::promote");

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote tag: invalid model index"));
        return {};
    }

    auto * modelItem = itemForIndex(itemIndex);
    if (!modelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote tag: no tag item was found"));
        return {};
    }

    auto * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote non-tag items"));
        return {};
    }

    checkAndCreateModelRootItems();

    fixupItemParent(*modelItem);
    auto * parentItem = modelItem->parent();

    if (parentItem == m_allTagsRootItem) {
        REPORT_INFO(QT_TR_NOOP("Can't promote tag: already at top level"));
        return {};
    }

    int row = parentItem->rowForChild(modelItem);
    if (row < 0) {
        QNDEBUG(
            "model::TagModel",
            "Can't find row of promoted item within its "
                << "parent item");
        return {};
    }

    fixupItemParent(*parentItem);
    auto * grandParentItem = parentItem->parent();

    auto * pGrandParentTagItem = grandParentItem->cast<TagItem>();
    if (!canCreateTagItem(*grandParentItem) ||
        (pGrandParentTagItem && !canUpdateTagItem(*pGrandParentTagItem)))
    {
        REPORT_INFO(
            QT_TR_NOOP("Can't promote tag: can't create and/or update tags "
                       "for the grand parent tag due to restrictions"));
        return {};
    }

    int parentRow = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(parentRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't promote tag: can't find parent item's row within "
                       "the grand parent model item"));
        return {};
    }

    QModelIndex parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * pTakenItem = parentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, detected "
                       "internal inconsistency in the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original promoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    auto grandParentIndex = indexForItem(grandParentItem);
    int appropriateRow = rowForNewItem(*grandParentItem, *pTakenItem);

    beginInsertRows(grandParentIndex, appropriateRow, appropriateRow);
    grandParentItem->insertChild(appropriateRow, pTakenItem);
    endInsertRows();

    auto newIndex =
        index(appropriateRow, static_cast<int>(Column::Name), grandParentIndex);

    if (!newIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, invalid model index "
                       "was returned for the promoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(grandParentIndex, appropriateRow, appropriateRow);
        Q_UNUSED(grandParentItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy(*tagItem);
    if (pGrandParentTagItem) {
        tagItemCopy.setParentLocalId(pGrandParentTagItem->localId());
        tagItemCopy.setParentGuid(pGrandParentTagItem->guid());
    }
    else {
        tagItemCopy.setParentLocalId(QString());
        tagItemCopy.setParentGuid(QString());
    }

    bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(tagItemCopy.localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNINFO(
            "model::TagModel",
            "The promoted tag model item was not found in "
                << "the index which is odd. Adding it there");
        Q_UNUSED(localIdIndex.insert(tagItemCopy))
    }
    else {
        localIdIndex.replace(it, tagItemCopy);
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
    QNTRACE("model::TagModel", "TagModel::demote");

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote tag: model index is invalid"));
        return {};
    }

    auto * modelItem = itemForIndex(itemIndex);
    if (!modelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote tag: no tag item was found"));
        return {};
    }

    auto * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote non-tag items"));
        return {};
    }

    checkAndCreateModelRootItems();

    fixupItemParent(*modelItem);
    auto * parentItem = modelItem->parent();

    auto * parentTagItem = parentItem->cast<TagItem>();
    if (parentTagItem && !canUpdateTagItem(*parentTagItem)) {
        REPORT_INFO(
            QT_TR_NOOP("Can't demote tag: can't update parent tag "
                       "due to insufficient permissions"));
        return {};
    }

    int row = parentItem->rowForChild(modelItem);
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

    auto * pSiblingItem = parentItem->childAtRow(row - 1);
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

    const QString & itemLinkedNotebookGuid = tagItem->linkedNotebookGuid();
    const QString & siblingItemLinkedNotebookGuid =
        pSiblingTagItem->linkedNotebookGuid();

    if ((parentItem == m_allTagsRootItem) &&
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
            "model::TagModel",
            error << ", item attempted to be demoted: " << *modelItem
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

    QModelIndex parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * pTakenItem = parentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, detected "
                       "internal inconsistency within the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original demoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, pTakenItem);
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
        parentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy(*tagItem);
    tagItemCopy.setParentLocalId(pSiblingTagItem->localId());
    tagItemCopy.setParentGuid(pSiblingTagItem->guid());

    bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(tagItemCopy.localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        QNINFO(
            "model::TagModel",
            "The demoted tag model item was not found in "
                << "the index which is odd. Adding it there");
        Q_UNUSED(localIdIndex.insert(tagItemCopy))
    }
    else {
        localIdIndex.replace(it, tagItemCopy);
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
        "model::TagModel",
        "TagModel::moveToParent: parent tag name = " << parentTagName);

    if (Q_UNLIKELY(parentTagName.isEmpty())) {
        return removeFromParent(index);
    }

    auto * modelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to move tag item to "
                       "another parent but the model index has no internal id "
                       "corresponding to the item"));
        return {};
    }

    if (Q_UNLIKELY(modelItem == m_allTagsRootItem)) {
        QNDEBUG("model::TagModel", "Can't move all tags root item to a new parent");
        return {};
    }

    if (Q_UNLIKELY(modelItem == m_invisibleRootItem)) {
        QNDEBUG("model::TagModel", "Can't move invisible root item to a new parent");
        return {};
    }

    const TagItem * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move non-tag model item to another parent"));
        return {};
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto tagItemIt = localIdIndex.find(tagItem->localId());
    if (Q_UNLIKELY(tagItemIt == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't find the tag item being "
                       "moved to another parent within the tag model"));
        return {};
    }

    fixupItemParent(*modelItem);
    auto * parentItem = modelItem->parent();
    auto * parentTagItem = parentItem->cast<TagItem>();

    if (parentTagItem &&
        (parentTagItem->nameUpper() == parentTagName.toUpper())) {
        QNDEBUG(
            "model::TagModel",
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
            tagItem->linkedNotebookGuid()) {
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

    auto * newParentItem = const_cast<TagItem *>(&(*newParentItemIt));
    if (Q_UNLIKELY(newParentItem->type() != ITagModelItem::Type::Tag)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: the tag model item corresponding to "
                       "the parent tag under which the current tag should be "
                       "moved has wrong item type"));
        return {};
    }

    auto * newParentTagItem = newParentItem->cast<TagItem>();
    if (Q_UNLIKELY(!newParentTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: the tag model item corresponding to "
                       "the parent tag under which the current tag should be "
                       "moved has no tag item"));
        return {};
    }

    // If the new parent is actually one of the children of the original item,
    // reject the reparent attempt
    const int numMovedItemChildren = modelItem->childrenCount();
    for (int i = 0; i < numMovedItemChildren; ++i) {
        const auto * childItem = modelItem->childAtRow(i);
        if (Q_UNLIKELY(!childItem)) {
            QNWARNING(
                "model::TagModel",
                "Found null child tag model item at row "
                    << i << ", parent item: " << *modelItem);
            continue;
        }

        if (childItem == newParentItem) {
            ErrorString error(
                QT_TR_NOOP("Can't set the parent of the tag to "
                           "one of its child tags"));
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(error);
            return {};
        }
    }

    removeModelItemFromParent(*modelItem);

    TagItem tagItemCopy(*tagItem);
    tagItemCopy.setParentLocalId(newParentTagItem->localId());
    tagItemCopy.setParentGuid(newParentTagItem->guid());
    tagItemCopy.setDirty(true);
    localIdIndex.replace(tagItemIt, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    QModelIndex parentIndex = indexForItem(newParentItem);
    int newRow = rowForNewItem(*newParentItem, *modelItem);

    beginInsertRows(parentIndex, newRow, newRow);
    newParentItem->insertChild(newRow, modelItem);
    endInsertRows();

    QModelIndex newIndex = indexForItem(modelItem);
    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndex TagModel::removeFromParent(const QModelIndex & index)
{
    QNTRACE("model::TagModel", "TagModel::removeFromParent");

    auto * modelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: detected attempt to remove "
                       "the tag model item from its parent but "
                       "the model index has no internal id "
                       "corresponding to any tag model item"));
        return {};
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can only remove tag items from their parent tags"));
        return {};
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(tagItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't find the tag to be removed from its "
                       "parent within the tag model"));
        QNDEBUG("model::TagModel", "Tag item: " << *tagItem);
        return {};
    }

    removeModelItemFromParent(*modelItem);

    TagItem tagItemCopy(*tagItem);
    tagItemCopy.setParentGuid(QString());
    tagItemCopy.setParentLocalId(QString());
    tagItemCopy.setDirty(true);
    localIdIndex.replace(it, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    checkAndCreateModelRootItems();

    QNDEBUG(
        "model::TagModel",
        "Setting all tags root item as the new parent for "
            << "the tag");

    setItemParent(*modelItem, *m_allTagsRootItem);

    auto newIndex = indexForItem(modelItem);
    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QStringList TagModel::tagNames(const QString & linkedNotebookGuid) const
{
    QNTRACE(
        "model::TagModel",
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
        "model::TagModel",
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

    auto & localIdIndex = m_data.get<ByLocalId>();
    int numExistingTags = static_cast<int>(localIdIndex.size());

    if (Q_UNLIKELY(numExistingTags + 1 >= m_account.tagCountMax())) {
        errorDescription.setBase(
            QT_TR_NOOP("Can't create a new tag: the account "
                       "can contain a limited number of tags"));

        errorDescription.details() = QString::number(m_account.tagCountMax());
        return {};
    }

    ITagModelItem * parentItem = nullptr;

    if (!linkedNotebookGuid.isEmpty()) {
        parentItem =
            &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!parentItem) {
        checkAndCreateModelRootItems();
        parentItem = m_allTagsRootItem;
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

        parentItem = const_cast<TagItem *>(&(*parentTagIt));

        QNDEBUG(
            "model::TagModel",
            "Will put the new tag under parent item: " << *parentItem);
    }

    TagItem item;
    item.setLocalId(UidGenerator::Generate());
    Q_UNUSED(m_tagItemsNotYetInLocalStorageIds.insert(item.localId()))

    item.setName(tagName);
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    auto * parentTagItem = parentItem->cast<TagItem>();
    if (parentTagItem) {
        item.setParentLocalId(parentTagItem->localId());
    }

    Q_EMIT aboutToAddTag();

    auto insertionResult = localIdIndex.insert(item);
    auto * tagItem = const_cast<TagItem *>(&(*insertionResult.first));
    setItemParent(*tagItem, *parentItem);

    updateTagInLocalStorage(item);

    QModelIndex addedTagIndex = indexForLocalId(item.localId());
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

bool TagModel::hasSynchronizableChildren(const ITagModelItem * modelItem) const
{
    const auto * linkedNotebookItem =
        modelItem->cast<TagLinkedNotebookRootItem>();

    if (linkedNotebookItem) {
        return true;
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (tagItem && tagItem->isSynchronizable()) {
        return true;
    }

    auto children = modelItem->children();
    for (const auto * pChild: qAsConst(children)) {
        if (Q_UNLIKELY(!pChild)) {
            QNWARNING(
                "model::TagModel",
                "Found null child at tag model item: " << *modelItem);
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
    QNTRACE("model::TagModel", "TagModel::mapChildItems");

    auto & localIdIndex = m_data.get<ByLocalId>();
    for (auto & item: localIdIndex) {
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
    QNTRACE("model::TagModel", "TagModel::mapChildItems: " << item);

    const auto * tagItem = item.cast<TagItem>();
    const auto * linkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();

    if (Q_UNLIKELY(!tagItem && !linkedNotebookItem)) {
        return;
    }

    auto parentIndex = indexForItem(&item);

    if (tagItem) {
        auto & parentLocalIdIndex = m_data.get<ByParentLocalId>();
        auto range = parentLocalIdIndex.equal_range(tagItem->localId());
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

    if (linkedNotebookItem) {
        auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();

        auto range = linkedNotebookGuidIndex.equal_range(
            linkedNotebookItem->linkedNotebookGuid());

        for (auto it = range.first; it != range.second; ++it) {
            auto & currentTagItem = const_cast<TagItem &>(*it);
            if (!currentTagItem.parentLocalId().isEmpty()) {
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

void TagModel::removeItemByLocalId(const QString & localId)
{
    QNTRACE("model::TagModel", "TagModel::removeItemByLocalId: " << localId);

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG("model::TagModel", "Can't find item to remove from the tag model");
        return;
    }

    auto * tagItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*tagItem);
    auto * parentItem = tagItem->parent();

    int row = parentItem->rowForChild(tagItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::TagModel",
            "Internal error: can't get the child item's row "
                << "within its parent in the tag model, child item: "
                << *tagItem << "\nParent item: " << *parentItem);
        return;
    }

    // Need to recursively remove all the children of this tag and do this
    // before the actual removal of their parent
    auto & parentLocalIdIndex = m_data.get<ByParentLocalId>();
    while (true) {
        auto childIt = parentLocalIdIndex.find(localId);
        if (childIt == parentLocalIdIndex.end()) {
            break;
        }

        removeItemByLocalId(childIt->localId());
    }

    auto parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();

    auto indexIt = m_indexIdToLocalIdBimap.right.find(itemIt->localId());
    if (indexIt != m_indexIdToLocalIdBimap.right.end()) {
        Q_UNUSED(m_indexIdToLocalIdBimap.right.erase(indexIt))
    }

    Q_UNUSED(localIdIndex.erase(itemIt))

    checkAndRemoveEmptyLinkedNotebookRootItem(*parentItem);
}

void TagModel::removeModelItemFromParent(ITagModelItem & item)
{
    QNTRACE("model::TagModel", "TagModel::removeModelItemFromParent: " << item);

    auto * parentItem = item.parent();
    if (Q_UNLIKELY(!parentItem)) {
        QNDEBUG("model::TagModel", "No parent item, nothing to do");
        return;
    }

    QNTRACE("model::TagModel", "Parent item: " << *parentItem);
    int row = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::TagModel",
            "Can't find the child tag item's row within its "
                << "parent; child item = " << item
                << ", parent item = " << *parentItem);
        return;
    }

    QNTRACE("model::TagModel", "Removing the child at row " << row);

    auto parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();
}

int TagModel::rowForNewItem(
    const ITagModelItem & parentItem, const ITagModelItem & newItem) const
{
    QNTRACE(
        "model::TagModel",
        "TagModel::rowForNewItem: new item = " << newItem << ", parent item = "
                                               << parentItem);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model::TagModel", "Won't sort on column " << m_sortedColumn);
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

    QNTRACE("model::TagModel", "Appropriate row = " << row);
    return row;
}

void TagModel::updateItemRowWithRespectToSorting(ITagModelItem & item)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::updateItemRowWithRespectToSorting: item = " << item);

    if (m_sortedColumn != Column::Name) {
        QNDEBUG("model::TagModel", "Won't sort on column " << m_sortedColumn);
        // Sorting by other columns is not yet implemented
        return;
    }

    fixupItemParent(item);
    auto * parentItem = item.parent();

    int currentItemRow = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(
            "model::TagModel",
            "Can't update tag model item's row: can't find "
                << "its original row within parent: " << item);
        return;
    }

    QModelIndex parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(parentItem->takeChild(currentItemRow))
    endRemoveRows();

    int appropriateRow = rowForNewItem(*parentItem, item);
    beginInsertRows(parentIndex, appropriateRow, appropriateRow);
    parentItem->insertChild(appropriateRow, &item);
    endInsertRows();

    QNTRACE(
        "model::TagModel",
        "Moved item from row " << currentItemRow << " to row " << appropriateRow
                               << "; item: " << item);
}

void TagModel::updatePersistentModelIndices()
{
    QNTRACE("model::TagModel", "TagModel::updatePersistentModelIndices");

    // Ensure any persistent model indices would be updated appropriately
    auto indices = persistentIndexList();
    for (const auto & index: qAsConst(indices)) {
        auto * item = itemForId(static_cast<IndexId>(index.internalId()));
        QModelIndex replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
}

void TagModel::updateTagInLocalStorage(const TagItem & item)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::updateTagInLocalStorage: local id = " << item.localId());

    Tag tag;

    auto notYetSavedItemIt =
        m_tagItemsNotYetInLocalStorageIds.find(item.localId());

    if (notYetSavedItemIt == m_tagItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG("model::TagModel", "Updating the tag");

        const auto * pCachedTag = m_cache.get(item.localId());
        if (Q_UNLIKELY(!pCachedTag)) {
            QUuid requestId = QUuid::createUuid();
            Q_UNUSED(m_findTagToPerformUpdateRequestIds.insert(requestId))

            Tag dummy;
            dummy.setLocalId(item.localId());

            QNDEBUG(
                "model::TagModel",
                "Emitting the request to find tag: "
                    << "local id = " << item.localId()
                    << ", request id = " << requestId);

            Q_EMIT findTag(dummy, requestId);
            return;
        }

        tag = *pCachedTag;
    }

    tagFromItem(item, tag);

    auto requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageIds.end()) {
        Q_UNUSED(m_addTagRequestIds.insert(requestId));

        QNTRACE(
            "model::TagModel",
            "Emitting the request to add the tag to the local "
                << "storage: id = " << requestId << ", tag: " << tag);

        Q_EMIT addTag(tag, requestId);
        Q_UNUSED(m_tagItemsNotYetInLocalStorageIds.erase(notYetSavedItemIt))
    }
    else {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));

        // While the tag is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(tag.localId()))

        QNTRACE(
            "model::TagModel",
            "Emitting the request to update tag in the local "
                << "storage: id = " << requestId << ", tag: " << tag);

        Q_EMIT updateTag(tag, requestId);
    }
}

void TagModel::tagFromItem(const TagItem & item, Tag & tag) const
{
    tag.setLocalId(item.localId());
    tag.setGuid(item.guid());
    tag.setLinkedNotebookGuid(item.linkedNotebookGuid());
    tag.setName(item.name());
    tag.setLocal(!item.isSynchronizable());
    tag.setDirty(item.isDirty());
    tag.setFavorited(item.isFavorited());
    tag.setParentLocalId(item.parentLocalId());
    tag.setParentGuid(item.parentGuid());
}

void TagModel::setNoteCountForTag(
    const QString & tagLocalId, const int noteCount)
{
    TagDataByLocalId & localIdIndex = m_data.get<ByLocalId>();

    auto itemIt = localIdIndex.find(tagLocalId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        // Probably this tag was expunged
        QNDEBUG(
            "model::TagModel",
            "No tag receiving the note count update was found "
                << "in the model: " << tagLocalId);
        return;
    }

    auto * modelItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*modelItem);
    auto * parentItem = modelItem->parent();

    int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(
            QT_TR_NOOP("Can't find the row of tag model item being updated "
                       "with the note count within its parent"));

        QNWARNING(
            "model::TagModel",
            error << ", tag local id: " << tagLocalId
                  << "\nTag model item: " << *modelItem);

        Q_EMIT notifyError(error);
        return;
    }

    TagItem itemCopy(*itemIt);
    itemCopy.setNoteCount(noteCount);
    Q_UNUSED(localIdIndex.replace(itemIt, itemCopy))

    auto id = idForItem(*modelItem);
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

    auto * modelItem = itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: can't "
                       "find the model item corresponding to index"));
        return;
    }

    auto * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: "
                       "the target model item is not a tag item"));
        return;
    }

    if (favorited == tagItem->isFavorited()) {
        QNDEBUG("model::TagModel", "Favorited flag's value hasn't changed");
        return;
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    auto it = localIdIndex.find(tagItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: the modified tag "
                       "entry was not found within the model"));
        return;
    }

    TagItem itemCopy(*tagItem);
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the tag as dirty as favorited property is not included
    // into the synchronization protocol

    localIdIndex.replace(it, itemCopy);
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
        "model::TagModel",
        "TagModel::findOrCreateLinkedNotebookModelItem: "
            << linkedNotebookGuid);

    checkAndCreateModelRootItems();

    if (Q_UNLIKELY(linkedNotebookGuid.isEmpty())) {
        QNWARNING(
            "model::TagModel",
            "Detected the request for finding of creation "
                << "of a linked notebook model item for empty linked notebook "
                << "guid");
        return *m_allTagsRootItem;
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);

    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        QNDEBUG(
            "model::TagModel",
            "Found existing linked notebook model item for "
                << "linked notebook guid " << linkedNotebookGuid);
        return linkedNotebookItemIt.value();
    }

    QNTRACE(
        "model::TagModel",
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
            "model::TagModel",
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

    auto * linkedNotebookItem = &(linkedNotebookItemIt.value());
    QNTRACE("model::TagModel", "Linked notebook root item: " << *linkedNotebookItem);

    int row = rowForNewItem(*m_allTagsRootItem, *linkedNotebookItem);
    beginInsertRows(indexForItem(m_allTagsRootItem), row, row);
    m_allTagsRootItem->insertChild(row, linkedNotebookItem);
    endInsertRows();

    return *linkedNotebookItem;
}

void TagModel::checkAndRemoveEmptyLinkedNotebookRootItem(
    ITagModelItem & modelItem)
{
    if (modelItem.type() != ITagModelItem::Type::LinkedNotebook) {
        return;
    }

    auto * linkedNotebookItem = modelItem.cast<TagLinkedNotebookRootItem>();
    if (!linkedNotebookItem) {
        return;
    }

    if (modelItem.hasChildren()) {
        return;
    }

    QNTRACE(
        "model::TagModel",
        "Removed the last child from the linked notebook "
            << "root item, will remove that item as well");

    removeModelItemFromParent(modelItem);

    QString linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();

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
        "model::TagModel",
        "TagModel::checkAndFindLinkedNotebookRestrictions: " << tagItem);

    const QString & linkedNotebookGuid = tagItem.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        QNTRACE("model::TagModel", "No linked notebook guid");
        return;
    }

    auto restrictionsIt =
        m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (restrictionsIt != m_tagRestrictionsByLinkedNotebookGuid.end()) {
        QNTRACE(
            "model::TagModel",
            "Already have the tag restrictions for linked "
                << "notebook guid " << linkedNotebookGuid);
        return;
    }

    auto it = m_findNotebookRequestForLinkedNotebookGuid.left.find(
        linkedNotebookGuid);

    if (it != m_findNotebookRequestForLinkedNotebookGuid.left.end()) {
        QNTRACE(
            "model::TagModel",
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
    notebook.unsetLocalId();
    notebook.setLinkedNotebookGuid(linkedNotebookGuid);

    QNTRACE(
        "model::TagModel",
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
    auto * parentItem = item.parent();
    if (parentItem) {
        // No fixup is needed
        return;
    }

    checkAndCreateModelRootItems();

    if ((&item == m_allTagsRootItem) || (&item == m_invisibleRootItem)) {
        // No fixup is needed for these special items
        return;
    }

    auto * linkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        setItemParent(item, *m_allTagsRootItem);
        return;
    }

    auto * tagItem = item.cast<TagItem>();
    if (tagItem) {
        const QString & parentTagLocalId = tagItem->parentLocalId();
        if (!parentTagLocalId.isEmpty()) {
            auto & localIdIndex = m_data.get<ByLocalId>();
            auto it = localIdIndex.find(parentTagLocalId);
            if (it != localIdIndex.end()) {
                auto * parentItem = const_cast<TagItem *>(&(*it));
                setItemParent(item, *parentItem);
            }
            else {
                QNDEBUG(
                    "model::TagModel",
                    "No tag corresponding to parent local id "
                        << parentTagLocalId << ", setting all tags root item "
                        << "as parent");
                setItemParent(item, *m_allTagsRootItem);
            }

            return;
        }

        const QString & linkedNotebookGuid = tagItem->linkedNotebookGuid();
        if (!linkedNotebookGuid.isEmpty()) {
            auto * linkedNotebookItem =
                &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));

            setItemParent(item, *linkedNotebookItem);
            return;
        }
    }

    setItemParent(item, *m_allTagsRootItem);
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
    if (Q_UNLIKELY(!m_invisibleRootItem)) {
        m_invisibleRootItem = new InvisibleTagRootItem;
        QNDEBUG("model::TagModel", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_allTagsRootItem)) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_allTagsRootItem = new AllTagsRootItem;
        m_allTagsRootItem->setParent(m_invisibleRootItem);
        endInsertRows();
        QNDEBUG("model::TagModel", "Created all tags root item");
    }
}

#define MODEL_ITEM_NAME(item, itemName)                                        \
    if (item.type() == ITagModelItem::Type::Tag) {                             \
        const auto * tagItem = item.cast<TagItem>();                          \
        if (tagItem) {                                                        \
            itemName = tagItem->nameUpper();                                  \
        }                                                                      \
    }                                                                          \
    else if (item.type() == ITagModelItem::Type::LinkedNotebook) {             \
        const auto * linkedNotebookItem =                                     \
            item.cast<TagLinkedNotebookRootItem>();                            \
        if (linkedNotebookItem) {                                             \
            itemName = linkedNotebookItem->username().toUpper();              \
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
