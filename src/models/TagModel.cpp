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
#include <vector>

// Limit for the queries to the local storage
#define TAG_LIST_LIMIT (100)
#define LINKED_NOTEBOOK_LIST_LIMIT (40)

#define NUM_TAG_MODEL_COLUMNS (5)

#define REPORT_ERROR(error, ...) \
    ErrorString errorDescription(error); \
    QNWARNING(errorDescription << "" __VA_ARGS__ ); \
    Q_EMIT notifyError(errorDescription)

#define REPORT_INFO(info, ...) \
    ErrorString errorDescription(info); \
    QNINFO(errorDescription << "" __VA_ARGS__ ); \
    Q_EMIT notifyError(errorDescription)

namespace quentier {

TagModel::TagModel(const Account & account,
                   LocalStorageManagerAsync & localStorageManagerAsync,
                   TagCache & cache, QObject * parent) :
    ItemModel(parent),
    m_account(account),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_cache(cache),
    m_modelItemsByLocalUid(),
    m_modelItemsByLinkedNotebookGuid(),
    m_linkedNotebookItems(),
    m_indexIdToLocalUidBimap(),
    m_indexIdToLinkedNotebookGuidBimap(),
    m_lastFreeIndexId(1),
    m_listTagsOffset(0),
    m_listTagsRequestId(),
    m_tagItemsNotYetInLocalStorageUids(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_expungeTagRequestIds(),
    m_noteCountPerTagRequestIds(),
    m_noteCountsPerAllTagsRequestId(),
    m_findTagToRestoreFailedUpdateRequestIds(),
    m_findTagToPerformUpdateRequestIds(),
    m_findTagAfterNotelessTagsErasureRequestIds(),
    m_listTagsPerNoteRequestIds(),
    m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids(),
    m_listLinkedNotebooksOffset(0),
    m_listLinkedNotebooksRequestId(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_tagLocalUidsByNoteLocalUid(),
    m_tagRestrictionsByLinkedNotebookGuid(),
    m_findNotebookRequestForLinkedNotebookGuid(),
    m_lastNewTagNameCounter(0),
    m_lastNewTagNameCounterByLinkedNotebookGuid(),
    m_allTagsListed(false),
    m_allLinkedNotebooksListed(false)
{
    createConnections(localStorageManagerAsync);

    requestTagsList();
    requestLinkedNotebooksList();
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

bool TagModel::allTagsListed() const
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
}

void TagModel::favoriteTag(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("TagModel::favoriteTag: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row()
            << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", internal id = ") << index.internalId());

    setTagFavorited(index, true);
}

void TagModel::unfavoriteTag(const QModelIndex & index)
{
    QNDEBUG(QStringLiteral("TagModel::unfavoriteTag: index: is valid = ")
            << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", row = ") << index.row()
            << QStringLiteral(", column = ") << index.column()
            << QStringLiteral(", internal id = ") << index.internalId());

    setTagFavorited(index, false);
}

QString TagModel::localUidForItemName(const QString & itemName,
                                      const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("TagModel::localUidForItemName: name = ") << itemName
            << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid);

    QModelIndex index = indexForTagName(itemName, linkedNotebookGuid);
    const TagModelItem * pItem = itemForIndex(index);
    if (!pItem) {
        QNTRACE(QStringLiteral("No tag with such name was found"));
        return QString();
    }

    if (pItem->type() != TagModelItem::Type::Tag) {
        QNTRACE(QStringLiteral("Tag model item is not of tag type"));
        return QString();
    }

    const TagItem * pTagItem = pItem->tagItem();
    if (!pTagItem) {
        QNDEBUG(QStringLiteral("No tag item within the tag model item"));
        return QString();
    }

    return pTagItem->localUid();
}

QString TagModel::itemNameForLocalUid(const QString & localUid) const
{
    QNDEBUG(QStringLiteral("TagModel::itemNameForLocalUid: ") << localUid);

    const TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(localUid);
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNTRACE(QStringLiteral("No tag item with such local uid"));
        return QString();
    }

    return it->name();
}

QStringList TagModel::itemNames(const QString & linkedNotebookGuid) const
{
    return tagNames(linkedNotebookGuid);
}

bool TagModel::allItemsListed() const
{
    return m_allTagsListed && m_allLinkedNotebooksListed;
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

    if ((index.column() == Columns::Dirty) ||
        (index.column() == Columns::FromLinkedNotebook))
    {
        return indexFlags;
    }

    const TagModelItem * pItem = itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        return indexFlags;
    }

    if (pItem->type() != TagModelItem::Type::Tag) {
        return indexFlags;
    }

    const TagItem * pTagItem = pItem->tagItem();
    if (!pTagItem) {
        return indexFlags;
    }

    if (!canUpdateTagItem(*pTagItem)) {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        QModelIndex parentIndex = index;

        while(true)
        {
            const TagModelItem * pParentItem = itemForIndex(parentIndex);
            if (Q_UNLIKELY(!pParentItem)) {
                break;
            }

            if (pParentItem == m_fakeRootItem) {
                break;
            }

            if (pParentItem->type() != TagModelItem::Type::Tag) {
                return indexFlags;
            }

            const TagItem * pParentTagItem = pParentItem->tagItem();
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
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_TAG_MODEL_COLUMNS)) {
        return QVariant();
    }

    const TagModelItem * pItem = itemForIndex(index);
    if (!pItem) {
        return QVariant();
    }

    if (pItem == m_fakeRootItem) {
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
        return dataImpl(*pItem, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*pItem, column);
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

    const TagModelItem * pParentItem = itemForIndex(parent);
    return (pParentItem ? pParentItem->numChildren() : 0);
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

    const TagModelItem * pParentItem = itemForIndex(parent);
    if (!pParentItem) {
        return QModelIndex();
    }

    const TagModelItem * pItem = pParentItem->childAtRow(row);
    if (!pItem) {
        return QModelIndex();
    }

    IndexId id = idForItem(*pItem);
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

    const TagModelItem * pChildItem = itemForIndex(index);
    if (!pChildItem) {
        return QModelIndex();
    }

    const TagModelItem * pParentItem = pChildItem->parent();
    if (!pParentItem) {
        return QModelIndex();
    }

    if (pParentItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const TagModelItem * pGrandParentItem = pParentItem->parent();
    if (!pGrandParentItem) {
        return QModelIndex();
    }

    int row = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Internal inconsistency detected in TagModel: parent of the item can't find the item within its children: item = ")
                  << *pParentItem << QStringLiteral("\nParent item: ") << *pGrandParentItem);
        return QModelIndex();
    }

    IndexId id = idForItem(*pParentItem);
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
    QNDEBUG(QStringLiteral("TagModel::setData: row = ") << modelIndex.row()
            << QStringLiteral(", column = ") << modelIndex.column()
            << QStringLiteral(", internal id = ") << modelIndex.internalId()
            << QStringLiteral(", value = ") << value << QStringLiteral(", role = ") << role);

    if (role != Qt::EditRole) {
        QNDEBUG(QStringLiteral("Non-edit role, skipping"));
        return false;
    }

    if (!modelIndex.isValid()) {
        QNDEBUG(QStringLiteral("The model index is invalid, skipping"));
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        REPORT_ERROR(QT_TR_NOOP("The \"dirty\" flag can't be set manually for a tag"));
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        REPORT_ERROR(QT_TR_NOOP("The \"from linked notebook\" flag can't be set manually for a tag"));
        return false;
    }

    const TagModelItem * pItem = itemForIndex(modelIndex);
    if (!pItem) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: no tag model item found for model index"));
        return false;
    }

    if (pItem == m_fakeRootItem) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't set data for the invisible root item within the tag model"));
        return false;
    }

    if (pItem->type() != TagModelItem::Type::Tag) {
        QNDEBUG(QStringLiteral("The model index points to a non-tag item"));
        return false;
    }

    const TagItem * pTagItem = pItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        QNWARNING(QStringLiteral("Found no tag item under the tag model item of tag type"));
        return false;
    }

    if (!canUpdateTagItem(*pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't update the tag, restrictions apply"));
        return false;
    }

    TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    bool shouldMakeParentsSynchronizable = false;

    TagItem tagItemCopy = *pTagItem;
    bool dirty = tagItemCopy.isDirty();
    switch(modelIndex.column())
    {
    case Columns::Name:
        {
            QString newName = value.toString().trimmed();
            bool changed = (newName != tagItemCopy.name());
            if (!changed) {
                QNDEBUG(QStringLiteral("Tag name hasn't changed"));
                return true;
            }

            auto nameIt = nameIndex.find(newName.toUpper());
            if (nameIt != nameIndex.end()) {
                ErrorString error(QT_TR_NOOP("Can't change tag name: no two tags within the account are allowed "
                                             "to have the same name in a case-insensitive manner"));
                QNINFO(error << QStringLiteral(", suggested name = ") << newName);
                Q_EMIT notifyError(error);
                return false;
            }

            ErrorString errorDescription;
            if (!Tag::validateName(newName, &errorDescription)) {
                ErrorString error(QT_TR_NOOP("Can't change tag name"));
                error.appendBase(errorDescription.base());
                error.appendBase(errorDescription.additionalBases());
                error.details() = errorDescription.details();
                QNINFO(error << QStringLiteral("; suggested name = ") << newName);
                Q_EMIT notifyError(error);
                return false;
            }

            dirty = true;
            tagItemCopy.setName(newName);
            break;
        }
    case Columns::Synchronizable:
        {
            if (m_account.type() == Account::Type::Local) {
                ErrorString error(QT_TR_NOOP("Can't make the tag synchronizable within the local account"));
                QNINFO(error);
                Q_EMIT notifyError(error);
                return false;
            }

            if (tagItemCopy.isSynchronizable() && !value.toBool()) {
                ErrorString error(QT_TR_NOOP("Can't make already synchronizable tag not synchronizable"));
                QNINFO(error << QStringLiteral(", already synchronizable tag item: ") << tagItemCopy);
                Q_EMIT notifyError(error);
                return false;
            }

            dirty |= (value.toBool() != tagItemCopy.isSynchronizable());
            tagItemCopy.setSynchronizable(value.toBool());
            shouldMakeParentsSynchronizable = true;
            break;
        }
    default:
        QNINFO(QStringLiteral("Can't edit data for column ") << modelIndex.column()
               << QStringLiteral(" in the tag model"));
        return false;
    }

    tagItemCopy.setDirty(dirty);

    TagDataByLocalUid & index = m_data.get<ByLocalUid>();

    if (shouldMakeParentsSynchronizable)
    {
        QNDEBUG(QStringLiteral("Making the parents of the tag made synchronizable also synchronizable"));

        const TagModelItem * pProcessedItem = pItem;
        TagItem dummy;
        while(pProcessedItem->parent())
        {
            const TagModelItem * pParentItem = pProcessedItem->parent();
            if (pParentItem == m_fakeRootItem) {
                break;
            }

            if (pParentItem->type() != TagModelItem::Type::Tag) {
                break;
            }

            const TagItem * pParentTagItem = pParentItem->tagItem();
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
                ErrorString error(QT_TR_NOOP("Can't find one of currently made synchronizable tag's parent tags"));
                QNWARNING(error << QStringLiteral(", item: ") << dummy);
                Q_EMIT notifyError(error);
                return false;
            }

            index.replace(dummyIt, dummy);
            QModelIndex changedIndex = indexForLocalUid(dummy.localUid());
            if (Q_UNLIKELY(!changedIndex.isValid())) {
                ErrorString error(QT_TR_NOOP("Can't get the valid model index for one of currently "
                                             "made synchronizable tag's parent tags"));
                QNWARNING(error << QStringLiteral(", item for which the index was requested: ") << dummy);
                Q_EMIT notifyError(error);
                return false;
            }

            changedIndex = this->index(changedIndex.row(), Columns::Synchronizable,
                                       changedIndex.parent());
            Q_EMIT dataChanged(changedIndex, changedIndex);
            pProcessedItem = pParentItem;
        }
    }

    auto it = index.find(tagItemCopy.localUid());
    if (Q_UNLIKELY(it == index.end())) {
        ErrorString error(QT_TR_NOOP("Can't find the tag being modified"));
        QNWARNING(error << QStringLiteral(" by its local uid , item: ") << tagItemCopy);
        Q_EMIT notifyError(error);
        return false;
    }

    index.replace(it, tagItemCopy);
    Q_EMIT dataChanged(modelIndex, modelIndex);

    if (m_sortedColumn == Columns::Name) {
        updateItemRowWithRespectToSorting(*pItem);
    }

    updateTagInLocalStorage(tagItemCopy);

    QNDEBUG(QStringLiteral("Successfully set the data"));
    return true;
}

bool TagModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG(QStringLiteral("TagModel::insertRows: row = ") << row
            << QStringLiteral(", count = ") << count << QStringLiteral(", parent index: row = ")
            << parent.row() << QStringLiteral(", column = ") << parent.column()
            << QStringLiteral(", internal id = ") << parent.internalId());

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * pParentItem = (parent.isValid()
                                       ? itemForIndex(parent)
                                       : m_fakeRootItem);
    if (Q_UNLIKELY(!pParentItem)) {
        QNWARNING(QStringLiteral("Can't insert row into the tag model: can't find parent item per model index"));
        return false;
    }

    if ((pParentItem != m_fakeRootItem) && !canCreateTagItem(*pParentItem)) {
        QNINFO(QStringLiteral("Can't insert row into the tag item: restrictions apply: ") << *pParentItem);
        return false;
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingTags = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingTags + count >= m_account.tagCountMax())) {
        ErrorString error(QT_TR_NOOP("Can't create tag(s): the account can contain a limited number of tags"));
        error.details() = QString::number(m_account.tagCountMax());
        QNINFO(error);
        Q_EMIT notifyError(error);
        return false;
    }

    std::vector<TagDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        // Adding tag item
        TagItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewTag(QString()));
        item.setDirty(true);
        item.setSynchronizable(m_account.type() != Account::Type::Local);

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        // Adding wrapping model item
        TagModelItem modelItem(TagModelItem::Type::Tag, &(*(addedItems.back())));
        auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);
        modelItemInsertionResult.value().setParent(pParentItem);
    }
    endInsertRows();

    if (m_sortedColumn == Columns::Name)
    {
        Q_EMIT layoutAboutToBeChanged();

        for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it)
        {
            const TagItem & item = *(*it);
            auto tagModelItemIt = m_modelItemsByLocalUid.find(item.localUid());
            if (tagModelItemIt != m_modelItemsByLocalUid.end()) {
                updateItemRowWithRespectToSorting(tagModelItemIt.value());
            }
        }

        Q_EMIT layoutChanged();
    }

    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        updateTagInLocalStorage(*(*it));
    }

    QNDEBUG(QStringLiteral("Successfully inserted the rows"));
    return true;
}

