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

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/Validation.h>
#include <quentier/utility/SuppressWarnings.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <qevercloud/types/Notebook.h>

#include <QByteArray>
#include <QDataStream>
#include <QMimeData>

#include <algorithm>
#include <limits>
#include <string_view>
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

using namespace std::string_view_literals;

namespace {

constexpr int gTagModelColumnCount = 5;

const auto gMimeType = "application/x-com.quentier.tagmodeldatalist"sv;

[[nodiscard]] bool findChildTagItem(
    const ITagModelItem & item, const QString & tagLocalId)
{
    const auto childCount = item.childrenCount();
    for (std::decay_t<decltype(childCount)> i = 0; i < childCount; ++i) {
        const auto * childItem = item.childAtRow(i);
        if (childItem) {
            if (findChildTagItem(*childItem, tagLocalId)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

TagModel::TagModel(
    Account account, local_storage::ILocalStoragePtr localStorage,
    TagCache & cache, QObject * parent) :
    AbstractItemModel{std::move(account), parent},
    m_localStorage{std::move(localStorage)}, m_cache{cache}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "TagModel ctor: local storage is null"}};
    }
}

TagModel::~TagModel()
{
    delete m_allTagsRootItem;
    delete m_invisibleRootItem;
}

QString TagModel::mimeTypeName()
{
    return QString::fromUtf8(gMimeType.data(), gMimeType.size());
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

void TagModel::start()
{
    QNDEBUG("model::TagModel", "TagModel::start");

    if (m_isStarted) {
        QNDEBUG("model::TagModel", "Already started");
        return;
    }

    m_isStarted = true;

    connectToLocalStorageEvents();
    requestTagsList();
    requestLinkedNotebooksList();
}

void TagModel::stop(const StopMode stopMode)
{
    QNDEBUG("model::TagModel", "TagModel::stop: " << stopMode);

    if (!m_isStarted) {
        QNDEBUG("model::TagModel", "Already stopped");
        return;
    }

    m_isStarted = false;
    disconnectFromLocalStorageEvents();
    clearModel();
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

        auto canceler = setupCanceler();
        Q_ASSERT(canceler);

        auto expungeTagFuture = m_localStorage->expungeTagByLocalId(
            tagItem->localId());

        threading::onFailed(
            std::move(expungeTagFuture), this,
            [this, canceler = std::move(canceler),
             localId = tagItem->localId()](const QException & e) {
                if (canceler->isCanceled()) {
                    return;
                }

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
    list << QString::fromUtf8(gMimeType.data(), gMimeType.size());
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
        QString::fromUtf8(gMimeType.data(), gMimeType.size()),
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

    if (!mimeData ||
        !mimeData->hasFormat(
            QString::fromUtf8(gMimeType.data(), gMimeType.size())))
    {
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

    QByteArray data = qUncompress(
        mimeData->data(QString::fromUtf8(gMimeType.data(), gMimeType.size())));

    QDataStream in{&data, QIODevice::ReadOnly};

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

    if (newParentItem == tagItem->parent()) {
        QNDEBUG(
            "model::TagModel",
            "Item is already under the chosen parent, nothing to do");
        return true;
    }

    // Check that we aren't trying to move the tag under one of its
    // children
    if (findChildTagItem(*newParentItem, tagItem->localId())) {
        ErrorString error{
            QT_TR_NOOP("Can't move tag under one of its child tags")};
        QNINFO("model::TagModel", error);
        Q_EMIT notifyError(std::move(error));
        return false;
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

void TagModel::connectToLocalStorageEvents()
{
    QNDEBUG("model::TagModel", "TagModel::connectToLocalStorageEvents");

    if (m_connectedToLocalStorage) {
        QNDEBUG("model::TagModel", "Already connected to local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::tagPut,
        this,
        [this](const qevercloud::Tag & tag) {
            const auto status = onTagAddedOrUpdated(tag);
            if (status == TagPutStatus::Added) {
                requestNoteCountForTag(tag.localId());
            }
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::tagExpunged,
        this,
        [this](const QString & tagLocalId,
               const QStringList & expungedChildTagLocalIds) {
            QNDEBUG(
                "model::TagModel",
                "Tag expunged: local id = " << tagLocalId
                    << ", expunged child tag local ids: "
                    << expungedChildTagLocalIds.join(QStringLiteral(", ")));

            Q_EMIT aboutToRemoveTags();
            // NOTE: all child items would be removed from the model automatically
            removeItemByLocalId(tagLocalId);
            Q_EMIT removedTags();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notebookPut,
        this,
        [this](const qevercloud::Notebook & notebook) {
            if (!notebook.linkedNotebookGuid()) {
                return;
            }
            updateRestrictionsFromNotebooks(
                *notebook.linkedNotebookGuid(), QList{notebook});
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notebookExpunged,
        this,
        [this]([[maybe_unused]] const QString & notebookLocalId) {
            // Notebook was expunged along with its notes which could have any
            // possible binding with tags => need to update note counts for all
            // tags
            requestNoteCountsPerAllTags();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::notePut,
        this,
        [this]([[maybe_unused]] const qevercloud::Note & note) {
            // Note could be added or updated and in the latter case some tags
            // might have been removed from it. So need to update note counts
            // for all tags
            requestNoteCountsPerAllTags();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::noteUpdated,
        this,
        [this]([[maybe_unused]] const qevercloud::Note & note,
               [[maybe_unused]] const local_storage::ILocalStorage::UpdateNoteOptions & options) {
            // Same as above, we don't know exactly what was changed about the
            // note, probably some tags were removed from it. So updating note
            // counts for all tags
            requestNoteCountsPerAllTags();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::noteExpunged,
        this,
        [this]([[maybe_unused]] const QString & noteLocalId) {
            requestNoteCountsPerAllTags();
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::linkedNotebookPut,
        this,
        [this](const qevercloud::LinkedNotebook & linkedNotebook) {
            onLinkedNotebookAddedOrUpdated(linkedNotebook);
        });

    QObject::connect(
        notifier,
        &local_storage::ILocalStorageNotifier::linkedNotebookExpunged,
        this,
        [this](const qevercloud::Guid & linkedNotebookGuid) {
            onLinkedNotebookExpunged(linkedNotebookGuid);
        });

    m_connectedToLocalStorage = true;
}

void TagModel::disconnectFromLocalStorageEvents()
{
    QNDEBUG("model::TagModel", "TagModel::disconnectFromLocalStorageEvents");

    if (!m_connectedToLocalStorage) {
        QNDEBUG("model::TagModel", "Already disconnected from local storage");
        return;
    }

    if (!m_connectedToLocalStorage) {
        QNDEBUG(
            "model::TagModel",
            "Already disconnected from local storage");
        return;
    }

    auto * notifier = m_localStorage->notifier();
    Q_ASSERT(notifier);
    notifier->disconnect(this);

    m_connectedToLocalStorage = false;
}

void TagModel::requestTagsList()
{
    QNTRACE(
        "model::TagModel",
        "TagModel::requestTagsList: offset = " << m_listTagsOffset);

    local_storage::ILocalStorage::ListTagsOptions options;
    options.m_limit = 100;
    options.m_offset = m_listTagsOffset;
    options.m_order = local_storage::ILocalStorage::ListTagsOrder::NoOrder;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    QNDEBUG(
        "model::TagModel",
        "Requesting a list of tags: offset = " << m_listTagsOffset);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto listTagsFuture = m_localStorage->listTags(options);

    auto listTagsThenFuture = threading::then(
        std::move(listTagsFuture), this,
        [this, canceler](const QList<qevercloud::Tag> & tags) {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "model::TagModel",
                "Received " << tags.size() << " tags from local storage");

            for (const auto & tag: std::as_const(tags)) {
                onTagAddedOrUpdated(tag);
            }

            if (tags.isEmpty()) {
                QNDEBUG(
                    "model::TagModel", "Received all tags from local storage");

                m_allTagsListed = true;
                requestNoteCountsPerAllTags();

                if (m_allLinkedNotebooksListed) {
                    Q_EMIT notifyAllTagsListed();
                    Q_EMIT notifyAllItemsListed();
                }

                return;
            }

            m_listTagsOffset +=
                static_cast<quint64>(std::max<qint64>(tags.size(), 0));

            requestTagsList();
        });

    threading::onFailed(
        std::move(listTagsThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Failed to list notebooks")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void TagModel::requestNoteCountForTag(const QString & tagLocalId)
{
    QNDEBUG(
        "model::TagModel", "TagModel::requestNoteCountForTag: " << tagLocalId);

    const auto options = local_storage::ILocalStorage::NoteCountOptions{} |
        local_storage::ILocalStorage::NoteCountOption::IncludeNonDeletedNotes;

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto noteCountFuture =
        m_localStorage->noteCountPerTagLocalId(tagLocalId, options);

    auto noteCountThenFuture = threading::then(
        std::move(noteCountFuture), this,
        [this, tagLocalId, canceler](const quint32 count) {
            if (canceler->isCanceled()) {
                return;
            }

            setNoteCountForTag(tagLocalId, count);
        });

    threading::onFailed(
        std::move(noteCountThenFuture), this,
        [this, tagLocalId,
         canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{
                QT_TR_NOOP("Failed to get note count for tag local id"});
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING(
                "model::TagModel", error << "; tag local id = " << tagLocalId);
            Q_EMIT notifyError(std::move(error));
        });
}

void TagModel::requestNoteCountsPerAllTags()
{
    QNDEBUG("model::TagModel", "TagModel::requestNoteCountsPerAllTags");

    const auto noteCountOptions =
        local_storage::ILocalStorage::NoteCountOptions{} |
        local_storage::ILocalStorage::NoteCountOption::IncludeNonDeletedNotes;

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto noteCountsFuture = m_localStorage->noteCountsPerTags(
        local_storage::ILocalStorage::ListTagsOptions{}, noteCountOptions);

    auto noteCountsThenFuture = threading::then(
        std::move(noteCountsFuture), this,
        [this, canceler](const QHash<QString, quint32> & noteCountsPerTags) {
            if (canceler->isCanceled()) {
                return;
            }

            auto & localIdIndex = m_data.get<ByLocalId>();
            for (const auto it:
                 qevercloud::toRange(std::as_const(noteCountsPerTags))) {
                const auto lit = localIdIndex.find(it.key());
                if (Q_UNLIKELY(lit == localIdIndex.end())) {
                    QNDEBUG(
                        "model::TagModel",
                        "Received note count "
                            << it.value() << " for tag " << it.key()
                            << " which is not present in the model");
                    continue;
                }

                auto item = *lit;
                item.setNoteCount(it.value());

                const QString parentLocalId = item.parentLocalId();
                const QString linkedNotebookGuid = item.linkedNotebookGuid();

                localIdIndex.replace(lit, std::move(item));

                if (parentLocalId.isEmpty() && linkedNotebookGuid.isEmpty()) {
                    continue;
                }

                // If tag item has either parent tag or linked notebook local
                // id, we'll send dataChanged signal for it here; for all tags
                // from user's own account and without parent tags we'll send
                // dataChanged signal later, once for all such tags
                auto idx = indexForLocalId(it.key());
                if (idx.isValid()) {
                    idx = index(
                        idx.row(), static_cast<int>(Column::NoteCount),
                        idx.parent());
                    Q_EMIT dataChanged(idx, idx);
                }
            }

            const auto allTagsRootItemIndex = indexForItem(m_allTagsRootItem);

            const auto startIndex =
                index(0, static_cast<int>(Column::NoteCount),
                      allTagsRootItemIndex);

            const auto endIndex = index(
                rowCount(allTagsRootItemIndex), static_cast<int>(Column::NoteCount),
                allTagsRootItemIndex);

            Q_EMIT dataChanged(startIndex, endIndex);
        });

    threading::onFailed(
        std::move(noteCountsThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{
                QT_TR_NOOP("Failed to get note counts for all tags"});
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

void TagModel::requestLinkedNotebooksList()
{
    QNTRACE("model::TagModel", "TagModel::requestLinkedNotebooksList");

    local_storage::ILocalStorage::ListLinkedNotebooksOptions options;
    options.m_limit = 40;
    options.m_offset = m_listLinkedNotebooksOffset;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    options.m_order =
        local_storage::ILocalStorage::ListLinkedNotebooksOrder::NoOrder;

    QNDEBUG(
        "model::TagModel",
        "Request a list of linked notebooks: offset = "
            << m_listLinkedNotebooksOffset);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto listLinkedNotebooksFuture =
        m_localStorage->listLinkedNotebooks(options);

    auto listLinkedNotebooksThenFuture = threading::then(
        std::move(listLinkedNotebooksFuture), this,
        [this,
         canceler](const QList<qevercloud::LinkedNotebook> & linkedNotebooks) {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "model::TagModel",
                "Received " << linkedNotebooks.size() << " linked notebooks "
                            << "from local storage");
            for (const auto & linkedNotebook: std::as_const(linkedNotebooks)) {
                onLinkedNotebookAddedOrUpdated(linkedNotebook);
            }

            if (linkedNotebooks.isEmpty()) {
                QNDEBUG(
                    "model::TagModel",
                    "Received all linked notebooks from local storage");

                m_allLinkedNotebooksListed = true;

                if (m_allTagsListed) {
                    Q_EMIT notifyAllTagsListed();
                    Q_EMIT notifyAllItemsListed();
                }

                return;
            }

            m_listLinkedNotebooksOffset += static_cast<quint64>(
                std::max<qint64>(linkedNotebooks.size(), 0));

            requestLinkedNotebooksList();
        });

    threading::onFailed(
        std::move(listLinkedNotebooksThenFuture), this,
        [this, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP("Failed to list linked notebooks")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING("model::NotebookModel", error);
            Q_EMIT notifyError(std::move(error));
        });
}

TagModel::TagPutStatus TagModel::onTagAddedOrUpdated(
    const qevercloud::Tag & tag, const QStringList * tagNoteLocalIds)
{
    m_cache.put(tag.localId(), tag);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(tag.localId());
    const bool newTag = (itemIt == localIdIndex.end());
    if (newTag) {
        Q_EMIT aboutToAddTag();

        onTagAdded(tag, tagNoteLocalIds);

        auto addedTagIndex = indexForLocalId(tag.localId());
        Q_EMIT addedTag(addedTagIndex);
        return TagPutStatus::Added;
    }

    auto tagIndexBefore = indexForLocalId(tag.localId());
    Q_EMIT aboutToUpdateTag(tagIndexBefore);

    onTagUpdated(tag, itemIt, tagNoteLocalIds);

    QModelIndex tagIndexAfter = indexForLocalId(tag.localId());
    Q_EMIT updatedTag(tagIndexAfter);
    return TagPutStatus::Updated;
}

void TagModel::onTagAdded(
    const qevercloud::Tag & tag, const QStringList * tagNoteLocalIds)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onTagAdded: tag local id = "
            << tag.localId() << ", tag note local ids: "
            << (tagNoteLocalIds
                    ? tagNoteLocalIds->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    ITagModelItem * parentItem = nullptr;
    auto & localIdIndex = m_data.get<ByLocalId>();

    if (!tag.parentTagLocalId().isEmpty()) {
        const auto it = localIdIndex.find(tag.parentTagLocalId());
        if (it != localIdIndex.end()) {
            parentItem = const_cast<TagItem *>(&(*it));
        }
    }
    else if (tag.linkedNotebookGuid()) {
        const auto & linkedNotebookGuid = *tag.linkedNotebookGuid();
        parentItem =
            &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!parentItem) {
        checkAndCreateModelRootItems();
        parentItem = m_allTagsRootItem;
    }

    auto parentIndex = indexForItem(parentItem);

    TagItem newItem;
    tagToItem(tag, newItem);

    checkAndFindLinkedNotebookRestrictions(newItem);

    if (tagNoteLocalIds) {
        newItem.setNoteCount(static_cast<quint32>(std::clamp<qint64>(
            tagNoteLocalIds->size(), 0, std::numeric_limits<quint32>::max())));
    }

    const auto insertionResult = localIdIndex.insert(newItem);
    const auto it = insertionResult.first;
    auto * item = const_cast<TagItem *>(&(*it));

    const int row = rowForNewItem(*parentItem, *item);

    beginInsertRows(parentIndex, row, row);
    parentItem->insertChild(row, item);
    endInsertRows();

    mapChildItems(*item);
}

void TagModel::onTagUpdated(
    const qevercloud::Tag & tag, TagDataByLocalId::iterator it,
    const QStringList * tagNoteLocalIds)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onTagUpdated: tag local id = "
            << tag.localId() << ", tag note local ids: "
            << (tagNoteLocalIds
                    ? tagNoteLocalIds->join(QStringLiteral(", "))
                    : QStringLiteral("none")));

    TagItem itemCopy;
    tagToItem(tag, itemCopy);

    if (tagNoteLocalIds) {
        itemCopy.setNoteCount(static_cast<quint32>(std::clamp<qint64>(
            tagNoteLocalIds->size(), 0, std::numeric_limits<quint32>::max())));
    }

    auto * tagItem = const_cast<TagItem *>(&(*it));
    auto * parentItem = tagItem->parent();
    if (Q_UNLIKELY(!parentItem)) {
        // FIXME: should try to fix it automatically
        ErrorString error{
            QT_TR_NOOP("Tag model item being updated does not "
                       "have a parent item linked with it")};

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
    if (!tag.parentTagLocalId().isEmpty()) {
        const auto parentIt = localIdIndex.find(tag.parentTagLocalId());
        if (parentIt != localIdIndex.end()) {
            newParentItem = const_cast<TagItem *>(&(*parentIt));
        }
    }
    else if (tag.linkedNotebookGuid()) {
        newParentItem =
            &(findOrCreateLinkedNotebookModelItem(*tag.linkedNotebookGuid()));
    }

    if (!newParentItem) {
        checkAndCreateModelRootItems();
        newParentItem = m_allTagsRootItem;
    }

    auto parentItemIndex = indexForItem(parentItem);
    auto newParentItemIndex =
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

    quint32 numNotesPerTag = it->noteCount();
    itemCopy.setNoteCount(numNotesPerTag);

    Q_UNUSED(localIdIndex.replace(it, itemCopy))
    newParentItem->insertChild(row, tagItem);

    endInsertRows();

    auto modelIndexFrom = index(row, 0, newParentItemIndex);
    auto modelIndexTo =
        index(row, gTagModelColumnCount - 1, newParentItemIndex);

    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    // 3) Ensure all the child tag model items are properly located under this
    // tag model item
    auto modelItemIndex = indexForItem(tagItem);

    auto & parentLocalIdIndex = m_data.get<ByParentLocalId>();
    const auto range = parentLocalIdIndex.equal_range(tagItem->localId());
    for (auto childIt = range.first; childIt != range.second; ++childIt) {
        const TagItem & childItem = *childIt;
        const QString & childItemLocalId = childItem.localId();

        auto childItemIt = localIdIndex.find(childItemLocalId);
        if (childItemIt != localIdIndex.end()) {
            auto & childItem = const_cast<TagItem &>(*childItemIt);

            int itemRow = tagItem->rowForChild(&childItem);
            if (itemRow >= 0) {
                continue;
            }

            itemRow = rowForNewItem(*tagItem, childItem);
            beginInsertRows(modelItemIndex, itemRow, itemRow);
            tagItem->insertChild(itemRow, &childItem);
            endInsertRows();
        }
    }

    // 4) Update the position of the updated item within its new parent
    updateItemRowWithRespectToSorting(*tagItem);
}

void TagModel::tagToItem(const qevercloud::Tag & tag, TagItem & item)
{
    item.setLocalId(tag.localId());

    if (tag.guid()) {
        item.setGuid(*tag.guid());
    }

    if (tag.name()) {
        item.setName(*tag.name());
    }

    if (!tag.parentTagLocalId().isEmpty()) {
        item.setParentLocalId(tag.parentTagLocalId());
    }

    if (tag.parentGuid()) {
        item.setParentGuid(*tag.parentGuid());
    }

    if (tag.linkedNotebookGuid()) {
        item.setLinkedNotebookGuid(*tag.linkedNotebookGuid());
    }

    item.setSynchronizable(!tag.isLocalOnly());
    item.setDirty(tag.isLocallyModified());
    item.setFavorited(tag.isLocallyFavorited());

    QNTRACE(
        "model::TagModel",
        "Created tag model item from tag; item: " << item << "\nTag: " << tag);
}

bool TagModel::canUpdateTagItem(const TagItem & item) const
{
    const auto & linkedNotebookGuid = item.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    const auto it =
        m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
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
    if (parentItem.type() == ITagModelItem::Type::AllTagsRoot) {
        return true;
    }

    QString linkedNotebookGuid;
    if (parentItem.type() == ITagModelItem::Type::LinkedNotebook) {
        const auto * linkedNotebookItem = parentItem.cast<TagLinkedNotebookRootItem>();
        Q_ASSERT(linkedNotebookItem);
        linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
    }
    else if (parentItem.type() == ITagModelItem::Type::Tag) {
        const auto * tagItem = parentItem.cast<TagItem>();
        Q_ASSERT(tagItem);
        linkedNotebookGuid = tagItem->linkedNotebookGuid();
    }

    if (linkedNotebookGuid.isEmpty()) {
        return true;
    }

    const auto it =
        m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (it == m_tagRestrictionsByLinkedNotebookGuid.end()) {
        return false;
    }

    const Restrictions & restrictions = it.value();
    if (!restrictions.m_canCreateTags) {
        return false;
    }

    return true;
}

void TagModel::updateRestrictionsFromNotebooks(
    const qevercloud::Guid & linkedNotebookGuid,
    const QList<qevercloud::Notebook> & notebooks)
{
    QNDEBUG("model::TagModel", "TagModel::updateRestrictionsFromNotebooks");

    Restrictions restrictions;
    restrictions.m_canCreateTags = true;
    restrictions.m_canUpdateTags = true;

    for (const auto & notebook: std::as_const(notebooks)) {
        if (!notebook.restrictions()) {
            continue;
        }

        const auto & notebookRestrictions = *notebook.restrictions();

        if (notebookRestrictions.noCreateTags().value_or(false)) {
            restrictions.m_canCreateTags = false;
        }

        if (notebookRestrictions.noUpdateTags().value_or(false)) {
            restrictions.m_canUpdateTags = false;
        }
    }

    m_tagRestrictionsByLinkedNotebookGuid[linkedNotebookGuid] =
        restrictions;

    QNTRACE(
        "model::TagModel",
        "Set restrictions for tags from linked notebook with "
            << "guid " << linkedNotebookGuid
            << ": can create tags = "
            << (restrictions.m_canCreateTags ? "true" : "false")
            << ", can update tags = "
            << (restrictions.m_canUpdateTags ? "true" : "false"));
}

void TagModel::onLinkedNotebookAddedOrUpdated(
    const qevercloud::LinkedNotebook & linkedNotebook)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::onLinkedNotebookAddedOrUpdated: " << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.guid())) {
        QNWARNING(
            "model::TagModel",
            "Can't process the addition or update of "
                << "a linked notebook without guid: " << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.username())) {
        QNWARNING(
            "model::TagModel",
            "Can't process the addition or update of "
                << "a linked notebook without username: " << linkedNotebook);
        return;
    }

    const auto & linkedNotebookGuid = *linkedNotebook.guid();

    auto it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(
        linkedNotebookGuid);

    if (it != m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end()) {
        if (it.value() == linkedNotebook.username()) {
            QNDEBUG("model::TagModel", "The username hasn't changed, nothing to do");
            return;
        }

        it.value() = *linkedNotebook.username();

        QNDEBUG(
            "model::TagModel",
            "Updated the username corresponding to linked "
                << "notebook guid " << linkedNotebookGuid << " to "
                << *linkedNotebook.username());
    }
    else {
        QNDEBUG(
            "model::TagModel",
            "Adding new username " << *linkedNotebook.username()
                                   << " corresponding to linked notebook guid "
                                   << linkedNotebookGuid);

        it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(
            linkedNotebookGuid, *linkedNotebook.username());
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
                *linkedNotebook.username(), linkedNotebookGuid));

        checkAndCreateModelRootItems();

        auto * linkedNotebookItem = &(linkedNotebookItemIt.value());
        int row = rowForNewItem(*m_allTagsRootItem, *linkedNotebookItem);
        beginInsertRows(indexForItem(m_allTagsRootItem), row, row);
        m_allTagsRootItem->insertChild(row, linkedNotebookItem);
        endInsertRows();
    }
    else {
        linkedNotebookItemIt->setUsername(*linkedNotebook.username());

        QNTRACE(
            "model::TagModel",
            "Updated the linked notebook username to "
                << *linkedNotebook.username()
                << " for linked notebook item corresponding to "
                << "linked notebook guid " << linkedNotebookGuid);
    }

    const auto linkedNotebookItemIndex =
        indexForLinkedNotebookGuid(linkedNotebookGuid);

    Q_EMIT dataChanged(linkedNotebookItemIndex, linkedNotebookItemIndex);
}

ITagModelItem * TagModel::itemForId(const IndexId id) const
{
    QNTRACE("model::TagModel", "TagModel::itemForId: " << id);

    if (id == m_allTagsRootItemIndexId) {
        return m_allTagsRootItem;
    }

    const auto localIdIt = m_indexIdToLocalIdBimap.left.find(id);
    if (localIdIt == m_indexIdToLocalIdBimap.left.end()) {
        const auto linkedNotebookGuidIt =
            m_indexIdToLinkedNotebookGuidBimap.left.find(id);

        if (linkedNotebookGuidIt ==
            m_indexIdToLinkedNotebookGuidBimap.left.end()) {
            QNDEBUG(
                "model::TagModel",
                "Found no tag model item corresponding to "
                    << "model index internal id");

            return nullptr;
        }

        const auto & linkedNotebookGuid = linkedNotebookGuidIt->second;
        const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
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

    const auto & localId = localIdIt->second;

    QNTRACE(
        "model::TagModel",
        "Found tag local id corresponding to model index "
            << "internal id: " << localId);

    const auto & localIdIndex = m_data.get<ByLocalId>();
    if (const auto it = localIdIndex.find(localId); it != localIdIndex.end()) {
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
        const auto it = m_indexIdToLocalIdBimap.right.find(tagItem->localId());
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
        const auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(
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
        "Detected attempt to assign id to unidentified tag model item: "
            << item);
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
            return QVariant{tagItem->name()};
        case Column::Synchronizable:
            return QVariant{tagItem->isSynchronizable()};
        case Column::Dirty:
            return QVariant{tagItem->isDirty()};
        case Column::FromLinkedNotebook:
            return QVariant{!tagItem->linkedNotebookGuid().isEmpty()};
        case Column::NoteCount:
            return QVariant{tagItem->noteCount()};
        default:
            return {};
        }
    }

    const auto * linkedNotebookItem = item.cast<TagLinkedNotebookRootItem>();
    if (linkedNotebookItem) {
        switch (column) {
        case Column::Name:
            return QVariant{linkedNotebookItem->username()};
        case Column::FromLinkedNotebook:
            return QVariant{true};
        default:
            return {};
        }
    }

    return {};
}

QVariant TagModel::dataAccessibleText(
    const ITagModelItem & item, const Column column) const
{
    auto textData = dataImpl(item, column);
    if (textData.isNull()) {
        return {};
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

    return QVariant{std::move(accessibleText)};
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
    if (const auto it = localIdIndex.find(localId); it != localIdIndex.end()) {
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
            "Tag model item has no parent, returning invalid index: " << *item);
        return {};
    }

    const int row = parentItem->rowForChild(item);
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
    const auto range = nameIndex.equal_range(tagName.toUpper());
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

    const auto it = m_linkedNotebookItems.find(linkedNotebookGuid);
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

    const int row = parentItem->rowForChild(modelItem);
    if (row < 0) {
        QNDEBUG(
            "model::TagModel",
            "Can't find row of promoted item within its "
                << "parent item");
        return {};
    }

    fixupItemParent(*parentItem);
    auto * grandParentItem = parentItem->parent();

    auto * grandParentTagItem = grandParentItem->cast<TagItem>();
    if (!canCreateTagItem(*grandParentItem) ||
        (grandParentTagItem && !canUpdateTagItem(*grandParentTagItem)))
    {
        REPORT_INFO(
            QT_TR_NOOP("Can't promote tag: can't create and/or update tags "
                       "for the grand parent tag due to restrictions"));
        return {};
    }

    const int parentRow = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(parentRow < 0)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't promote tag: can't find parent item's row within "
                       "the grand parent model item"));
        return {};
    }

    auto parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * takenItem = parentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(takenItem != modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't promote tag, detected "
                       "internal inconsistency in the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original promoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, takenItem);
        endInsertRows();

        return {};
    }

    const auto grandParentIndex = indexForItem(grandParentItem);
    const int appropriateRow = rowForNewItem(*grandParentItem, *takenItem);

    beginInsertRows(grandParentIndex, appropriateRow, appropriateRow);
    grandParentItem->insertChild(appropriateRow, takenItem);
    endInsertRows();

    const auto newIndex =
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
        parentItem->insertChild(row, takenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy{*tagItem};
    if (grandParentTagItem) {
        tagItemCopy.setParentLocalId(grandParentTagItem->localId());
        tagItemCopy.setParentGuid(grandParentTagItem->guid());
    }
    else {
        tagItemCopy.setParentLocalId(QString{});
        tagItemCopy.setParentGuid(QString{});
    }

    const bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(tagItemCopy.localId());
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
        const auto dirtyColumnIndex = index(
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

    const int row = parentItem->rowForChild(modelItem);
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

    auto * siblingItem = parentItem->childAtRow(row - 1);
    if (Q_UNLIKELY(!siblingItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: no sibling tag "
                       "appropriate for demoting was found"));
        return {};
    }

    auto * siblingTagItem = siblingItem->cast<TagItem>();
    if (Q_UNLIKELY(!siblingTagItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: the sibling model item "
                       "is not of a tag type"));
        return {};
    }

    const auto & itemLinkedNotebookGuid = tagItem->linkedNotebookGuid();
    const auto & siblingItemLinkedNotebookGuid =
        siblingTagItem->linkedNotebookGuid();

    if (parentItem == m_allTagsRootItem &&
        siblingItemLinkedNotebookGuid != itemLinkedNotebookGuid)
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
                  << "\nSibling item: " << *siblingItem);
        Q_EMIT notifyError(std::move(error));
        return {};
    }

    if (!canCreateTagItem(*siblingItem)) {
        REPORT_INFO(
            QT_TR_NOOP("Can't demote tag: can't create tags within "
                       "the sibling tag due to restrictions"));
        return {};
    }

    auto siblingItemIndex = indexForItem(siblingItem);
    if (Q_UNLIKELY(!siblingItemIndex.isValid())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't demote tag: can't get valid model index for "
                       "the sibling tag"));
        return {};
    }

    const auto parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    auto * takenItem = parentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(takenItem != modelItem)) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, detected "
                       "internal inconsistency within the tag model: "
                       "the item to take out from its parent doesn't "
                       "match the original demoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, takenItem);
        endInsertRows();

        return {};
    }

    const int appropriateRow = rowForNewItem(*siblingItem, *takenItem);

    // Need to update this index since its row within parent might have changed
    siblingItemIndex = indexForItem(siblingItem);
    beginInsertRows(siblingItemIndex, appropriateRow, appropriateRow);
    siblingItem->insertChild(appropriateRow, takenItem);
    endInsertRows();

    const auto newIndex =
        index(appropriateRow, static_cast<int>(Column::Name), siblingItemIndex);

    if (!newIndex.isValid()) {
        REPORT_ERROR(
            QT_TR_NOOP("Internal error: can't demote tag, invalid model index "
                       "was returned for the demoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(siblingItemIndex, appropriateRow, appropriateRow);
        Q_UNUSED(siblingItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        parentItem->insertChild(row, takenItem);
        endInsertRows();

        return {};
    }

    TagItem tagItemCopy{*tagItem};
    tagItemCopy.setParentLocalId(siblingTagItem->localId());
    tagItemCopy.setParentGuid(siblingTagItem->guid());

    const bool wasDirty = tagItemCopy.isDirty();
    tagItemCopy.setDirty(true);

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto it = localIdIndex.find(tagItemCopy.localId());
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
        const auto dirtyColumnIndex = index(
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

    const auto * tagItem = modelItem->cast<TagItem>();
    if (!tagItem) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't move non-tag model item to another parent"));
        return {};
    }

    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto tagItemIt = localIdIndex.find(tagItem->localId());
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
            "The tag is already under the parent with the correct name, "
                << "nothing to do");
        return index;
    }

    auto & nameIndex = m_data.get<ByNameUpper>();
    const auto newParentItemsRange = nameIndex.equal_range(parentTagName.toUpper());
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
            ErrorString error{
                QT_TR_NOOP("Can't set the parent of the tag to "
                           "one of its child tags")};
            QNINFO("model::TagModel", error);
            Q_EMIT notifyError(std::move(error));
            return {};
        }
    }

    removeModelItemFromParent(*modelItem);

    TagItem tagItemCopy{*tagItem};
    tagItemCopy.setParentLocalId(newParentTagItem->localId());
    tagItemCopy.setParentGuid(newParentTagItem->guid());
    tagItemCopy.setDirty(true);
    localIdIndex.replace(tagItemIt, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    const auto parentIndex = indexForItem(newParentItem);
    const int newRow = rowForNewItem(*newParentItem, *modelItem);

    beginInsertRows(parentIndex, newRow, newRow);
    newParentItem->insertChild(newRow, modelItem);
    endInsertRows();

    const auto newIndex = indexForItem(modelItem);
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
    const auto it = localIdIndex.find(tagItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't find the tag to be removed from its "
                       "parent within the tag model"));
        QNDEBUG("model::TagModel", "Tag item: " << *tagItem);
        return {};
    }

    removeModelItemFromParent(*modelItem);

    TagItem tagItemCopy{*tagItem};
    tagItemCopy.setParentGuid(QString{});
    tagItemCopy.setParentLocalId(QString{});
    tagItemCopy.setDirty(true);
    localIdIndex.replace(it, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    checkAndCreateModelRootItems();

    QNDEBUG(
        "model::TagModel",
        "Setting all tags root item as the new parent for the tag");

    setItemParent(*modelItem, *m_allTagsRootItem);

    const auto newIndex = indexForItem(modelItem);
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

    const auto tagNameSize = tagName.size();
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

    const auto existingItemIndex = indexForTagName(tagName, linkedNotebookGuid);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Tag with such name already exists"));

        return {};
    }

    if (!linkedNotebookGuid.isEmpty()) {
        const auto restrictionsIt =
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
    const int numExistingTags = static_cast<int>(localIdIndex.size());
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
        const auto parentTagRange =
            nameIndex.equal_range(parentTagName.toUpper());

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

    const auto insertionResult = localIdIndex.insert(item);
    auto * tagItem = const_cast<TagItem *>(&(*insertionResult.first));
    setItemParent(*tagItem, *parentItem);

    updateTagInLocalStorage(item);

    const auto addedTagIndex = indexForLocalId(item.localId());
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
        return {};
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

    for (auto it: qevercloud::toRange(m_linkedNotebookItems)) {
        auto & item = it.value();
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
        const auto range = parentLocalIdIndex.equal_range(tagItem->localId());
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

        const auto range = linkedNotebookGuidIndex.equal_range(
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

        tagNames.insert(item.nameUpper());
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
    const auto itemIt = localIdIndex.find(localId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        QNDEBUG("model::TagModel", "Can't find item to remove from the tag model");
        return;
    }

    auto * tagItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*tagItem);
    auto * parentItem = tagItem->parent();

    const int row = parentItem->rowForChild(tagItem);
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
        const auto childIt = parentLocalIdIndex.find(localId);
        if (childIt == parentLocalIdIndex.end()) {
            break;
        }

        removeItemByLocalId(childIt->localId());
    }

    const auto parentItemModelIndex = indexForItem(parentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();

    const auto indexIt = m_indexIdToLocalIdBimap.right.find(itemIt->localId());
    if (indexIt != m_indexIdToLocalIdBimap.right.end()) {
        Q_UNUSED(m_indexIdToLocalIdBimap.right.erase(indexIt))
    }

    localIdIndex.erase(itemIt);

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
    const int row = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(
            "model::TagModel",
            "Can't find the child tag item's row within its "
                << "parent; child item = " << item
                << ", parent item = " << *parentItem);
        return;
    }

    QNTRACE("model::TagModel", "Removing the child at row " << row);

    const auto parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(parentItem->takeChild(row))
    endRemoveRows();
}

void TagModel::restoreTagItemFromLocalStorage(const QString & localId)
{
    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto findTagFuture = m_localStorage->findTagByLocalId(localId);

    auto findTagThenFuture = threading::then(
        std::move(findTagFuture), this,
        [this, localId, canceler](const std::optional<qevercloud::Tag> & tag) {
            if (canceler->isCanceled()) {
                return;
            }

            if (Q_UNLIKELY(!tag)) {
                QNWARNING(
                    "model::TagModel",
                    "Could not find tag by local id in local storage");
                removeItemByLocalId(localId);
                return;
            }

            onTagAddedOrUpdated(*tag);
        });

    threading::onFailed(
        std::move(findTagThenFuture), this,
        [this, localId, canceler = std::move(canceler)](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            QNWARNING(
                "model::TagModel",
                "Failed to restore tag from local storage: "
                    << message);
            Q_EMIT notifyError(std::move(message));
        });
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

    const auto children = parentItem.children();
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

    const int currentItemRow = parentItem->rowForChild(&item);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(
            "model::TagModel",
            "Can't update tag model item's row: can't find "
                << "its original row within parent: " << item);
        return;
    }

    const auto parentIndex = indexForItem(parentItem);
    beginRemoveRows(parentIndex, currentItemRow, currentItemRow);
    Q_UNUSED(parentItem->takeChild(currentItemRow))
    endRemoveRows();

    const int appropriateRow = rowForNewItem(*parentItem, item);
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
    const auto indices = persistentIndexList();
    for (const auto & index: std::as_const(indices)) {
        auto * item = itemForId(static_cast<IndexId>(index.internalId()));
        const auto replacementIndex = indexForItem(item);
        changePersistentIndex(index, replacementIndex);
    }
}

void TagModel::updateTagInLocalStorage(const TagItem & item)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::updateTagInLocalStorage: local id = " << item.localId());

    qevercloud::Tag tag;

    auto notYetSavedItemIt =
        m_tagItemsNotYetInLocalStorageIds.find(item.localId());

    if (notYetSavedItemIt == m_tagItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG("model::TagModel", "Updating tag");

        const auto * cachedTag = m_cache.get(item.localId());
        if (Q_UNLIKELY(!cachedTag)) {
            auto canceler = setupCanceler();
            Q_ASSERT(canceler);

            auto findTagFuture =
                m_localStorage->findTagByLocalId(item.localId());

            auto findTagThenFuture = threading::then(
                std::move(findTagFuture), this,
                [this, canceler, localId = item.localId()](
                    const std::optional<qevercloud::Tag> & tag) {
                    if (canceler->isCanceled()) {
                        return;
                    }

                    if (Q_UNLIKELY(!tag)) {
                        ErrorString error{QT_TR_NOOP(
                            "Could not find tag in local storage by local id")};
                        error.details() = localId;
                        QNWARNING("model::TagModel", error);
                        Q_EMIT notifyError(std::move(error));
                        return;
                    }

                    m_cache.put(localId, *tag);
                    auto & localIdIndex = m_data.get<ByLocalId>();
                    if (const auto it = localIdIndex.find(localId);
                        it != localIdIndex.end())
                    {
                        updateTagInLocalStorage(*it);
                    }
                });

            threading::onFailed(
                std::move(findTagThenFuture), this,
                [this, canceler = std::move(canceler),
                 localId = item.localId()](const QException & e) {
                    if (canceler->isCanceled()) {
                        return;
                    }

                    auto message = exceptionMessage(e);
                    QNWARNING(
                        "model::TagModel",
                        "Failed to find and update tag in local storage; "
                            << "local id: " << localId << ", error: "
                            << message);
                    Q_EMIT notifyError(std::move(message));
                });

            return;
        }

        tag = *cachedTag;
    }

    tagFromItem(item, tag);

    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageIds.end()) {
        QNDEBUG(
            "model::TagModel", "Adding tag to local storage: " << tag);

        m_tagItemsNotYetInLocalStorageIds.erase(notYetSavedItemIt);

        auto canceler = setupCanceler();
        Q_ASSERT(canceler);

        auto putTagFuture = m_localStorage->putTag(std::move(tag));
        threading::onFailed(
            std::move(putTagFuture), this,
            [this, canceler = std::move(canceler),
             localId = item.localId()](const QException & e) {
                if (canceler->isCanceled()) {
                    return;
                }

                auto message = exceptionMessage(e);
                QNWARNING(
                    "model::TagModel",
                    "Failed to add tag to local storage: " << message);
                Q_EMIT notifyError(std::move(message));
                removeItemByLocalId(localId);
            });

        return;
    }

    // While the notebook is being updated in the local storage,
    // remove its stale copy from the cache
    m_cache.remove(tag.localId());

    QNDEBUG("model::TagModel", "Updating tag in local storage: " << tag);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto putTagFuture = m_localStorage->putTag(std::move(tag));
    threading::onFailed(
        std::move(putTagFuture), this,
        [this, canceler = std::move(canceler),
         localId = item.localId()](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            auto message = exceptionMessage(e);
            QNWARNING(
                "model::TagModel",
                "Failed to update tag in local storage: " << message);
            Q_EMIT notifyError(std::move(message));

            // Try to restore the tag to its actual version from
            // the local storage
            restoreTagItemFromLocalStorage(localId);
        });
}

void TagModel::tagFromItem(const TagItem & item, qevercloud::Tag & tag) const
{
    tag.setLocalId(item.localId());
    tag.setGuid(
        item.guid().isEmpty() ? std::nullopt : std::make_optional(item.guid()));

    tag.setLinkedNotebookGuid(
        item.linkedNotebookGuid().isEmpty()
        ? std::nullopt
        : std::make_optional(item.linkedNotebookGuid()));

    tag.setName(
        item.name().isEmpty() ? std::nullopt : std::make_optional(item.name()));

    tag.setLocalOnly(!item.isSynchronizable());
    tag.setLocallyModified(item.isDirty());
    tag.setLocallyFavorited(item.isFavorited());
    tag.setParentTagLocalId(item.parentLocalId());
    tag.setParentGuid(
        item.parentGuid().isEmpty() ? std::nullopt
                                    : std::make_optional(item.parentGuid()));
}

void TagModel::setNoteCountForTag(
    const QString & tagLocalId, const quint32 noteCount)
{
    auto & localIdIndex = m_data.get<ByLocalId>();
    const auto itemIt = localIdIndex.find(tagLocalId);
    if (Q_UNLIKELY(itemIt == localIdIndex.end())) {
        // Probably this tag was expunged
        QNDEBUG(
            "model::TagModel",
            "No tag receiving the note count update was found in the model: "
                << tagLocalId);
        return;
    }

    auto * modelItem = const_cast<TagItem *>(&(*itemIt));

    fixupItemParent(*modelItem);
    auto * parentItem = modelItem->parent();

    const int row = parentItem->rowForChild(modelItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error{
            QT_TR_NOOP("Can't find the row of tag model item being updated "
                       "with the note count within its parent")};

        QNWARNING(
            "model::TagModel",
            error << ", tag local id: " << tagLocalId
                  << "\nTag model item: " << *modelItem);

        Q_EMIT notifyError(std::move(error));
        return;
    }

    TagItem itemCopy{*itemIt};
    itemCopy.setNoteCount(noteCount);
    localIdIndex.replace(itemIt, itemCopy);

    const auto id = idForItem(*modelItem);
    const auto index = createIndex(row, static_cast<int>(Column::NoteCount), id);
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
    const auto it = localIdIndex.find(tagItem->localId());
    if (Q_UNLIKELY(it == localIdIndex.end())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't set favorited flag for the tag: the modified tag "
                       "entry was not found within the model"));
        return;
    }

    TagItem itemCopy{*tagItem};
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
            "Detected the request for finding of creation of a linked notebook "
                << "model item for empty linked notebook guid");
        return *m_allTagsRootItem;
    }

    auto linkedNotebookItemIt =
        m_linkedNotebookItems.find(linkedNotebookGuid);

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
            << ", will create one");

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

    TagLinkedNotebookRootItem tagLinkedNotebookRootItem(
        linkedNotebookOwnerUsername, linkedNotebookGuid);

    linkedNotebookItemIt = m_linkedNotebookItems.insert(
        linkedNotebookGuid, tagLinkedNotebookRootItem);

    auto * linkedNotebookItem = &(linkedNotebookItemIt.value());
    QNTRACE(
        "model::TagModel",
        "Linked notebook root item: " << *linkedNotebookItem);

    const int row = rowForNewItem(*m_allTagsRootItem, *linkedNotebookItem);
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

    const auto linkedNotebookGuid = linkedNotebookItem->linkedNotebookGuid();
    const auto indexIt =
        m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);

    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt);
    }

    const auto linkedNotebookItemIt =
        m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        m_linkedNotebookItems.erase(linkedNotebookItemIt);
    }
}