bool TagModel::removeRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG(QStringLiteral("TagModel::removeRows: row = ") << row
            << QStringLiteral(", count = ") << count << QStringLiteral(", parent index: row = ")
            << parent.row() << QStringLiteral(", column = ") << parent.column()
            << QStringLiteral(", internal id = ") << parent.internalId());

    RemoveRowsScopeGuard removeRowsScopeGuard(*this);
    Q_UNUSED(removeRowsScopeGuard)

    if (!m_fakeRootItem) {
        QNDEBUG(QStringLiteral("No fake root item"));
        return false;
    }

    const TagModelItem * pParentItem = (parent.isValid()
                                        ? itemForIndex(parent)
                                        : m_fakeRootItem);
    if (!pParentItem) {
        QNDEBUG(QStringLiteral("No item corresponding to the parent index"));
        return false;
    }

    /**
     * First need to check if the rows to be removed are allowed to be removed
     */
    for(int i = 0; i < count; ++i)
    {
        const TagModelItem * pModelItem = pParentItem->childAtRow(row + i);
        if (!pModelItem) {
            QNWARNING(QStringLiteral("Detected null pointer to child tag item on attempt to remove row ")
                      << (row + i) << QStringLiteral(" from parent item: ") << *pParentItem);
            continue;
        }

        if (pModelItem->type() != TagModelItem::Type::Tag) {
            ErrorString error(QT_TR_NOOP("Can't remove tag linked notebook root item"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }

        const TagItem * pTagItem = pModelItem->tagItem();
        if (Q_UNLIKELY(!pTagItem)) {
            ErrorString error(QT_TR_NOOP("Internal error: found no tag item under the tag model item of tag type"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (!pTagItem->linkedNotebookGuid().isEmpty()) {
            ErrorString error(QT_TR_NOOP("Can't remove tag from linked notebook"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (pTagItem->isSynchronizable()) {
            ErrorString error(QT_TR_NOOP("Can't remove synchronizable tag"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }

        if (hasSynchronizableChildren(pModelItem)) {
            ErrorString error(QT_TR_NOOP("Can't remove tag with synchronizable children"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    /**
     * Need to re-parent all children of each removed item to the parent of the removed items i.e.
     * to make the grand-parent of each child its new parent. But before that will just take them away from the current parent
     * and ollect into a temporary list
     */
    QList<const TagModelItem*> removedItemsChildren;
    for(int i = 0; i < count; ++i)
    {
        const TagModelItem * pModelItem = pParentItem->childAtRow(row + i);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(QStringLiteral("Detected null pointer to tag model item within the items to be removed"));
            continue;
        }

        QModelIndex modelItemIndex = indexForItem(pModelItem);
        while (pModelItem->hasChildren())
        {
            beginRemoveRows(modelItemIndex, 0, 0);
            const TagModelItem * pChildItem = pModelItem->takeChild(0);
            endRemoveRows();

            if (Q_UNLIKELY(!pChildItem)) {
                continue;
            }

            TagItem childItemCopy(*pChildItem->tagItem());

            if ((pParentItem->type() == TagModelItem::Type::Tag) && pParentItem->tagItem()) {
                childItemCopy.setParentGuid(pParentItem->tagItem()->guid());
                childItemCopy.setParentLocalUid(pParentItem->tagItem()->localUid());
            }
            else {
                childItemCopy.setParentGuid(QString());
                childItemCopy.setParentLocalUid(QString());
            }

            childItemCopy.setDirty(true);

            auto tagItemIt = localUidIndex.find(childItemCopy.localUid());
            if (Q_UNLIKELY(tagItemIt == localUidIndex.end())) {
                QNINFO(QStringLiteral("The tag item which parent is being removed was not found within the model. Adding it there"));
                Q_UNUSED(localUidIndex.insert(childItemCopy))
            }
            else {
                localUidIndex.replace(tagItemIt, childItemCopy);
            }

            updateTagInLocalStorage(childItemCopy);

            /**
             * NOTE: no dataChanged signal here because the corresponding model item is now parentless
             * and hence is unavailable to the view
             */

            removedItemsChildren << pChildItem;
        }
    }

    /**
     * Actually remove the rows each of which has no children anymore
     */
    beginRemoveRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        const TagModelItem * pModelItem = pParentItem->takeChild(row);
        if (!pModelItem) {
            continue;
        }

        const TagItem * pTagItem = pModelItem->tagItem();

        Tag tag;
        tag.setLocalUid(pTagItem->localUid());

        QUuid requestId = QUuid::createUuid();
        Q_UNUSED(m_expungeTagRequestIds.insert(requestId))
        Q_EMIT expungeTag(tag, requestId);
        QNTRACE(QStringLiteral("Emitted the request to expunge the tag from the local storage: request id = ")
                << requestId << QStringLiteral(", tag local uid: ") << pTagItem->localUid());

        auto it = localUidIndex.find(pTagItem->localUid());
        if (it != localUidIndex.end()) {
            Q_UNUSED(localUidIndex.erase(it))
        }

        auto modelItemIt = m_modelItemsByLocalUid.find(tag.localUid());
        if (modelItemIt != m_modelItemsByLocalUid.end()) {
            Q_UNUSED(m_modelItemsByLocalUid.erase(modelItemIt))
        }

        auto indexIt = m_indexIdToLocalUidBimap.right.find(tag.localUid());
        if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
            Q_UNUSED(m_indexIdToLocalUidBimap.right.erase(indexIt))
        }
    }
    endRemoveRows();

    /**
     * Insert the previously collected children of the removed items under the removed items' parent item
     */
    while(!removedItemsChildren.isEmpty())
    {
        const TagModelItem * pChildItem = removedItemsChildren.takeAt(0);
        if (Q_UNLIKELY(!pChildItem)) {
            continue;
        }

        int newRow = rowForNewItem(*pParentItem, *pChildItem);
        beginInsertRows(parent, newRow, newRow);
        pParentItem->insertChild(newRow, pChildItem);
        endInsertRows();
    }

    QNDEBUG(QStringLiteral("Successfully removed the row(s)"));
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

    if (Q_UNLIKELY(!m_fakeRootItem)) {
        QNDEBUG(QStringLiteral("No fake root item, nothing to sort"));
        return;
    }

    m_sortOrder = order;
    Q_EMIT sortingChanged();

    Q_EMIT layoutAboutToBeChanged();

    if (m_sortOrder == Qt::AscendingOrder)
    {
        for(auto it = m_modelItemsByLocalUid.constBegin(); it != m_modelItemsByLocalUid.constEnd(); ++it) {
            it->sortChildren(LessByName());
        }

        for(auto it = m_modelItemsByLinkedNotebookGuid.constBegin(); it != m_modelItemsByLinkedNotebookGuid.constEnd(); ++it) {
            it->sortChildren(LessByName());
        }

        m_fakeRootItem->sortChildren(LessByName());
    }
    else
    {
        for(auto it = m_modelItemsByLocalUid.constBegin(); it != m_modelItemsByLocalUid.constEnd(); ++it) {
            it->sortChildren(GreaterByName());
        }

        for(auto it = m_modelItemsByLinkedNotebookGuid.constBegin(); it != m_modelItemsByLinkedNotebookGuid.constEnd(); ++it) {
            it->sortChildren(GreaterByName());
        }

        m_fakeRootItem->sortChildren(GreaterByName());
    }

    updatePersistentModelIndices();
    Q_EMIT layoutChanged();

    QNDEBUG(QStringLiteral("Successfully sorted the tag model"));
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
        return Q_NULLPTR;
    }

    const TagModelItem * pModelItem = itemForIndex(indexes.at(0));
    if (!pModelItem) {
        return Q_NULLPTR;
    }

    QByteArray encodedItem;
    QDataStream out(&encodedItem, QIODevice::WriteOnly);
    out << *pModelItem;

    QMimeData * pMimeData = new QMimeData;
    pMimeData->setData(TAG_MODEL_MIME_TYPE, qCompress(encodedItem, TAG_MODEL_MIME_DATA_MAX_COMPRESSION));
    return pMimeData;
}

bool TagModel::dropMimeData(const QMimeData * pMimeData, Qt::DropAction action,
                            int row, int column, const QModelIndex & parentIndex)
{
    QNDEBUG(QStringLiteral("TagModel::dropMimeData: action = ") << action
            << QStringLiteral(", row = ") << row << QStringLiteral(", column = ")
            << column << QStringLiteral(", parent index: is valid = ")
            << (parentIndex.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", parent row = ") << parentIndex.row()
            << QStringLiteral(", parent column = ") << (parentIndex.column())
            << QStringLiteral(", parent internal id: ")  << parentIndex.internalId()
            << QStringLiteral(", mime data formats: ")
            << (pMimeData ? pMimeData->formats().join(QStringLiteral("; ")) : QStringLiteral("<null>")));

    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (!pMimeData || !pMimeData->hasFormat(TAG_MODEL_MIME_TYPE)) {
        return false;
    }

    const TagModelItem * pNewParentItem = itemForIndex(parentIndex);
    if (!pNewParentItem) {
        REPORT_ERROR(QT_TR_NOOP("Internal error, can't move the tag: the new parent item was not found "
                                "within the tag model by model index"));
        return false;
    }

    if ((pNewParentItem != m_fakeRootItem) && !canCreateTagItem(*pNewParentItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move the tag under the new parent: restrictions apply "
                                "or the restrictions settings were not fetched yet"));
        return false;
    }

    if (pNewParentItem->type() != TagModelItem::Type::Tag) {
        QNDEBUG(QStringLiteral("Can't drop tags onto tag linked notebook root items"));
        return false;
    }

    const TagItem * pNewParentTagItem = pNewParentItem->tagItem();
    if (Q_UNLIKELY(!pNewParentTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move the tag under the new parent: the new parent model item "
                                "was recognized as a tag one but it has no inner tag item"));
    }

    QByteArray data = qUncompress(pMimeData->data(TAG_MODEL_MIME_TYPE));
    TagModelItem item;
    QDataStream in(&data, QIODevice::ReadOnly);
    in >> item;

    if (item.type() != TagModelItem::Type::Tag) {
        QNDEBUG(QStringLiteral("Can only drag-drop tag model items of tag type"));
        return false;
    }

    const TagItem * pTagItem = item.tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't move the tag under the new parent: the dropped model item "
                                "was recognized as a tag one but it has no inner tag item"));
        return false;
    }

    if (pTagItem->linkedNotebookGuid() != pNewParentTagItem->linkedNotebookGuid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't assign parent tags between linked notebooks or between user's tags "
                                "and those from a linked notebook"));
        return false;
    }

    // Check that we aren't trying to move the tag under one of its children
    const TagModelItem * pTrackedParentItem = pNewParentItem;
    while(pTrackedParentItem && (pTrackedParentItem != m_fakeRootItem))
    {
        if (pTrackedParentItem->tagItem() && (pTrackedParentItem->tagItem()->localUid() == pTagItem->localUid())) {
            ErrorString error(QT_TR_NOOP("Can't move the tag under one of its child tags"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return false;
        }

        pTrackedParentItem = pTrackedParentItem->parent();
    }

    if (pTagItem->parentLocalUid() == pNewParentTagItem->localUid()) {
        QNDEBUG(QStringLiteral("Item is already under the chosen parent, nothing to do"));
        return true;
    }

    TagItem tagItem(*pTagItem);
    tagItem.setParentLocalUid(pNewParentTagItem->localUid());
    tagItem.setParentGuid(pNewParentTagItem->guid());
    tagItem.setDirty(true);

    const TagModelItem * pModelItem = Q_NULLPTR;
    auto modelItemIt = m_modelItemsByLocalUid.find(tagItem.localUid());
    if (modelItemIt == m_modelItemsByLocalUid.end()) {
        TagModelItem modelItem(TagModelItem::Type::Tag, pTagItem);
        modelItemIt = m_modelItemsByLocalUid.insert(tagItem.localUid(), modelItem);
    }

    pModelItem = &(modelItemIt.value());

    if (row == -1)
    {
        if (!parentIndex.isValid() && !m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        row = parentIndex.isValid() ? parentIndex.row() : m_fakeRootItem->numChildren();
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto originalItemIt = localUidIndex.find(tagItem.localUid());
    if (originalItemIt != localUidIndex.end())
    {
        // Need to manually remove the tag model item from its original parent
        const TagItem & originalTagItem = *originalItemIt;
        const QString & originalItemParentLocalUid = originalTagItem.parentLocalUid();
        const QString & originalItemLinkedNotebookGuid = originalTagItem.linkedNotebookGuid();
        const TagModelItem * pOriginalItemParent = Q_NULLPTR;

        if (!originalItemParentLocalUid.isEmpty())
        {
            auto originalParentItemIt = m_modelItemsByLocalUid.find(originalItemParentLocalUid);
            if (originalParentItemIt != m_modelItemsByLocalUid.end()) {
                pOriginalItemParent = &(originalParentItemIt.value());
            }
        }
        else if (!originalItemLinkedNotebookGuid.isEmpty())
        {
            auto originalParentItemIt = m_modelItemsByLinkedNotebookGuid.find(originalItemLinkedNotebookGuid);
            if (originalParentItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
                pOriginalItemParent = &(originalParentItemIt.value());
            }
        }
        else {
            pOriginalItemParent = m_fakeRootItem;
        }

        if (!pOriginalItemParent)
        {
            if (!m_fakeRootItem) {
                m_fakeRootItem = new TagModelItem;
            }

            pOriginalItemParent = m_fakeRootItem;

            // NOTE: here we don't care about the proper row with respect to sorting because we'll be removing
            // this item from this parent further on anyway
            int row = pOriginalItemParent->numChildren();
            beginInsertRows(QModelIndex(), row, row);
            pModelItem->setParent(pOriginalItemParent);
            endInsertRows();
        }

        if (pOriginalItemParent->tagItem() && !pOriginalItemParent->tagItem()->linkedNotebookGuid().isEmpty()) {
            REPORT_ERROR(QT_TR_NOOP("Can't drag tag items from parent tags coming from linked notebook"));
            return false;
        }

        QModelIndex originalParentIndex = indexForItem(pOriginalItemParent);
        int originalItemRow = pOriginalItemParent->rowForChild(pModelItem);

        if (originalItemRow >= 0) {
            beginRemoveRows(originalParentIndex, originalItemRow, originalItemRow);
            Q_UNUSED(pOriginalItemParent->takeChild(originalItemRow));
            endRemoveRows();
            checkAndRemoveEmptyLinkedNotebookRootItem(*pOriginalItemParent);
        }
    }

    beginInsertRows(parentIndex, row, row);
    auto it = localUidIndex.end();
    if (originalItemIt != localUidIndex.end()) {
        localUidIndex.replace(originalItemIt, tagItem);
        it = originalItemIt;
    }
    else {
        auto insertionResult = localUidIndex.insert(tagItem);
        it = insertionResult.first;
    }

    pNewParentItem->insertChild(row, pModelItem);
    endInsertRows();

    updateItemRowWithRespectToSorting(*pModelItem);
    updateTagInLocalStorage(*it);

    QModelIndex index = indexForLocalUid(tagItem.localUid());
    Q_EMIT notifyTagParentChanged(index);

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

void TagModel::onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addTagRequestIds.find(requestId);
    if (it == m_addTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onAddTagFailed: tag = ") << tag << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_addTagRequestIds.erase(it))

    Q_EMIT notifyError(errorDescription);

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

void TagModel::onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
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
    Q_EMIT findTag(tag, requestId);
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

void TagModel::onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
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

    Q_EMIT notifyError(errorDescription);
}

void TagModel::onListTagsWithNoteLocalUidsComplete(LocalStorageManager::ListObjectsOptions flag,
                                                   size_t limit, size_t offset,
                                                   LocalStorageManager::ListTagsOrder::type order,
                                                   LocalStorageManager::OrderDirection::type orderDirection,
                                                   QString linkedNotebookGuid,
                                                   QList<std::pair<Tag,QStringList> > foundTagsWithNoteLocalUids,
                                                   QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListTagsWithNoteLocalUidsComplete: flag = ") << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid)
            << QStringLiteral(", num found tags = ") << foundTagsWithNoteLocalUids.size() << QStringLiteral(", request id = ") << requestId);

    for(auto it = foundTagsWithNoteLocalUids.constBegin(), end = foundTagsWithNoteLocalUids.constEnd(); it != end; ++it)
    {
        const Tag & tag = it->first;
        const QStringList & noteLocalUids = it->second;

        onTagAddedOrUpdated(tag, &noteLocalUids);

        for(auto nit = noteLocalUids.constBegin(), nend = noteLocalUids.constEnd(); nit != nend; ++nit)
        {
            const QString & noteLocalUid = *nit;
            if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
                continue;
            }

            QStringList & tagLocalUids = m_tagLocalUidsByNoteLocalUid[noteLocalUid];
            if (!tagLocalUids.contains(tag.localUid())) {
                tagLocalUids << tag.localUid();
            }
        }
    }

    m_listTagsRequestId = QUuid();

    if (!foundTagsWithNoteLocalUids.isEmpty()) {
        QNTRACE(QStringLiteral("The number of found tags is greater than zero, requesting more tags from the local storage"));
        m_listTagsOffset += static_cast<size_t>(foundTagsWithNoteLocalUids.size());
        requestTagsList();
        return;
    }

    m_allTagsListed = true;

    if (m_allLinkedNotebooksListed) {
        Q_EMIT notifyAllTagsListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void TagModel::onListTagsWithNoteLocalUidsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                 size_t limit, size_t offset,
                                                 LocalStorageManager::ListTagsOrder::type order,
                                                 LocalStorageManager::OrderDirection::type orderDirection,
                                                 QString linkedNotebookGuid, ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListTagsWithNoteLocalUidsFailed: flag = ") << flag << QStringLiteral(", limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ")
            << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid)
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listTagsRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void TagModel::onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeTagComplete: tag = ") << tag
            << QStringLiteral("\nExpunged child tag local uids: ") << expungedChildTagLocalUids.join(QStringLiteral(", "))
            << QStringLiteral(", request id = ") << requestId);

    auto it = m_expungeTagRequestIds.find(requestId);
    if (it != m_expungeTagRequestIds.end()) {
        Q_UNUSED(m_expungeTagRequestIds.erase(it))
        return;
    }

    Q_EMIT aboutToRemoveTags();
    removeItemByLocalUid(tag.localUid());   // NOTE: all child items would be removed from the model automatically
    Q_EMIT removedTags();
}

void TagModel::onExpungeTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
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

void TagModel::onGetNoteCountPerTagComplete(int noteCount, Tag tag, QUuid requestId)
{
    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onGetNoteCountPerTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ")
            << requestId << QStringLiteral(", note count = ") << noteCount);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto itemIt = localUidIndex.find(tag.localUid());
    if (Q_UNLIKELY(itemIt == localUidIndex.end())) {
        ErrorString error(QT_TR_NOOP("No tag receiving the note count update was found in the model"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag);
        Q_EMIT notifyError(error);
        return;
    }

    auto modelItemIt = m_modelItemsByLocalUid.find(itemIt->localUid());
    if (Q_UNLIKELY(modelItemIt == m_modelItemsByLocalUid.end())) {
        ErrorString error(QT_TR_NOOP("No tag model item corresponding to a receiving the note count update "
                                     "was found in the model"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag);
        Q_EMIT notifyError(error);
        return;
    }

    const TagModelItem * pModelItem = &(modelItemIt.value());
    const TagModelItem * pParentItem = pModelItem->parent();
    if (Q_UNLIKELY(!pParentItem)) {
        ErrorString error(QT_TR_NOOP("The tag model item being updated with the note count "
                                     "does not have a parent item linked with it"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag item: ") << *itemIt);
        Q_EMIT notifyError(error);
        return;
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(QT_TR_NOOP("Can't find the row of tag model item being updated with the note count "
                                     "within its parent"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag model item: ") << *pModelItem);
        Q_EMIT notifyError(error);
        return;
    }

    TagItem itemCopy = *itemIt;
    itemCopy.setNumNotesPerTag(noteCount);
    Q_UNUSED(localUidIndex.replace(itemIt, itemCopy))

    IndexId id = idForItem(*pModelItem);
    QModelIndex index = createIndex(row, Columns::NumNotesPerTag, id);
    Q_EMIT dataChanged(index, index);

    // NOTE: in future, if/when sorting by note count is supported, will need to check if need to re-sort and Q_EMIT the layout changed signal
}

void TagModel::onGetNoteCountPerTagFailed(ErrorString errorDescription, Tag tag, QUuid requestId)
{
    auto it = m_noteCountPerTagRequestIds.find(requestId);
    if (it == m_noteCountPerTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onGetNoteCountPerTagFailed: error description = ") << errorDescription
            << QStringLiteral(", tag = ") << tag << QStringLiteral(", request id = ") << requestId);

    Q_UNUSED(m_noteCountPerTagRequestIds.erase(it))

    ErrorString error(QT_TR_NOOP("Failed to get note count for one of tags"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void TagModel::onGetNoteCountsPerAllTagsComplete(QHash<QString, int> noteCountsPerTagLocalUid, QUuid requestId)
{
    if (requestId != m_noteCountsPerAllTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onGetNoteCountsPerAllTagsComplete: note counts were received for ")
            << noteCountsPerTagLocalUid.size() << QStringLiteral(" tag local uids; request id = ")
            << requestId);

    m_noteCountsPerAllTagsRequestId = QUuid();

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    for(auto it = localUidIndex.begin(), end = localUidIndex.end(); it != end; ++it)
    {
        TagItem item = *it;
        auto noteCountIt = noteCountsPerTagLocalUid.find(item.localUid());
        if (noteCountIt != noteCountsPerTagLocalUid.end()) {
            item.setNumNotesPerTag(noteCountIt.value());
        }
        else {
            item.setNumNotesPerTag(0);
        }

        localUidIndex.replace(it, item);

        const QString & parentLocalUid = item.parentLocalUid();
        const QString & linkedNotebookGuid = item.linkedNotebookGuid();
        if (parentLocalUid.isEmpty() && linkedNotebookGuid.isEmpty()) {
            continue;
        }

        // If tag item has either parent tag or linked notebook local uid, we'll send dataChanged signal for it here;
        // for all tags from user's own account and without parent tags we'll send dataChanged signal later, once for all such tags
        QModelIndex idx = indexForLocalUid(item.localUid());
        if (idx.isValid()) {
            idx = index(idx.row(), Columns::NumNotesPerTag, idx.parent());
            Q_EMIT dataChanged(idx, idx);
        }
    }

    QModelIndex startIndex = index(0, Columns::NumNotesPerTag, QModelIndex());
    QModelIndex endIndex = index(rowCount(QModelIndex()), Columns::NumNotesPerTag, QModelIndex());
    Q_EMIT dataChanged(startIndex, endIndex);
}

void TagModel::onGetNoteCountsPerAllTagsFailed(ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_noteCountsPerAllTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onGetNoteCountsPerAllTagsFailed: error description = ")
              << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_noteCountsPerAllTagsRequestId = QUuid();

    ErrorString error(QT_TR_NOOP("Failed to get note counts for tags"));
    error.appendBase(errorDescription.base());
    error.appendBase(errorDescription.additionalBases());
    error.details() = errorDescription.details();
    Q_EMIT notifyError(error);
}

void TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeNotelessTagsFromLinkedNotebooksComplete: request id = ") << requestId);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    for(auto it = localUidIndex.begin(); it != localUidIndex.end(); ++it)
    {
        const TagItem & item = *it;

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
        Q_EMIT findTag(tag, requestId);
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

void TagModel::onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId)
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
    requestNoteCountsPerAllTags();

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

    if (!note.hasTagLocalUids())
    {
        if (note.hasTagGuids()) {
            QNDEBUG(QStringLiteral("The note has tag guids but not tag local uids, need to request the proper list "
                                   "of tags from this note before their note counts can be updated"));
            requestTagsPerNote(note);
        }
        else {
            QNDEBUG(QStringLiteral("The note has no tags => no need to update the note count per any tag"));
        }

        return;
    }

    const QStringList & tagLocalUids = note.tagLocalUids();
    m_tagLocalUidsByNoteLocalUid[note.localUid()] = tagLocalUids;

    for(auto it = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); it != end; ++it) {
        Tag dummy;
        dummy.setLocalUid(*it);
        requestNoteCountForTag(dummy);
    }
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

    QStringList oldTagLocalUids;
    auto it = m_tagLocalUidsByNoteLocalUid.find(note.localUid());
    if (it != m_tagLocalUidsByNoteLocalUid.end()) {
        oldTagLocalUids = it.value();
    }

    QStringList newTagLocalUids = (note.hasTagLocalUids() ? note.tagLocalUids() : QStringList());

    bool sameTags = (oldTagLocalUids.size() == newTagLocalUids.size());
    if (sameTags)
    {
        for(auto oldTagIt = oldTagLocalUids.constBegin(), end = oldTagLocalUids.constEnd(); oldTagIt != end; ++oldTagIt)
        {
            if (!newTagLocalUids.contains(*oldTagIt)) {
                sameTags = false;
                break;
            }
        }
    }

    if (sameTags) {
        QNDEBUG(QStringLiteral("The list of this note's tags hasn't changed, no need to update the note count per any tag: ")
                << oldTagLocalUids.join(QStringLiteral(", ")));
        return;
    }

    QNDEBUG(QStringLiteral("The list of this note's tags has changed, need to update the note count per both old and new tags"));
    QNTRACE(QStringLiteral("Old tags: ") << oldTagLocalUids.join(QStringLiteral(", "))
            << QStringLiteral("; new tags: ") << newTagLocalUids.join(QStringLiteral(", ")));

    QStringList tagsToUpdate = oldTagLocalUids;
    tagsToUpdate << newTagLocalUids;
    Q_UNUSED(tagsToUpdate.removeDuplicates())
    QNTRACE(QStringLiteral("Local uids of tags for which the update of note count is needed: ")
            << tagsToUpdate.join(QStringLiteral(", ")));

    for(auto tagIt = tagsToUpdate.constBegin(), end = tagsToUpdate.constEnd(); tagIt != end; ++tagIt) {
        Tag dummy;
        dummy.setLocalUid(*tagIt);
        requestNoteCountForTag(dummy);
    }

    // Finally, update tag local uids per note local uid in our own hash
    if (it != m_tagLocalUidsByNoteLocalUid.end())
    {
        if (!newTagLocalUids.isEmpty()) {
            it.value() = newTagLocalUids;
        }
        else {
            Q_UNUSED(m_tagLocalUidsByNoteLocalUid.erase(it))
        }
    }
    else if (!newTagLocalUids.isEmpty())
    {
        m_tagLocalUidsByNoteLocalUid[note.localUid()] = newTagLocalUids;
    }
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

    QNDEBUG(QStringLiteral("Note has no tag local uids"));

    auto it = m_tagLocalUidsByNoteLocalUid.find(note.localUid());
    if (it != m_tagLocalUidsByNoteLocalUid.end())
    {
        const QStringList & tagLocalUids = it.value();
        QNTRACE(QStringLiteral("Last known tag local uids for the expunged note: ")
                << tagLocalUids.join(QStringLiteral(", ")));

        for(auto tagIt = tagLocalUids.constBegin(), end = tagLocalUids.constEnd(); tagIt != end; ++tagIt) {
            Tag dummy;
            dummy.setLocalUid(*tagIt);
            requestNoteCountForTag(dummy);
        }

        // Finally, erasing the entry for this note from our hash since the note was expunged
        Q_UNUSED(m_tagLocalUidsByNoteLocalUid.erase(it))

        return;
    }

    QNDEBUG(QStringLiteral("Found no cached tag local uids for this note"));

    // Inefficient fallback which should be used rarely if at all
    requestNoteCountsPerAllTags();
}

void TagModel::onAddLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onAddLinkedNotebookComplete: request id = ")
            << requestId << QStringLiteral(", linked notebook: ") << linkedNotebook);
    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onUpdateLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onUpdateLinkedNotebookComplete: request id = ")
            << requestId << QStringLiteral(", linked notebook: ") << linkedNotebook);
    onLinkedNotebookAddedOrUpdated(linkedNotebook);
}

void TagModel::onExpungeLinkedNotebookComplete(LinkedNotebook linkedNotebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("TagModel::onExpungeLinkedNotebookComplete: request id = ")
            << requestId << QStringLiteral(", linked notebook: ") << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(QStringLiteral("Received linked notebook expunged event but the linked notebook has no guid: ")
                  << linkedNotebook << QStringLiteral(", request id = ") << requestId);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    QStringList expungedTagLocalUids;
    const TagDataByLinkedNotebookGuid & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
    auto range = linkedNotebookGuidIndex.equal_range(linkedNotebookGuid);
    expungedTagLocalUids.reserve(static_cast<int>(std::distance(range.first, range.second)));
    for(auto it = range.first; it != range.second; ++it) {
        expungedTagLocalUids << it->localUid();
    }

    for(auto it = expungedTagLocalUids.constBegin(), end = expungedTagLocalUids.constEnd(); it != end; ++it) {
        removeItemByLocalUid(*it);
    }

    auto modelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (modelItemIt != m_modelItemsByLinkedNotebookGuid.end())
    {
        const TagModelItem * pModelItem = &(modelItemIt.value());
        const TagModelItem * pParentItem = pModelItem->parent();
        if (pParentItem)
        {
            int row = pParentItem->rowForChild(pModelItem);
            if (row >= 0)
            {
                QModelIndex parentItemIndex = indexForItem(pParentItem);
                beginRemoveRows(parentItemIndex, row, row);
                Q_UNUSED(pParentItem->takeChild(row))
                endRemoveRows();
            }
        }

        Q_UNUSED(m_modelItemsByLinkedNotebookGuid.erase(modelItemIt))
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        Q_UNUSED(m_linkedNotebookItems.erase(linkedNotebookItemIt))
    }

    auto indexIt = m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);
    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }
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
                                          ErrorString errorDescription, QUuid requestId)
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
    requestNoteCountsPerAllTags();
}

void TagModel::onListAllLinkedNotebooksComplete(size_t limit, size_t offset,
                                                LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                QList<LinkedNotebook> foundLinkedNotebooks,
                                                QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListAllLinkedNotebooksComplete: limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ")
            << order << QStringLiteral(", order direction = ") << orderDirection
            << QStringLiteral(", request id = ") << requestId);

    for(auto it = foundLinkedNotebooks.constBegin(), end = foundLinkedNotebooks.constEnd(); it != end; ++it) {
        onLinkedNotebookAddedOrUpdated(*it);
    }

    m_listLinkedNotebooksRequestId = QUuid();

    if (!foundLinkedNotebooks.isEmpty()) {
        QNTRACE(QStringLiteral("The number of found linked notebooks is not empty, requesting more "
                               "linked notebooks from the local storage"));
        m_listLinkedNotebooksOffset += static_cast<size_t>(foundLinkedNotebooks.size());
        requestLinkedNotebooksList();
        return;
    }

    m_allLinkedNotebooksListed = true;

    if (m_allTagsListed) {
        Q_EMIT notifyAllTagsListed();
        Q_EMIT notifyAllItemsListed();
    }
}

void TagModel::onListAllLinkedNotebooksFailed(size_t limit, size_t offset,
                                              LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                              LocalStorageManager::OrderDirection::type orderDirection,
                                              ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("TagModel::onListAllLinkedNotebooksFailed: limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
            << QStringLiteral(", order direction = ") << orderDirection << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    m_listLinkedNotebooksRequestId = QUuid();

    Q_EMIT notifyError(errorDescription);
}

void TagModel::createConnections(LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG(QStringLiteral("TagModel::createConnections"));

    // Local signals to localStorageManagerAsync's slots
    QObject::connect(this, QNSIGNAL(TagModel,addTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onAddTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,updateTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,findTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,listTagsWithNoteLocalUids,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                    LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onListTagsWithNoteLocalUidsRequest,LocalStorageManager::ListObjectsOptions,
                                                       size_t,size_t,LocalStorageManager::ListTagsOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,expungeTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onExpungeTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,requestNoteCountPerTag,Tag,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onGetNoteCountPerTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,requestNoteCountsForAllTags,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onGetNoteCountsPerAllTagsRequest,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,listAllTagsPerNote,Note,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                    LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerAsync,
                     QNSLOT(LocalStorageManagerAsync,onListAllTagsPerNoteRequest,Note,LocalStorageManager::ListObjectsOptions,
                            size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(TagModel,listAllLinkedNotebooks,size_t,size_t,
                                    LocalStorageManager::ListLinkedNotebooksOrder::type,
                                    LocalStorageManager::OrderDirection::type,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onListAllLinkedNotebooksRequest,
                                                       size_t,size_t,LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QUuid));

    // localStorageManagerAsync's signals to local slots
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onAddTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onAddTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onUpdateTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findTagComplete,Tag,QUuid),
                     this, QNSLOT(TagModel,onFindTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onFindTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listTagsWithNoteLocalUidsComplete,LocalStorageManager::ListObjectsOptions,
                                                         size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                         QString,QList<std::pair<Tag,QStringList> >,QUuid),
                     this, QNSLOT(TagModel,onListTagsWithNoteLocalUidsComplete,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,QList<std::pair<Tag,QStringList> >,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listTagsWithNoteLocalUidsFailed,LocalStorageManager::ListObjectsOptions,
                                                         size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,
                                                         QString,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onListTagsWithNoteLocalUidsFailed,LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                  LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QString,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagComplete,Tag,QStringList,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onExpungeTagFailed,Tag,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountPerTagComplete,int,Tag,QUuid),
                     this, QNSLOT(TagModel,onGetNoteCountPerTagComplete,int,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountPerTagFailed,ErrorString,Tag,QUuid),
                     this, QNSLOT(TagModel,onGetNoteCountPerTagFailed,ErrorString,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountsPerAllTagsComplete,QHash<QString,int>,QUuid),
                     this, QNSLOT(TagModel,onGetNoteCountsPerAllTagsComplete,QHash<QString,int>,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,getNoteCountsPerAllTagsFailed,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onGetNoteCountsPerAllTagsFailed,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                     this, QNSLOT(TagModel,onExpungeNotelessTagsFromLinkedNotebooksComplete,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,findNotebookFailed,Notebook,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onFindNotebookFailed,Notebook,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(TagModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this, QNSLOT(TagModel,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(TagModel,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(TagModel,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,addLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(TagModel,onAddLinkedNotebookComplete,LinkedNotebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(TagModel,onUpdateLinkedNotebookComplete,LinkedNotebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(TagModel,onExpungeLinkedNotebookComplete,LinkedNotebook,QUuid));
    QObject::connect(&localStorageManagerAsync,
                     QNSIGNAL(LocalStorageManagerAsync,listAllTagsPerNoteComplete,QList<Tag>,Note,LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     this,
                     QNSLOT(TagModel,onListAllTagsPerNoteComplete,QList<Tag>,Note,LocalStorageManager::ListObjectsOptions,
                              size_t,size_t,LocalStorageManager::ListTagsOrder::type,LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listAllLinkedNotebooksComplete,
                                                         size_t,size_t,LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,
                                                         QList<LinkedNotebook>,QUuid),
                     this, QNSLOT(TagModel,onListAllLinkedNotebooksComplete,size_t,size_t,
                                  LocalStorageManager::ListLinkedNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,listAllLinkedNotebooksFailed,
                                                         size_t,size_t,LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,ErrorString,QUuid),
                     this, QNSLOT(TagModel,onListAllLinkedNotebooksFailed,size_t,size_t,
                                  LocalStorageManager::ListLinkedNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,ErrorString,QUuid));
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
    Q_EMIT listTagsWithNoteLocalUids(flags, TAG_LIST_LIMIT, m_listTagsOffset, order, direction, QString(), m_listTagsRequestId);
}

void TagModel::requestNoteCountForTag(const Tag & tag)
{
    QNDEBUG(QStringLiteral("TagModel::requestNoteCountForTag: ") << tag);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_noteCountPerTagRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to compute the number of notes per tag, request id = ") << requestId);
    Q_EMIT requestNoteCountPerTag(tag, requestId);
}

void TagModel::requestTagsPerNote(const Note & note)
{
    QNDEBUG(QStringLiteral("TagModel::requestTagsPerNote: ") << note);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_listTagsPerNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to list tags per note: request id = ") << requestId);
    Q_EMIT listAllTagsPerNote(note, LocalStorageManager::ListAll,
                              /* limit = */ 0, /* offset = */ 0, LocalStorageManager::ListTagsOrder::NoOrder,
                              LocalStorageManager::OrderDirection::Ascending, requestId);
}

void TagModel::requestNoteCountsPerAllTags()
{
    QNDEBUG(QStringLiteral("TagModel::requestNoteCountsPerAllTags"));

    m_noteCountsPerAllTagsRequestId = QUuid::createUuid();
    Q_EMIT requestNoteCountsForAllTags(m_noteCountsPerAllTagsRequestId);
}

void TagModel::requestLinkedNotebooksList()
{
    QNDEBUG(QStringLiteral("TagModel::requestLinkedNotebooksList"));

    LocalStorageManager::ListLinkedNotebooksOrder::type order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    LocalStorageManager::OrderDirection::type direction = LocalStorageManager::OrderDirection::Ascending;

    m_listLinkedNotebooksRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to list linked notebooks: offset = ") << m_listLinkedNotebooksOffset
            << QStringLiteral(", request id = ") << m_listLinkedNotebooksRequestId);
    Q_EMIT listAllLinkedNotebooks(LINKED_NOTEBOOK_LIST_LIMIT, m_listLinkedNotebooksOffset, order, direction, m_listLinkedNotebooksRequestId);
}

void TagModel::onTagAddedOrUpdated(const Tag & tag, const QStringList * pTagNoteLocalUids)
{
    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    m_cache.put(tag.localUid(), tag);

    auto itemIt = localUidIndex.find(tag.localUid());
    bool newTag = (itemIt == localUidIndex.end());
    if (newTag)
    {
        Q_EMIT aboutToAddTag();

        onTagAdded(tag, pTagNoteLocalUids);

        QModelIndex addedTagIndex = indexForLocalUid(tag.localUid());
        Q_EMIT addedTag(addedTagIndex);
    }
    else
    {
        QModelIndex tagIndexBefore = indexForLocalUid(tag.localUid());
        Q_EMIT aboutToUpdateTag(tagIndexBefore);

        onTagUpdated(tag, itemIt, pTagNoteLocalUids);

        QModelIndex tagIndexAfter = indexForLocalUid(tag.localUid());
        Q_EMIT updatedTag(tagIndexAfter);
    }
}

void TagModel::onTagAdded(const Tag & tag, const QStringList * pTagNoteLocalUids)
{
    QNDEBUG(QStringLiteral("TagModel::onTagAdded: tag local uid = ") << tag.localUid()
            << QStringLiteral(", tag note local uids: ")
            << (pTagNoteLocalUids ? pTagNoteLocalUids->join(QStringLiteral(", ")) : QStringLiteral("none")));

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    const TagModelItem * pParentItem = Q_NULLPTR;

    if (tag.hasParentLocalUid())
    {
        auto parentIt = m_modelItemsByLocalUid.find(tag.parentLocalUid());
        if (parentIt != m_modelItemsByLocalUid.end()) {
            pParentItem = &(parentIt.value());
        }
    }
    else if (tag.hasLinkedNotebookGuid())
    {
        const QString & linkedNotebookGuid = tag.linkedNotebookGuid();
        pParentItem = &(findOrCreateLinkedNotebookModelItem(linkedNotebookGuid));
    }

    if (!pParentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        pParentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    TagItem item;
    tagToItem(tag, item);

    checkAndFindLinkedNotebookRestrictions(item);

    if (pTagNoteLocalUids) {
        item.setNumNotesPerTag(pTagNoteLocalUids->size());
    }

    // The new item is going to be inserted into the last row of the parent item
    auto insertionResult = localUidIndex.insert(item);
    auto it = insertionResult.first;
    const TagItem * pItem = &(*it);

    auto modelItemIt = m_modelItemsByLocalUid.insert(pItem->localUid(), TagModelItem(TagModelItem::Type::Tag, pItem));
    const TagModelItem * pModelItem = &(modelItemIt.value());

    int row = rowForNewItem(*pParentItem, *pModelItem);

    beginInsertRows(parentIndex, row, row);
    pParentItem->insertChild(row, pModelItem);
    endInsertRows();

    mapChildItems(*pModelItem);
}

void TagModel::onTagUpdated(const Tag & tag, TagDataByLocalUid::iterator it, const QStringList * pTagNoteLocalUids)
{
    QNDEBUG(QStringLiteral("TagModel::onTagUpdated: tag local uid = ") << tag.localUid()
            << QStringLiteral(", tag note local uids: ")
            << (pTagNoteLocalUids ? pTagNoteLocalUids->join(QStringLiteral(", ")) : QStringLiteral("none")));

    TagItem itemCopy;
    tagToItem(tag, itemCopy);

    if (pTagNoteLocalUids) {
        itemCopy.setNumNotesPerTag(pTagNoteLocalUids->size());
    }

    const TagItem * pTagItem = &(*it);
    const TagModelItem & modelItem = modelItemForTagItem(*pTagItem);

    const TagModelItem * pParentItem = modelItem.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        ErrorString error(QT_TR_NOOP("Tag model item being updated does not have a parent item linked with it"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag model item: ") << modelItem);
        Q_EMIT notifyError(error);
        return;
    }

    int row = pParentItem->rowForChild(&modelItem);
    if (Q_UNLIKELY(row < 0)) {
        ErrorString error(QT_TR_NOOP("Can't find the row of the updated tag item within its parent"));
        QNWARNING(error << QStringLiteral(", tag: ") << tag << QStringLiteral("\nTag model item: ") << modelItem);
        Q_EMIT notifyError(error);
        return;
    }

    const TagModelItem * pNewParentItem = Q_NULLPTR;
    if (tag.hasParentLocalUid())
    {
        auto parentIt = m_modelItemsByLocalUid.find(tag.parentLocalUid());
        if (parentIt != m_modelItemsByLocalUid.end()) {
            pNewParentItem = &(parentIt.value());
        }
    }
    else if (tag.hasLinkedNotebookGuid())
    {
        auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
        if (linkedNotebookModelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
            pNewParentItem = &(linkedNotebookModelItemIt.value());
        }
    }

    if (!pNewParentItem)
    {
        if (Q_UNLIKELY(!m_fakeRootItem)) {
            m_fakeRootItem = new TagModelItem;
        }

        pNewParentItem = m_fakeRootItem;
    }

    // NOTE: it's ok for any of these indexes to be invalid since either of them
    // can be the index of the fake root item
    QModelIndex parentItemIndex = indexForItem(pParentItem);
    QModelIndex newParentItemIndex = ((pParentItem == pNewParentItem)
                                      ? parentItemIndex
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

    int numNotesPerTag = it->numNotesPerTag();
    itemCopy.setNumNotesPerTag(numNotesPerTag);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    Q_UNUSED(localUidIndex.replace(it, itemCopy))
    pNewParentItem->insertChild(row, &modelItem);

    endInsertRows();

    QModelIndex modelIndexFrom = index(row, 0, newParentItemIndex);
    QModelIndex modelIndexTo = index(row, NUM_TAG_MODEL_COLUMNS - 1, newParentItemIndex);
    Q_EMIT dataChanged(modelIndexFrom, modelIndexTo);

    // 3) Ensure all the child tag model items are properly located under this tag model item
    QModelIndex modelItemIndex = indexForItem(&modelItem);
    TagDataByParentLocalUid & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
    auto range = parentLocalUidIndex.equal_range(pTagItem->localUid());
    for(auto childIt = range.first; childIt != range.second; ++childIt)
    {
        const TagItem & childItem = *childIt;
        const QString & childItemLocalUid = childItem.localUid();

        auto childModelItemIt = m_modelItemsByLocalUid.find(childItemLocalUid);
        if (childModelItemIt != m_modelItemsByLocalUid.end())
        {
            const TagModelItem & childModelItem = childModelItemIt.value();
            int row = modelItem.rowForChild(&childModelItem);
            if (row >= 0) {
                continue;
            }

            row = rowForNewItem(modelItem, childModelItem);
            beginInsertRows(modelItemIndex, row, row);
            modelItem.insertChild(row, &childModelItem);
            endInsertRows();
        }
    }

    // 4) Update the position of the updated item within its new parent
    updateItemRowWithRespectToSorting(modelItem);
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

    QNTRACE(QStringLiteral("Created tag model item from tag; item: ") << item << QStringLiteral("\nTag: ") << tag);
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

bool TagModel::canCreateTagItem(const TagModelItem & parentItem) const
{
    if (parentItem.type() != TagModelItem::Type::Tag) {
        return false;
    }

    const TagItem * pTagItem = parentItem.tagItem();
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
    QNDEBUG(QStringLiteral("TagModel::updateRestrictionsFromNotebook: local uid = ") << notebook.localUid()
            << QStringLiteral(", linked notebook guid = ")
            << (notebook.hasLinkedNotebookGuid() ? notebook.linkedNotebookGuid() : QStringLiteral("<null>")));

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

void TagModel::onLinkedNotebookAddedOrUpdated(const LinkedNotebook & linkedNotebook)
{
    QNDEBUG(QStringLiteral("TagModel::onLinkedNotebookAddedOrUpdated: ") << linkedNotebook);

    if (Q_UNLIKELY(!linkedNotebook.hasGuid())) {
        QNWARNING(QStringLiteral("Can't process the addition or update of a linked notebook without guid: ")
                  << linkedNotebook);
        return;
    }

    if (Q_UNLIKELY(!linkedNotebook.hasUsername())) {
        QNWARNING(QStringLiteral("Can't process the addition or update of a linked notebook without username: ")
                  << linkedNotebook);
        return;
    }

    const QString & linkedNotebookGuid = linkedNotebook.guid();

    auto it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(linkedNotebookGuid);
    if (it != m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end())
    {
        if (it.value() == linkedNotebook.username()) {
            QNDEBUG(QStringLiteral("The username hasn't changed, nothing to do"));
            return;
        }

        it.value() = linkedNotebook.username();
        QNDEBUG(QStringLiteral("Updated the username corresponding to linked notebook guid ") << linkedNotebookGuid
                << QStringLiteral(" to ") << linkedNotebook.username());
    }
    else
    {
        QNDEBUG(QStringLiteral("Adding new username ") << linkedNotebook.username()
                << QStringLiteral(" corresponding to linked notebook guid ") << linkedNotebookGuid);
        it = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(linkedNotebookGuid, linkedNotebook.username());
    }

    auto modelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (modelItemIt == m_modelItemsByLinkedNotebookGuid.end()) {
        QNDEBUG(QStringLiteral("Found no model item corresponding to linked notebook guid ") << linkedNotebookGuid);
        return;
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt == m_linkedNotebookItems.end())
    {
        QNWARNING(QStringLiteral("Found linked notebook model item for linked notebook guid ") << linkedNotebookGuid
                  << QStringLiteral(" but no linked notebook item; will try to correct"));
        linkedNotebookItemIt = m_linkedNotebookItems.insert(linkedNotebookGuid, TagLinkedNotebookRootItem(linkedNotebook.username(), linkedNotebookGuid));
    }
    else
    {
        linkedNotebookItemIt->setUsername(linkedNotebook.username());
        QNTRACE(QStringLiteral("Updated the linked notebook username to ")
                << linkedNotebook.username()
                << QStringLiteral(" for linked notebook item corresponding to linked notebook guid ")
                << linkedNotebookGuid);
    }

    QModelIndex linkedNotebookItemIndex = indexForLinkedNotebookGuid(linkedNotebookGuid);
    Q_EMIT dataChanged(linkedNotebookItemIndex, linkedNotebookItemIndex);
}

const TagModelItem * TagModel::itemForId(const IndexId id) const
{
    QNDEBUG(QStringLiteral("TagModel::itemForId: ") << id);

    auto localUidIt = m_indexIdToLocalUidBimap.left.find(id);
    if (localUidIt == m_indexIdToLocalUidBimap.left.end())
    {
        auto linkedNotebookGuidIt = m_indexIdToLinkedNotebookGuidBimap.left.find(id);
        if (linkedNotebookGuidIt == m_indexIdToLinkedNotebookGuidBimap.left.end()) {
            QNDEBUG(QStringLiteral("Found no tag model item corresponding to model index internal id"));
            return Q_NULLPTR;
        }

        const QString & linkedNotebookGuid = linkedNotebookGuidIt->second;
        auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (linkedNotebookModelItemIt == m_modelItemsByLinkedNotebookGuid.end()) {
            QNDEBUG(QStringLiteral("Found no tag linked notebook root model item corresponding to the linked notebook guid "
                                   "corresponding to model index internal id"));
            return Q_NULLPTR;
        }

        return &(linkedNotebookModelItemIt.value());
    }

    const QString & localUid = localUidIt->second;
    QNTRACE(QStringLiteral("Found tag local uid corresponding to model index internal id: ") << localUid);

    auto itemIt = m_modelItemsByLocalUid.find(localUid);
    if (itemIt != m_modelItemsByLocalUid.end()) {
        QNTRACE(QStringLiteral("Found tag model item corresponding to local uid: ") << *itemIt);
        return &(*itemIt);
    }

    QNTRACE(QStringLiteral("Found no tag item corresponding to local uid"));
    return Q_NULLPTR;
}

TagModel::IndexId TagModel::idForItem(const TagModelItem & item) const
{
    if (item.tagItem())
    {
        auto it = m_indexIdToLocalUidBimap.right.find(item.tagItem()->localUid());
        if (it == m_indexIdToLocalUidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLocalUidBimap.insert(IndexIdToLocalUidBimap::value_type(id, item.tagItem()->localUid())))
            return id;
        }

        return it->second;
    }
    else if (item.tagLinkedNotebookItem())
    {
        auto it = m_indexIdToLinkedNotebookGuidBimap.right.find(item.tagLinkedNotebookItem()->linkedNotebookGuid());
        if (it == m_indexIdToLinkedNotebookGuidBimap.right.end()) {
            IndexId id = m_lastFreeIndexId++;
            Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.insert(IndexIdToLinkedNotebookGuidBimap::value_type(id, item.tagLinkedNotebookItem()->linkedNotebookGuid())))
            return id;
        }

        return it->second;
    }

    return 0;
}

QVariant TagModel::dataImpl(const TagModelItem & item, const Columns::type column) const
{
    if ((item.type() == TagModelItem::Type::Tag) && item.tagItem())
    {
        switch(column)
        {
        case Columns::Name:
            return QVariant(item.tagItem()->name());
        case Columns::Synchronizable:
            return QVariant(item.tagItem()->isSynchronizable());
        case Columns::Dirty:
            return QVariant(item.tagItem()->isDirty());
        case Columns::FromLinkedNotebook:
            return QVariant(!item.tagItem()->linkedNotebookGuid().isEmpty());
        case Columns::NumNotesPerTag:
            return QVariant(item.tagItem()->numNotesPerTag());
        default:
            return QVariant();
        }
    }
    else if ((item.type() == TagModelItem::Type::LinkedNotebook) && item.tagLinkedNotebookItem())
    {
        switch(column)
        {
        case Columns::Name:
            return QVariant(item.tagLinkedNotebookItem()->username());
        case Columns::FromLinkedNotebook:
            return QVariant(true);
        default:
            return QVariant();
        }
    }

    return QVariant();
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
    auto it = m_modelItemsByLocalUid.find(localUid);
    if (it == m_modelItemsByLocalUid.end()) {
        return Q_NULLPTR;
    }

    const TagModelItem & item = *it;
    return &item;
}

QModelIndex TagModel::indexForItem(const TagModelItem * pItem) const
{
    if (!pItem) {
        return QModelIndex();
    }

    if (pItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const TagModelItem * pParentItem = pItem->parent();
    if (!pParentItem) {
        pParentItem = m_fakeRootItem;
        pItem->setParent(pParentItem);
    }

    int row = pParentItem->rowForChild(pItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Internal error: can't get the row of the child item in parent in TagModel, child item: ")
                  << *pItem << QStringLiteral("\nParent item: ") << *pParentItem);
        return QModelIndex();
    }

    IndexId itemId = idForItem(*pItem);
    return createIndex(row, Columns::Name, itemId);
}

QModelIndex TagModel::indexForLocalUid(const QString & localUid) const
{
    auto it = m_modelItemsByLocalUid.find(localUid);
    if (it == m_modelItemsByLocalUid.end()) {
        return QModelIndex();
    }

    const TagModelItem & item = *it;
    return indexForItem(&item);
}

QModelIndex TagModel::indexForTagName(const QString & tagName, const QString & linkedNotebookGuid) const
{
    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    auto range = nameIndex.equal_range(tagName.toUpper());
    for(auto it = range.first; it != range.second; ++it)
    {
        const TagItem & item = *it;
        if (item.linkedNotebookGuid() == linkedNotebookGuid) {
            return indexForLocalUid(item.localUid());
        }
    }

    return QModelIndex();
}

QModelIndex TagModel::indexForLinkedNotebookGuid(const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("TagModel::indexForLinkedNotebookGuid: linked notebook guid = ")
            << linkedNotebookGuid);

    auto it = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (it == m_modelItemsByLinkedNotebookGuid.end()) {
        QNDEBUG(QStringLiteral("Found no model item for linked notebook guid ") << linkedNotebookGuid);
        return QModelIndex();
    }

    const TagModelItem & modelItem = it.value();
    return indexForItem(&modelItem);
}

QModelIndex TagModel::promote(const QModelIndex & itemIndex)
{
    QNDEBUG(QStringLiteral("TagModel::promote"));

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote the tag: model index is invalid"));
        return QModelIndex();
    }

    const TagModelItem * pModelItem = itemForIndex(itemIndex);
    if (!pModelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote the tag: found no tag item for given model index"));
        return QModelIndex();
    }

    if (pModelItem == m_fakeRootItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote the invisible root item within the tag model"));
        return QModelIndex();
    }

    if (pModelItem->type() != TagModelItem::Type::Tag) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote non-tag items"));
        return QModelIndex();
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote the tag: internal error, found no tag item under the model item of tag type"));
        return QModelIndex();
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * pParentItem = pModelItem->parent();
    if (!pParentItem)
    {
        QNDEBUG(QStringLiteral("The promoted item has no parent, moving it under fake root item"));
        pParentItem = m_fakeRootItem;
        int row = rowForNewItem(*pParentItem, *pModelItem);
        beginInsertRows(QModelIndex(), row, row);
        pParentItem->insertChild(row, pModelItem);
        endInsertRows();
    }

    if (pParentItem == m_fakeRootItem) {
        REPORT_INFO(QT_TR_NOOP("Can't promote the tag: already a top level item"));
        return QModelIndex();
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (row < 0) {
        QNDEBUG(QStringLiteral("Can't find row of promoted item within its parent item"));
        return QModelIndex();
    }

    const TagModelItem * pGrandParentItem = pParentItem->parent();
    if (!pGrandParentItem)
    {
        QNDEBUG(QStringLiteral("Promoted item's parent has no parent of its own, will move it under the fake root item"));
        pGrandParentItem = m_fakeRootItem;
        int rowInGrandParent = rowForNewItem(*pGrandParentItem, *pParentItem);
        beginInsertRows(QModelIndex(), rowInGrandParent, rowInGrandParent);
        pGrandParentItem->insertChild(rowInGrandParent, pParentItem);
        endInsertRows();
    }

    if ( (pGrandParentItem != m_fakeRootItem) &&
         (!canCreateTagItem(*pGrandParentItem) ||
          (pGrandParentItem->tagItem() && !canUpdateTagItem(*(pGrandParentItem->tagItem())))) )
    {
        REPORT_INFO(QT_TR_NOOP("Can't promote the tag: can't create and/or update tags for the grand parent tag "
                               "due to restrictions"));
        return QModelIndex();
    }

    int parentRow = pGrandParentItem->rowForChild(pParentItem);
    if (Q_UNLIKELY(parentRow < 0)) {
        REPORT_ERROR(QT_TR_NOOP("Can't promote the tag: can't find the parent tag's row within the grand parent model item"));
        return QModelIndex();
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    const TagModelItem * pTakenItem = pParentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != pModelItem))
    {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote the tag, detected internal inconsistency within the tag model: "
                                "the item to take out from its parent doesn't match the original promoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return QModelIndex();
    }

    QModelIndex grandParentIndex = indexForItem(pGrandParentItem);
    int appropriateRow = rowForNewItem(*pGrandParentItem, *pTakenItem);
    beginInsertRows(grandParentIndex, appropriateRow, appropriateRow);
    pGrandParentItem->insertChild(appropriateRow, pTakenItem);
    endInsertRows();

    QModelIndex newIndex = index(appropriateRow, Columns::Name, grandParentIndex);
    if (!newIndex.isValid())
    {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't promote the tag, invalid model index was returned "
                                "for the promoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(grandParentIndex, appropriateRow, appropriateRow);
        Q_UNUSED(pGrandParentItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return QModelIndex();
    }

    TagItem copyTagItem = *(pTakenItem->tagItem());
    if (pGrandParentItem->tagItem()) {
        copyTagItem.setParentLocalUid(pGrandParentItem->tagItem()->localUid());
        copyTagItem.setParentGuid(pGrandParentItem->tagItem()->guid());
    }
    else {
        copyTagItem.setParentLocalUid(QString());
        copyTagItem.setParentGuid(QString());
    }

    bool wasDirty = copyTagItem.isDirty();
    copyTagItem.setDirty(true);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(copyTagItem.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO(QStringLiteral("The promoted tag model item was not found in the underlying item which is odd. Adding it there"));
        Q_UNUSED(localUidIndex.insert(copyTagItem))
    }
    else {
        localUidIndex.replace(it, copyTagItem);
    }

    if (!wasDirty) {
        QModelIndex dirtyColumnIndex = index(appropriateRow, Columns::Dirty, grandParentIndex);
        Q_EMIT dataChanged(dirtyColumnIndex, dirtyColumnIndex);
    }

    updateTagInLocalStorage(copyTagItem);

    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndex TagModel::demote(const QModelIndex & itemIndex)
{
    QNDEBUG(QStringLiteral("TagModel::demote"));

    if (!itemIndex.isValid()) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: model index is invalid"));
        return QModelIndex();
    }

    const TagModelItem * pModelItem = itemForIndex(itemIndex);
    if (!pModelItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: found no tag item for given model index"));
        return QModelIndex();
    }

    if (pModelItem == m_fakeRootItem) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the invisible root item within the tag model"));
        return QModelIndex();
    }

    if (pModelItem->type() != TagModelItem::Type::Tag) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote non-tag items"));
        return QModelIndex();
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: internal error, found no tag item under the model item of tag type"));
        return QModelIndex();
    }

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    const TagModelItem * pParentItem = pModelItem->parent();
    if (!pParentItem)
    {
        QNDEBUG(QStringLiteral("Demoted item has no parent, moving it under the fake root item"));
        pParentItem = m_fakeRootItem;
        int row = rowForNewItem(*pParentItem, *pModelItem);
        beginInsertRows(QModelIndex(), row, row);
        pParentItem->insertChild(row, pModelItem);
        endInsertRows();
    }

    if ((pParentItem != m_fakeRootItem) && pParentItem->tagItem() && !canUpdateTagItem(*(pParentItem->tagItem()))) {
        REPORT_INFO(QT_TR_NOOP("Can't demote the tag: can't update the parent tag due to restrictions"));
        return QModelIndex();
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (row < 0) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: can't find the row of demoted tag within its parent"));
        return QModelIndex();
    }
    else if (row == 0) {
        REPORT_INFO(QT_TR_NOOP("Can't demote the tag: found no preceding sibling within the parent model item "
                               "to demote this tag under"));
        return QModelIndex();
    }

    const TagModelItem * pSiblingItem = pParentItem->childAtRow(row - 1);
    if (Q_UNLIKELY(!pSiblingItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: no sibling tag appropriate for demoting was found"));
        return QModelIndex();
    }

    const TagItem * pSiblingTagItem = pSiblingItem->tagItem();
    if (Q_UNLIKELY(!pSiblingTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: the sibling model item is not of a tag type"));
        return QModelIndex();
    }

    const QString & itemLinkedNotebookGuid = pTagItem->linkedNotebookGuid();
    const QString & siblingItemLinkedNotebookGuid = pSiblingTagItem->linkedNotebookGuid();
    if ((pParentItem == m_fakeRootItem) && (siblingItemLinkedNotebookGuid != itemLinkedNotebookGuid))
    {
        ErrorString error;
        if (itemLinkedNotebookGuid.isEmpty() != siblingItemLinkedNotebookGuid.isEmpty()) {
            error.setBase(QT_TR_NOOP("Can't demote the tag: can't mix tags from linked notebooks "
                                     "with tags from the current account"));
        }
        else {
            error.setBase(QT_TR_NOOP("Can't demote the tag: can't mix tags from different linked notebooks"));
        }

        QNINFO(error << QStringLiteral(", item attempted to be demoted: ") << *pModelItem
               << QStringLiteral("\nSibling item: ") << *pSiblingItem);
        Q_EMIT notifyError(error);
        return QModelIndex();
    }

    if (!canCreateTagItem(*pSiblingItem)) {
        REPORT_INFO(QT_TR_NOOP("Can't demote the tag: can't create tags within the sibling tag"));
        return QModelIndex();
    }

    QModelIndex siblingItemIndex = indexForItem(pSiblingItem);
    if (Q_UNLIKELY(!siblingItemIndex.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Can't demote the tag: can't get the valid model index for the sibling tag"));
        return QModelIndex();
    }

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    const TagModelItem * pTakenItem = pParentItem->takeChild(row);
    endRemoveRows();

    if (Q_UNLIKELY(pTakenItem != pModelItem))
    {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't demote the tag, detected internal inconsistency within the tag model: "
                                "the item to take out from its parent doesn't match the original demoted item"));

        // Reverting the change
        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return QModelIndex();
    }

    int appropriateRow = rowForNewItem(*pSiblingItem, *pTakenItem);
    siblingItemIndex = indexForItem(pSiblingItem);  // Need to update this index since its row within parent might have changed
    beginInsertRows(siblingItemIndex, appropriateRow, appropriateRow);
    pSiblingItem->insertChild(appropriateRow, pTakenItem);
    endInsertRows();

    QModelIndex newIndex = index(appropriateRow, Columns::Name, siblingItemIndex);
    if (!newIndex.isValid())
    {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't demote the tag, invalid model index was returned "
                                "for the demoted tag item"));

        // Trying to revert both done changes
        beginRemoveRows(siblingItemIndex, appropriateRow, appropriateRow);
        Q_UNUSED(pSiblingItem->takeChild(appropriateRow))
        endRemoveRows();

        beginInsertRows(parentIndex, row, row);
        pParentItem->insertChild(row, pTakenItem);
        endInsertRows();

        return QModelIndex();
    }

    TagItem copyTagItem = *(pTakenItem->tagItem());
    if (pSiblingItem->tagItem()) {
        copyTagItem.setParentLocalUid(pSiblingItem->tagItem()->localUid());
        copyTagItem.setParentGuid(pSiblingItem->tagItem()->guid());
    }
    else {
        copyTagItem.setParentLocalUid(QString());
        copyTagItem.setParentGuid(QString());
    }

    bool wasDirty = copyTagItem.isDirty();
    copyTagItem.setDirty(true);

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(copyTagItem.localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        QNINFO(QStringLiteral("The deletemoted tag model item was not found in the underlying item which is odd. Adding it there"));
        Q_UNUSED(localUidIndex.insert(copyTagItem))
    }
    else {
        localUidIndex.replace(it, copyTagItem);
    }

    if (!wasDirty) {
        QModelIndex dirtyColumnIndex = index(appropriateRow, Columns::Dirty, siblingItemIndex);
        Q_EMIT dataChanged(dirtyColumnIndex, dirtyColumnIndex);
    }

    updateTagInLocalStorage(copyTagItem);

    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QModelIndexList TagModel::persistentIndexes() const
{
    return persistentIndexList();
}

QModelIndex TagModel::moveToParent(const QModelIndex & index, const QString & parentTagName)
{
    QNDEBUG(QStringLiteral("TagModel::moveToParent: parent tag name = ") << parentTagName);

    if (Q_UNLIKELY(parentTagName.isEmpty())) {
        return removeFromParent(index);
    }

    const TagModelItem * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: detected attempt to move the tag item to parent "
                                "but the model index has no internal id corresponding to the tag model item"));
        return QModelIndex();
    }

    if (Q_UNLIKELY(pModelItem == m_fakeRootItem)) {
        QNDEBUG(QStringLiteral("Can't move the fake root item to a new parent"));
        return QModelIndex();
    }

    if (pModelItem->type() != TagModelItem::Type::Tag) {
        REPORT_ERROR(QT_TR_NOOP("Can't move non-tag model item to another parent"));
        return QModelIndex();
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: tag model item of tag type has no actual tag item"));
        return QModelIndex();
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto tagItemIt = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(tagItemIt == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the tag item being moved to another parent"));
        return QModelIndex();
    }

    const TagModelItem * pParentItem = pModelItem->parent();
    if (pParentItem && pParentItem->tagItem() && (pParentItem->tagItem()->nameUpper() == parentTagName.toUpper())) {
        QNDEBUG(QStringLiteral("The tag is already at the parent with the correct name, nothing to do"));
        return index;
    }

    TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    auto newParentItemsRange = nameIndex.equal_range(parentTagName.toUpper());
    auto newParentItemIt = nameIndex.end();
    for(auto it = newParentItemsRange.first; it != newParentItemsRange.second; ++it)
    {
        const TagItem & newParentTagItem = *it;
        if (newParentTagItem.linkedNotebookGuid() == pTagItem->linkedNotebookGuid()) {
            newParentItemIt = it;
            break;
        }
    }

    if (Q_UNLIKELY(newParentItemIt == nameIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the parent tag under which the current tag should be moved"));
        return QModelIndex();
    }

    auto newParentModelItemIt = m_modelItemsByLocalUid.find(newParentItemIt->localUid());
    if (Q_UNLIKELY(newParentModelItemIt == m_modelItemsByLocalUid.end())) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: can't find the model item corresponding to the parent tag "
                                "under which the current tag should be moved"));
        return QModelIndex();
    }

    const TagModelItem * pNewParentItem = &(*newParentModelItemIt);
    if (Q_UNLIKELY(pNewParentItem->type() != TagModelItem::Type::Tag)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: the tag model item corresponding to the parent tag "
                                "under which the current tag should be moved has wrong item type"));
        return QModelIndex();
    }

    const TagItem * pNewParentTagItem = pNewParentItem->tagItem();
    if (Q_UNLIKELY(!pNewParentTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: the tag model item corresponding to the parent tag "
                                "under which the current tag should be moved has no tag item"));
        return QModelIndex();
    }

    // If the new parent is actually one of the children of the original item, reject
    const int numMovedItemChildren = pModelItem->numChildren();
    for(int i = 0; i < numMovedItemChildren; ++i)
    {
        const TagModelItem * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING(QStringLiteral("Found null child item at row ") << i);
            continue;
        }

        if (pChildItem == pNewParentItem) {
            ErrorString error(QT_TR_NOOP("Can't set the parent of the tag to one of its child tags"));
            QNINFO(error);
            Q_EMIT notifyError(error);
            return QModelIndex();
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
    QNDEBUG(QStringLiteral("TagModel::removeFromParent"));

    const TagModelItem * pModelItem = itemForId(static_cast<IndexId>(index.internalId()));
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: detected attempt to remove the tag model item from its parent "
                                "but the model index has no internal id corresponding to any tag model item"));
        return QModelIndex();
    }

    if (pModelItem->type() != TagModelItem::Type::Tag) {
        REPORT_ERROR(QT_TR_NOOP("Can only remove tag model items from their parent tags"));
        return QModelIndex();
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Internal error: detected attempt to remove the tag model item from its parent "
                                "but the model item has no tag item even though it is of tag type"));
        return QModelIndex();
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    auto it = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Can't find the the tag to be removed from its parent within the tag model"));
        QNDEBUG("Tag item: " << *pTagItem);
        return QModelIndex();
    }

    removeModelItemFromParent(*pModelItem);

    TagItem tagItemCopy(*pTagItem);
    tagItemCopy.setParentGuid(QString());
    tagItemCopy.setParentLocalUid(QString());
    tagItemCopy.setDirty(true);
    localUidIndex.replace(it, tagItemCopy);

    updateTagInLocalStorage(tagItemCopy);

    if (!m_fakeRootItem) {
        m_fakeRootItem = new TagModelItem;
    }

    QNDEBUG(QStringLiteral("Setting fake root item as the new parent for the tag"));
    int newRow = rowForNewItem(*m_fakeRootItem, *pModelItem);

    beginInsertRows(QModelIndex(), newRow, newRow);
    m_fakeRootItem->insertChild(newRow, pModelItem);
    endInsertRows();

    QModelIndex newIndex = indexForItem(pModelItem);
    Q_EMIT notifyTagParentChanged(newIndex);
    return newIndex;
}

QStringList TagModel::tagNames(const QString & linkedNotebookGuid) const
{
    QNDEBUG(QStringLiteral("TagModel::tagNames: linked notebook guid = ") << linkedNotebookGuid
            << QStringLiteral(" (null = ") << (linkedNotebookGuid.isNull() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", empty = ") << (linkedNotebookGuid.isEmpty() ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(")"));

    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();

    QStringList result;
    result.reserve(static_cast<int>(nameIndex.size()));

    for(auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it)
    {
        if (it->linkedNotebookGuid() != linkedNotebookGuid) {
            continue;
        }

        const QString tagName = it->name();
        result << tagName;
    }

    return result;
}

QModelIndex TagModel::createTag(const QString & tagName, const QString & parentTagName,
                                const QString & linkedNotebookGuid, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("TagModel::createTag: tag name = ") << tagName << QStringLiteral(", parent tag name = ")
            << parentTagName << QStringLiteral(", linked notebook guid = ") << linkedNotebookGuid);

    if (tagName.isEmpty()) {
        errorDescription.setBase(QT_TR_NOOP("Tag name is empty"));
        return QModelIndex();
    }

    int tagNameSize = tagName.size();

    if (tagNameSize < qevercloud::EDAM_TAG_NAME_LEN_MIN) {
        errorDescription.setBase(QT_TR_NOOP("Tag name size is below the minimal acceptable length"));
        errorDescription.details() = QString::number(qevercloud::EDAM_TAG_NAME_LEN_MIN);
        return QModelIndex();
    }

    if (tagNameSize > qevercloud::EDAM_TAG_NAME_LEN_MAX) {
        errorDescription.setBase(QT_TR_NOOP("Tag name size is above the maximal acceptable length"));
        errorDescription.details() = QString::number(qevercloud::EDAM_TAG_NAME_LEN_MAX);
        return QModelIndex();
    }

    QModelIndex existingItemIndex = indexForTagName(tagName, linkedNotebookGuid);
    if (existingItemIndex.isValid()) {
        errorDescription.setBase(QT_TR_NOOP("Tag with such name already exists"));
        return QModelIndex();
    }

    if (!linkedNotebookGuid.isEmpty())
    {
        auto restrictionsIt = m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (restrictionsIt == m_tagRestrictionsByLinkedNotebookGuid.end()) {
            errorDescription.setBase(QT_TR_NOOP("Can't find the tag restrictions for the specified linked notebook"));
            return QModelIndex();
        }

        const Restrictions & restrictions = restrictionsIt.value();
        if (!restrictions.m_canCreateTags) {
            errorDescription.setBase(QT_TR_NOOP("Can't create a new tag as the linked notebook restrictions prohibit the creation of new tags"));
            return QModelIndex();
        }
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
    int numExistingTags = static_cast<int>(localUidIndex.size());
    if (Q_UNLIKELY(numExistingTags + 1 >= m_account.tagCountMax())) {
        errorDescription.setBase(QT_TR_NOOP("Can't create a new tag: the account can contain a limited number of tags"));
        errorDescription.details() = QString::number(m_account.tagCountMax());
        return QModelIndex();
    }

    const TagModelItem * pParentItem = Q_NULLPTR;

    if (!linkedNotebookGuid.isEmpty())
    {
        auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (linkedNotebookModelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
            pParentItem = &(linkedNotebookModelItemIt.value());
        }
    }

    if (!pParentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        pParentItem = m_fakeRootItem;
    }

    if (!parentTagName.isEmpty())
    {
        const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
        auto parentTagRange = nameIndex.equal_range(parentTagName.toUpper());
        auto parentTagIt = nameIndex.end();
        for(auto it = parentTagRange.first; it != parentTagRange.second; ++it)
        {
            if (it->linkedNotebookGuid() == linkedNotebookGuid) {
                parentTagIt = it;
                break;
            }
        }

        if (Q_UNLIKELY(parentTagIt == nameIndex.end())) {
            errorDescription.setBase(QT_TR_NOOP("Can't create a new tag: the parent tag was not found within the model"));
            errorDescription.details() = parentTagName;
            return QModelIndex();
        }

        const TagItem * pParentTagItem = &(*parentTagIt);
        auto parentModelItemIt = m_modelItemsByLocalUid.find(pParentTagItem->localUid());
        if (Q_UNLIKELY(parentModelItemIt == m_modelItemsByLocalUid.end())) {
            errorDescription.setBase(QT_TR_NOOP("Can't create a new tag: can't find the tag model item corresponding to a tag local uid"));
            errorDescription.details() = parentTagName + QStringLiteral(" (") + pParentTagItem->localUid() + QStringLiteral(")");
            return QModelIndex();
        }

        pParentItem = &(*parentModelItemIt);
        QNDEBUG(QStringLiteral("Will put the new tag under parent item: ") << *pParentItem);
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    TagItem item;
    item.setLocalUid(UidGenerator::Generate());
    Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.insert(item.localUid()))

    item.setName(tagName);
    item.setDirty(true);
    item.setSynchronizable(m_account.type() != Account::Type::Local);

    if (pParentItem->tagItem()) {
        item.setParentLocalUid(pParentItem->tagItem()->localUid());
    }

    Q_EMIT aboutToAddTag();

    auto insertionResult = localUidIndex.insert(item);
    const TagItem & insertedItem = *(insertionResult.first);

    TagModelItem modelItem(TagModelItem::Type::Tag, &insertedItem);
    auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);
    const TagModelItem * pModelItem = &(modelItemInsertionResult.value());

    int row = rowForNewItem(*pParentItem, *pModelItem);

    beginInsertRows(parentIndex, row, row);
    pParentItem->insertChild(row, pModelItem);
    endInsertRows();

    updateTagInLocalStorage(item);

    QModelIndex addedTagIndex = indexForLocalUid(item.localUid());
    Q_EMIT addedTag(addedTagIndex);

    return addedTagIndex;
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

bool TagModel::hasSynchronizableChildren(const TagModelItem * pModelItem) const
{
    if (pModelItem->tagLinkedNotebookItem()) {
        return true;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (!pTagItem && pTagItem->isSynchronizable()) {
        return true;
    }

    QList<const TagModelItem*> childItems = pModelItem->children();
    for(auto it = childItems.constBegin(), end = childItems.constEnd(); it != end; ++it) {
        if (hasSynchronizableChildren(*it)) {
            return true;
        }
    }

    return false;
}

void TagModel::mapChildItems()
{
    QNDEBUG(QStringLiteral("TagModel::mapChildItems"));

    for(auto it = m_modelItemsByLocalUid.begin(),
        end = m_modelItemsByLocalUid.end(); it != end; ++it)
    {
        const TagModelItem & item = *it;
        mapChildItems(item);
    }

    for(auto it = m_modelItemsByLinkedNotebookGuid.begin(),
        end = m_modelItemsByLinkedNotebookGuid.end(); it != end; ++it)
    {
        const TagModelItem & item = *it;
        mapChildItems(item);
    }
}

void TagModel::mapChildItems(const TagModelItem & item)
{
    QNTRACE(QStringLiteral("TagModel::mapChildItems: ") << item);

    const TagItem * pTagItem = Q_NULLPTR;
    const TagLinkedNotebookRootItem * pTagLinkedNotebookRootItem = Q_NULLPTR;

    if ((item.type() == TagModelItem::Type::Tag) && item.tagItem()) {
        pTagItem = item.tagItem();
    }
    else if ((item.type() == TagModelItem::Type::LinkedNotebook) && item.tagLinkedNotebookItem()) {
        pTagLinkedNotebookRootItem = item.tagLinkedNotebookItem();
    }

    if (Q_UNLIKELY(!pTagItem && !pTagLinkedNotebookRootItem)) {
        return;
    }

    QModelIndex parentIndex = indexForItem(&item);

    if (pTagItem)
    {
        TagDataByParentLocalUid & parentLocalUidIndex = m_data.get<ByParentLocalUid>();
        auto range = parentLocalUidIndex.equal_range(pTagItem->localUid());
        for(auto it = range.first; it != range.second; ++it)
        {
            const TagItem & currentTagItem = *it;

            const TagModelItem & currentModelItem = modelItemForTagItem(currentTagItem);
            int row = item.rowForChild(&currentModelItem);
            if (row >= 0) {
                continue;
            }

            removeModelItemFromParent(currentModelItem);

            row = rowForNewItem(item, currentModelItem);
            beginInsertRows(parentIndex, row, row);
            item.insertChild(row, &currentModelItem);
            endInsertRows();
        }
    }
    else if (pTagLinkedNotebookRootItem)
    {
        TagDataByLinkedNotebookGuid & linkedNotebookGuidIndex = m_data.get<ByLinkedNotebookGuid>();
        auto range = linkedNotebookGuidIndex.equal_range(pTagLinkedNotebookRootItem->linkedNotebookGuid());
        for(auto it = range.first; it != range.second; ++it)
        {
            const TagItem & currentTagItem = *it;
            if (!currentTagItem.parentLocalUid().isEmpty()) {
                continue;
            }

            const TagModelItem & currentModelItem = modelItemForTagItem(currentTagItem);
            int row = item.rowForChild(&currentModelItem);
            if (row >= 0) {
                continue;
            }

            removeModelItemFromParent(currentModelItem);

            row = rowForNewItem(item, currentModelItem);
            beginInsertRows(parentIndex, row, row);
            item.insertChild(row, &currentModelItem);
            endInsertRows();
        }
    }
}

QString TagModel::nameForNewTag(const QString & linkedNotebookGuid) const
{
    QString baseName = tr("New tag");
    const TagDataByNameUpper & nameIndex = m_data.get<ByNameUpper>();
    QSet<QString> tagNames;
    for(auto it = nameIndex.begin(), end = nameIndex.end(); it != end; ++it)
    {
        if (it->linkedNotebookGuid() != linkedNotebookGuid) {
            continue;
        }

        Q_UNUSED(tagNames.insert(it->nameUpper()))
    }

    int & lastNewTagNameCounter = (linkedNotebookGuid.isEmpty()
                                   ? m_lastNewTagNameCounter
                                   : m_lastNewTagNameCounterByLinkedNotebookGuid[linkedNotebookGuid]);
    return newItemName<QSet<QString> >(tagNames, lastNewTagNameCounter, baseName);
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

    const TagItem & tagItem = *itemIt;

    auto modelItemIt = m_modelItemsByLocalUid.find(tagItem.localUid());
    if (modelItemIt == m_modelItemsByLocalUid.end()) {
        QNDEBUG(QStringLiteral("Found no tag model item corresponding to tag item: ") << tagItem);
        return;
    }

    const TagModelItem * pModelItem = &(modelItemIt.value());
    const TagModelItem * pParentItem = pModelItem->parent();
    if (!pParentItem)
    {
        QNDEBUG(QStringLiteral("The removed item has no parent, will put it under the fake root item"));
        pParentItem = m_fakeRootItem;
        int row = pParentItem->numChildren();
        beginInsertRows(QModelIndex(), row, row);
        pModelItem->setParent(pParentItem);
        endInsertRows();
    }

    int row = pParentItem->rowForChild(pModelItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Internal error: can't get the row of the child item in parent in TagModel, child item: ")
                  << tagItem << QStringLiteral("\nParent item: ") << *pParentItem);
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

        const TagItem & childTagItem = *childIt;
        removeItemByLocalUid(childTagItem.localUid());
    }

    QModelIndex parentItemModelIndex = indexForItem(pParentItem);
    beginRemoveRows(parentItemModelIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();

    auto indexIt = m_indexIdToLocalUidBimap.right.find(itemIt->localUid());
    if (indexIt != m_indexIdToLocalUidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLocalUidBimap.right.erase(indexIt))
    }

    Q_UNUSED(m_modelItemsByLocalUid.erase(modelItemIt))
    Q_UNUSED(localUidIndex.erase(itemIt))

    checkAndRemoveEmptyLinkedNotebookRootItem(*pParentItem);
}

void TagModel::removeModelItemFromParent(const TagModelItem & item)
{
    QNDEBUG(QStringLiteral("TagModel::removeModelItemFromParent: ") << item);

    const TagModelItem * pParentItem = item.parent();
    if (Q_UNLIKELY(!pParentItem)) {
        QNDEBUG(QStringLiteral("No parent item, nothing to do"));
        return;
    }

    QNTRACE(QStringLiteral("Parent item: ") << *pParentItem);
    int row = pParentItem->rowForChild(&item);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING(QStringLiteral("Can't find the child tag item's row within its parent; child item = ")
                  << item << QStringLiteral(", parent item = ") << *pParentItem);
        return;
    }

    QNTRACE(QStringLiteral("Will remove the child at row ") << row);

    QModelIndex parentIndex = indexForItem(pParentItem);
    beginRemoveRows(parentIndex, row, row);
    Q_UNUSED(pParentItem->takeChild(row))
    endRemoveRows();
}

int TagModel::rowForNewItem(const TagModelItem & parentItem, const TagModelItem & newItem) const
{
    QNDEBUG(QStringLiteral("TagModel::rowForNewItem: new item = ") << newItem
            << QStringLiteral(", parent item = ") << parentItem);

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG(QStringLiteral("Won't sort on column ") << m_sortedColumn);
        // Sorting by other columns is not yet implemented
        return parentItem.numChildren();
    }

    QList<const TagModelItem*> children = parentItem.children();
    auto it = children.constEnd();

    if (m_sortOrder == Qt::AscendingOrder) {
        it = std::lower_bound(children.constBegin(), children.constEnd(), &newItem, LessByName());
    }
    else {
        it = std::lower_bound(children.constBegin(), children.constEnd(), &newItem, GreaterByName());
    }

    int row = -1;
    if (it == children.constEnd()) {
        row = parentItem.numChildren();
    }
    else {
        row = static_cast<int>(std::distance(children.constBegin(), it));
    }

    QNDEBUG(QStringLiteral("Appropriate row = ") << row);
    return row;
}

void TagModel::updateItemRowWithRespectToSorting(const TagModelItem & item)
{
    QNDEBUG(QStringLiteral("TagModel::updateItemRowWithRespectToSorting: item = ") << item);

    if (m_sortedColumn != Columns::Name) {
        QNDEBUG(QStringLiteral("Won't sort on column ") << m_sortedColumn);
        // Sorting by other columns is not yet implemented
        return;
    }

    const TagModelItem * pParentItem = item.parent();
    if (!pParentItem)
    {
        if ((item.type() == TagModelItem::Type::Tag) && item.tagItem() && !item.tagItem()->linkedNotebookGuid().isEmpty())
        {
            auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(item.tagItem()->linkedNotebookGuid());
            if (linkedNotebookModelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
                pParentItem = &(linkedNotebookModelItemIt.value());
            }
        }

        if (!pParentItem)
        {
            if (!m_fakeRootItem) {
                m_fakeRootItem = new TagModelItem;
            }

            pParentItem = m_fakeRootItem;
        }

        int row = rowForNewItem(*pParentItem, item);
        beginInsertRows(QModelIndex(), row, row);
        pParentItem->insertChild(row, &item);
        endInsertRows();
        return;
    }

    int currentItemRow = pParentItem->rowForChild(&item);
    if (Q_UNLIKELY(currentItemRow < 0)) {
        QNWARNING(QStringLiteral("Can't update tag model item's row: can't find its original row within parent: ") << item);
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

void TagModel::updateTagInLocalStorage(const TagItem & item)
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
            Q_EMIT findTag(dummy, requestId);
            return;
        }

        tag = *pCachedTag;
    }

    tagFromItem(item, tag);

    QUuid requestId = QUuid::createUuid();

    if (notYetSavedItemIt != m_tagItemsNotYetInLocalStorageUids.end())
    {
        Q_UNUSED(m_addTagRequestIds.insert(requestId));

        QNTRACE(QStringLiteral("Emitting the request to add the tag to local storage: id = ") << requestId
                << QStringLiteral(", tag: ") << tag);
        Q_EMIT addTag(tag, requestId);

        Q_UNUSED(m_tagItemsNotYetInLocalStorageUids.erase(notYetSavedItemIt))
    }
    else
    {
        Q_UNUSED(m_updateTagRequestIds.insert(requestId));

        // While the tag is being updated in the local storage,
        // remove its stale copy from the cache
        Q_UNUSED(m_cache.remove(tag.localUid()))

        QNTRACE(QStringLiteral("Emitting the request to update tag in the local storage: id = ") << requestId
                << QStringLiteral(", tag: ") << tag);
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

void TagModel::setTagFavorited(const QModelIndex & index, const bool favorited)
{
    if (Q_UNLIKELY(!index.isValid())) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the tag: model index is invalid"));
        return;
    }

    const TagModelItem * pModelItem = itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the tag: can't find the model item corresponding to index"));
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the tag: the target model item is not a tag item"));
        return;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the tag: the target model item has no tag item "
                                "even though it is of a tag type"));
        return;
    }

    if (favorited == pTagItem->isFavorited()) {
        QNDEBUG(QStringLiteral("Favorited flag's value hasn't changed"));
        return;
    }

    TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    auto it = localUidIndex.find(pTagItem->localUid());
    if (Q_UNLIKELY(it == localUidIndex.end())) {
        REPORT_ERROR(QT_TR_NOOP("Can't set favorited flag for the tag: the modified tag entry was not found within the model"));
        return;
    }

    TagItem itemCopy(*pTagItem);
    itemCopy.setFavorited(favorited);
    // NOTE: won't mark the tag as dirty as favorited property is not included into the synchronization protocol

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