void TagModel::checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem)
{
    QNTRACE(
        "model::TagModel",
        "TagModel::checkAndFindLinkedNotebookRestrictions: " << tagItem);

    const auto & linkedNotebookGuid = tagItem.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        QNTRACE("model::TagModel", "No linked notebook guid");
        return;
    }

    const auto restrictionsIt =
        m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);

    if (restrictionsIt != m_tagRestrictionsByLinkedNotebookGuid.end()) {
        QNTRACE(
            "model::TagModel",
            "Already have the tag restrictions for linked notebook guid "
                << linkedNotebookGuid);
        return;
    }

    const auto it = m_pendingListingNotebooksByLinkedNotebookGuid.find(
        linkedNotebookGuid);

    if (it != m_pendingListingNotebooksByLinkedNotebookGuid.end()) {
        QNTRACE(
            "model::TagModel",
            "Already waiting for notebooks list by linked notebook guid to "
                << "figure out tag restrictions for linked notebook guid "
                << linkedNotebookGuid);
        return;
    }

    m_pendingListingNotebooksByLinkedNotebookGuid.insert(linkedNotebookGuid);

    local_storage::ILocalStorage::ListNotebooksOptions options;
    options.m_order = local_storage::ILocalStorage::ListNotebooksOrder::NoOrder;
    options.m_linkedNotebookGuids << linkedNotebookGuid;
    options.m_affiliation =
        local_storage::ILocalStorage::Affiliation::ParticularLinkedNotebooks;
    options.m_direction =
        local_storage::ILocalStorage::OrderDirection::Ascending;

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    auto listNotebooksFuture = m_localStorage->listNotebooks(options);

    auto listNotebooksThenFuture = threading::then(
        std::move(listNotebooksFuture), this,
        [this, canceler, linkedNotebookGuid](
            const QList<qevercloud::Notebook> & notebooks) {
            if (canceler->isCanceled()) {
                return;
            }

            m_pendingListingNotebooksByLinkedNotebookGuid.remove(
                linkedNotebookGuid);

            updateRestrictionsFromNotebooks(linkedNotebookGuid, notebooks);
        });

    threading::onFailed(
        std::move(listNotebooksThenFuture), this,
        [this, canceler = std::move(canceler),
         linkedNotebookGuid](const QException & e) {
            if (canceler->isCanceled()) {
                return;
            }

            m_pendingListingNotebooksByLinkedNotebookGuid.remove(
                linkedNotebookGuid);

            auto message = exceptionMessage(e);
            ErrorString error{QT_TR_NOOP(
                "Could not find tag restrictions for linked notebook")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING(
                "model::TagModel",
                error << ", linked notebook guid = " << linkedNotebookGuid);
            Q_EMIT notifyError(std::move(error));
        });
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

void TagModel::onLinkedNotebookExpunged(const qevercloud::Guid & guid)
{
    QStringList expungedTagLocalIds;
    const auto & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    const auto range = linkedNotebookGuidIndex.equal_range(guid);

    expungedTagLocalIds.reserve(
        static_cast<int>(std::distance(range.first, range.second)));

    for (auto it = range.first; it != range.second; ++it) {
        expungedTagLocalIds << it->localId();
    }

    for (const auto & tagLocalId: std::as_const(expungedTagLocalIds)) {
        removeItemByLocalId(tagLocalId);
    }

    if (const auto linkedNotebookItemIt = m_linkedNotebookItems.find(guid);
        linkedNotebookItemIt != m_linkedNotebookItems.end())
    {
        auto * modelItem = &(linkedNotebookItemIt.value());
        if (auto * parentItem = modelItem->parent(); parentItem) {
            const int row = parentItem->rowForChild(modelItem);
            if (row >= 0) {
                QModelIndex parentItemIndex = indexForItem(parentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(parentItem->takeChild(row))
                endRemoveRows();
            }
        }

        m_linkedNotebookItems.erase(linkedNotebookItemIt);
    }

    if (const auto indexIt =
            m_indexIdToLinkedNotebookGuidBimap.right.find(guid);
        indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end())
    {
        m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt);
    }
}

void TagModel::clearModel()
{
    QNDEBUG("model::TagModel", "TagModel::clearModel");

    beginResetModel();

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    m_data.clear();

    delete m_invisibleRootItem;
    m_invisibleRootItem = nullptr;

    delete m_allTagsRootItem;
    m_allTagsRootItem = nullptr;

    m_allTagsRootItemIndexId = 1;

    m_linkedNotebookItems.clear();
    m_indexIdToLocalIdBimap.clear();
    m_indexIdToLinkedNotebookGuidBimap.clear();

    m_listTagsOffset = 0;
    m_tagItemsNotYetInLocalStorageIds.clear();

    m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.clear();
    m_listLinkedNotebooksOffset = 0;

    m_tagRestrictionsByLinkedNotebookGuid.clear();
    m_pendingListingNotebooksByLinkedNotebookGuid.clear();

    m_allTagsListed = false;
    m_allLinkedNotebooksListed = false;

    endResetModel();
}

utility::cancelers::ICancelerPtr TagModel::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

void TagModel::checkAndCreateModelRootItems()
{
    if (Q_UNLIKELY(!m_invisibleRootItem)) {
        m_invisibleRootItem = new InvisibleTagRootItem;
        QNDEBUG("model::TagModel", "Created invisible root item");
    }

    if (Q_UNLIKELY(!m_allTagsRootItem)) {
        beginInsertRows(QModelIndex{}, 0, 0);
        m_allTagsRootItem = new AllTagsRootItem;
        m_allTagsRootItem->setParent(m_invisibleRootItem);
        endInsertRows();
        QNDEBUG("model::TagModel", "Created all tags root item");
    }
}

#define MODEL_ITEM_NAME(item, itemName)                                        \
    if (item.type() == ITagModelItem::Type::Tag) {                             \
        const auto * tagItem = item.cast<TagItem>();                           \
        if (tagItem) {                                                         \
            itemName = tagItem->nameUpper();                                   \
        }                                                                      \
    }                                                                          \
    else if (item.type() == ITagModelItem::Type::LinkedNotebook) {             \
        const auto * linkedNotebookItem =                                      \
            item.cast<TagLinkedNotebookRootItem>();                            \
        if (linkedNotebookItem) {                                              \
            itemName = linkedNotebookItem->username().toUpper();               \
        }                                                                      \
    }

bool TagModel::LessByName::operator()(
    const ITagModelItem & lhs, const ITagModelItem & rhs) const
{
    if (lhs.type() == ITagModelItem::Type::AllTagsRoot &&
        rhs.type() != ITagModelItem::Type::AllTagsRoot)
    {
        return false;
    }
    else if (
        lhs.type() != ITagModelItem::Type::AllTagsRoot &&
        rhs.type() == ITagModelItem::Type::AllTagsRoot)
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if (lhs.type() == ITagModelItem::Type::LinkedNotebook &&
        rhs.type() != ITagModelItem::Type::LinkedNotebook)
    {
        return false;
    }
    else if (
        lhs.type() != ITagModelItem::Type::LinkedNotebook &&
        rhs.type() == ITagModelItem::Type::LinkedNotebook)
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
    const ITagModelItem * lhs, const ITagModelItem * rhs) const
{
    if (!lhs) {
        return true;
    }

    if (!rhs) {
        return false;
    }

    return this->operator()(*lhs, *rhs);
}

bool TagModel::GreaterByName::operator()(
    const ITagModelItem & lhs, const ITagModelItem & rhs) const
{
    if (lhs.type() == ITagModelItem::Type::AllTagsRoot &&
        rhs.type() != ITagModelItem::Type::AllTagsRoot)
    {
        return false;
    }

    if (lhs.type() != ITagModelItem::Type::AllTagsRoot &&
        rhs.type() == ITagModelItem::Type::AllTagsRoot)
    {
        return true;
    }

    // NOTE: treating linked notebook item as the one always going after
    // the non-linked notebook item
    if (lhs.type() == ITagModelItem::Type::LinkedNotebook &&
        rhs.type() != ITagModelItem::Type::LinkedNotebook)
    {
        return false;
    }

    if (lhs.type() != ITagModelItem::Type::LinkedNotebook &&
        rhs.type() == ITagModelItem::Type::LinkedNotebook)
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
    const ITagModelItem * lhs, const ITagModelItem * rhs) const
{
    if (!lhs) {
        return true;
    }

    if (!rhs) {
        return false;
    }

    return this->operator()(*lhs, *rhs);
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