const TagModelItem & TagModel::findOrCreateLinkedNotebookModelItem(const QString & linkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("TagModel::findOrCreateLinkedNotebookModelItem: ") << linkedNotebookGuid);

    if (Q_UNLIKELY(!m_fakeRootItem)) {
        m_fakeRootItem = new TagModelItem;
    }

    if (Q_UNLIKELY(linkedNotebookGuid.isEmpty())) {
        QNWARNING(QStringLiteral("Detected the request for finding of creation of a linked notebook model item "
                                 "for empty linked notebook guid"));
        return *m_fakeRootItem;
    }

    auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (linkedNotebookModelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
        QNDEBUG(QStringLiteral("Found existing linked notebook model item for linked notebook guid ")
                << linkedNotebookGuid);
        return linkedNotebookModelItemIt.value();
    }

    QNDEBUG(QStringLiteral("Found no existing model item corresponding to linked notebook guid ")
            << linkedNotebookGuid << QStringLiteral(", will create one"));

    const TagLinkedNotebookRootItem * pLinkedNotebookItem = Q_NULLPTR;
    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt == m_linkedNotebookItems.end())
    {
        QNDEBUG(QStringLiteral("Found no existing linked notebook root item, will create one"));

        auto linkedNotebookOwnerUsernameIt = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.find(linkedNotebookGuid);
        if (Q_UNLIKELY(linkedNotebookOwnerUsernameIt == m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.end())) {
            QNDEBUG(QStringLiteral("Found no linked notebook owner's username for linked notebook guid ")
                      << linkedNotebookGuid);
            linkedNotebookOwnerUsernameIt = m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids.insert(linkedNotebookGuid, QString());
        }

        const QString & linkedNotebookOwnerUsername = linkedNotebookOwnerUsernameIt.value();

        TagLinkedNotebookRootItem linkedNotebookItem(linkedNotebookOwnerUsername, linkedNotebookGuid);
        linkedNotebookItemIt = m_linkedNotebookItems.insert(linkedNotebookGuid, linkedNotebookItem);
    }

    pLinkedNotebookItem = &(linkedNotebookItemIt.value());
    QNTRACE(QStringLiteral("Linked notebook root item: ") << *pLinkedNotebookItem);

    linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.insert(linkedNotebookGuid,
                                                                        TagModelItem(TagModelItem::Type::LinkedNotebook,
                                                                                     Q_NULLPTR, pLinkedNotebookItem));
    const TagModelItem * pModelItem = &(linkedNotebookModelItemIt.value());
    int row = rowForNewItem(*m_fakeRootItem, *pModelItem);
    beginInsertRows(QModelIndex(), row, row);
    m_fakeRootItem->insertChild(row, pModelItem);
    endInsertRows();

    return linkedNotebookModelItemIt.value();
}

const TagModelItem & TagModel::modelItemForTagItem(const TagItem & tagItem)
{
    QNTRACE(QStringLiteral("TagModel::modelItemForTagItem: ") << tagItem);

    auto modelItemIt = m_modelItemsByLocalUid.find(tagItem.localUid());
    if (modelItemIt != m_modelItemsByLocalUid.end()) {
        return modelItemIt.value();
    }

    TagModelItem modelItem(TagModelItem::Type::Tag, &tagItem);
    modelItemIt = m_modelItemsByLocalUid.insert(tagItem.localUid(), modelItem);
    const TagModelItem * pModelItem = &(modelItemIt.value());

    const QString & parentLocalUid = tagItem.parentLocalUid();
    if (!parentLocalUid.isEmpty())
    {
        TagDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto parentTagItemIt = localUidIndex.find(parentLocalUid);
        if (parentTagItemIt != localUidIndex.end())
        {
            const TagItem & parentTagItem = *parentTagItemIt;
            const TagModelItem & parentModelItem = modelItemForTagItem(parentTagItem);

            int row = rowForNewItem(parentModelItem, *pModelItem);
            QModelIndex parentIndex = indexForItem(&parentModelItem);
            beginInsertRows(parentIndex, row, row);
            parentModelItem.insertChild(row, pModelItem);
            endInsertRows();

            return *pModelItem;
        }

        // If we got here, the parent tag item is not yet within the model, so will temporarily map
        // to either fake root item or linked notebook root item
    }

    const TagModelItem * pParentItem = Q_NULLPTR;

    const QString & linkedNotebookGuid = tagItem.linkedNotebookGuid();
    if (!linkedNotebookGuid.isEmpty())
    {
        auto linkedNotebookModelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (linkedNotebookModelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
            pParentItem = &(linkedNotebookModelItemIt.value());
        }
    }

    if (!pParentItem)
    {
        if (!m_fakeRootItem) {
            m_fakeRootItem = new TagModelItem;
        }

        pParentItem = m_fakeRootItem;
    }

    QModelIndex parentIndex = indexForItem(pParentItem);

    int row = rowForNewItem(*pParentItem, *pModelItem);
    beginInsertRows(parentIndex, row, row);
    pParentItem->insertChild(row, pModelItem);
    endInsertRows();

    return *pModelItem;
}

void TagModel::checkAndRemoveEmptyLinkedNotebookRootItem(const TagModelItem & modelItem)
{
    if (modelItem.type() != TagModelItem::Type::LinkedNotebook) {
        return;
    }

    if (!modelItem.tagLinkedNotebookItem()) {
        return;
    }

    if (modelItem.hasChildren()) {
        return;
    }

    QNDEBUG(QStringLiteral("Removed the last child from the linked notebook root item, will remove that item as well"));
    removeModelItemFromParent(modelItem);

    QString linkedNotebookGuid = modelItem.tagLinkedNotebookItem()->linkedNotebookGuid();

    auto indexIt = m_indexIdToLinkedNotebookGuidBimap.right.find(linkedNotebookGuid);
    if (indexIt != m_indexIdToLinkedNotebookGuidBimap.right.end()) {
        Q_UNUSED(m_indexIdToLinkedNotebookGuidBimap.right.erase(indexIt))
    }

    auto modelItemIt = m_modelItemsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (modelItemIt != m_modelItemsByLinkedNotebookGuid.end()) {
        Q_UNUSED(m_modelItemsByLinkedNotebookGuid.erase(modelItemIt))
    }

    auto linkedNotebookItemIt = m_linkedNotebookItems.find(linkedNotebookGuid);
    if (linkedNotebookItemIt != m_linkedNotebookItems.end()) {
        Q_UNUSED(m_linkedNotebookItems.erase(linkedNotebookItemIt))
    }
}

void TagModel::checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem)
{
    QNTRACE(QStringLiteral("TagModel::checkAndFindLinkedNotebookRestrictions: ") << tagItem);

    const QString & linkedNotebookGuid = tagItem.linkedNotebookGuid();
    if (linkedNotebookGuid.isEmpty()) {
        QNTRACE(QStringLiteral("No linked notebook guid"));
        return;
    }

    auto restrictionsIt = m_tagRestrictionsByLinkedNotebookGuid.find(linkedNotebookGuid);
    if (restrictionsIt != m_tagRestrictionsByLinkedNotebookGuid.end()) {
        QNTRACE(QStringLiteral("Already have the tag restrictions for linked notebook guid ") << linkedNotebookGuid);
        return;
    }

    auto it = m_findNotebookRequestForLinkedNotebookGuid.left.find(linkedNotebookGuid);
    if (it != m_findNotebookRequestForLinkedNotebookGuid.left.end()) {
        QNTRACE(QStringLiteral("Already emitted the request to find tag restrictions for linked notebook guid ") << linkedNotebookGuid);
        return;
    }

    QUuid requestId = QUuid::createUuid();
    m_findNotebookRequestForLinkedNotebookGuid.insert(LinkedNotebookGuidWithFindNotebookRequestIdBimap::value_type(linkedNotebookGuid, requestId));
    Notebook notebook;
    notebook.unsetLocalUid();
    notebook.setLinkedNotebookGuid(linkedNotebookGuid);
    QNTRACE(QStringLiteral("Emitted the request to find notebook by linked notebook guid: ") << linkedNotebookGuid
            << QStringLiteral(", for the purpose of finding the tag restrictions; request id = ") << requestId);
    Q_EMIT findNotebook(notebook, requestId);
}

#define MODEL_ITEM_NAME(item, itemName) \
    if ((item.type() == TagModelItem::Type::Tag) && item.tagItem()) { \
        itemName = item.tagItem()->nameUpper(); \
    } \
    else if ((item.type() == TagModelItem::Type::LinkedNotebook) && item.tagLinkedNotebookItem()) { \
        itemName = item.tagLinkedNotebookItem()->username().toUpper(); \
    }

bool TagModel::LessByName::operator()(const TagModelItem & lhs, const TagModelItem & rhs) const
{
    // NOTE: treating linked notebook item as the one always going after the non-linked notebook item
    if ((lhs.type() == TagModelItem::Type::LinkedNotebook) &&
        (rhs.type() != TagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((lhs.type() != TagModelItem::Type::LinkedNotebook) &&
             (rhs.type() == TagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
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
    // NOTE: treating linked notebook item as the one always going after the non-linked notebook item
    if ((lhs.type() == TagModelItem::Type::LinkedNotebook) &&
        (rhs.type() != TagModelItem::Type::LinkedNotebook))
    {
        return false;
    }
    else if ((lhs.type() != TagModelItem::Type::LinkedNotebook) &&
             (rhs.type() == TagModelItem::Type::LinkedNotebook))
    {
        return true;
    }

    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
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

TagModel::RemoveRowsScopeGuard::RemoveRowsScopeGuard(TagModel & model) :
    m_model(model)
{
    m_model.beginRemoveTags();
}

TagModel::RemoveRowsScopeGuard::~RemoveRowsScopeGuard()
{
    m_model.endRemoveTags();
}

} // namespace quentier
