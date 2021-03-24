/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_TAG_MODEL_H
#define QUENTIER_LIB_MODEL_TAG_MODEL_H

#include "TagCache.h"

#include "ITagModelItem.h"
#include "TagItem.h"
#include "TagLinkedNotebookRootItem.h"

#include <lib/model/common/AbstractItemModel.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>

#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#define TAG_MODEL_MIME_TYPE                                                    \
    QStringLiteral(                                                            \
        "application/x-com.quentier.tagmodeldatalist") // TAG_MODEL_MIME_TYPE

#define TAG_MODEL_MIME_DATA_MAX_COMPRESSION (9)

namespace quentier {

class TagModel final: public AbstractItemModel
{
    Q_OBJECT
public:
    explicit TagModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync, TagCache & cache,
        QObject * parent = nullptr);

    ~TagModel() override;

    enum class Column
    {
        Name,
        Synchronizable,
        Dirty,
        FromLinkedNotebook,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, Column column);

    [[nodiscard]] ITagModelItem * itemForIndex(const QModelIndex & index) const;
    [[nodiscard]] ITagModelItem * itemForLocalId(const QString & localId) const;

    [[nodiscard]] QModelIndex indexForItem(const ITagModelItem * pItem) const;

    [[nodiscard]] QModelIndex indexForTagName(
        const QString & tagName, const QString & linkedNotebookGuid = {}) const;

    /**
     * @param               The linked notebook guid for which the corresponding
     *                      linked notebook item's index is required
     * @return              The model index of the TagLinkedNotebookRootItem
     *                      corresponding to the given linked notebook guid
     */
    [[nodiscard]] QModelIndex indexForLinkedNotebookGuid(
        const QString & linkedNotebookGuid) const;

    /**
     * @brief promote moves the tag item pointed by the index from its parent
     * to its grandparent, if both exist
     *
     * @param index         The index of the tag item to be promoted
     * @return              The index of the promoted tag item or invalid model
     *                      index if tag item could not be promoted
     */
    [[nodiscard]] QModelIndex promote(const QModelIndex & index);

    /**
     * @brief demote moves the tag item pointed to by the index from its parent
     * to its previous sibling within the current parent
     *
     * @param index         The index of the tag item to be demoted
     * @return              The index of the demoted tag item or invalid model
     *                      index if tag item could not be demoted
     */
    [[nodiscard]] QModelIndex demote(const QModelIndex & index);

    /**
     * @brief moveToParent moves the tag item pointed to by the index under
     * the specified parent tag item
     *
     * @param index         The index of the tag item to be moved under the new
     *                      parent
     * @param parentTagName The name of the parent tag under which the tag item
     *                      is to be moved
     * @return              The index of the moved tag item or invalid model
     *                      index if something went wrong, for example, if
     *                      the passed in index did not really point to the tag
     *                      item
     */
    [[nodiscard]] QModelIndex moveToParent(
        const QModelIndex & index, const QString & parentTagName);

    /**
     * @brief removeFromParent removes the tag item pointed to by the index from
     * its parent tag (if any)
     *
     * @param index         The index of the tag to be removed from its parent
     * @return              The index of the tag item removed from its parent or
     *                      invalid index if something went wrong, for example
     *                      if the passed in index did not really point to
     *                      the tag item
     */
    [[nodiscard]] QModelIndex removeFromParent(const QModelIndex & index);

    /**
     * @brief tagNames
     * @param linkedNotebookGuid    The optional guid of a linked notebook to
     *                              which the returned tag names belong; if it
     *                              is null (i.e. linkedNotebookGuid.isNull()
     *                              returns true), the tag names would be
     *                              returned ignoring their belonging to user's
     *                              own account or linked notebook; if it's not
     *                              null but empty (i.e.
     *                              linkedNotebookGuid.isEmpty() returns true),
     *                              only the names of tags from user's own
     *                              account would be returned. Otherwise only
     *                              the names of tags from the corresponding
     *                              linked notebook would be returned
     * @return                      The sorted (in case insensitive manner) list
     *                              of tag names existing within the tag model
     */
    [[nodiscard]] QStringList tagNames(
        const QString & linkedNotebookGuid = {}) const;

    /**
     * @brief createTag is a convenience method to create a new tag within
     * the model
     *
     * @param tagName               The name of the new tag
     * @param parentTagName         The name of the parent tag for the new tag
     *                              (empty for no parent tag)
     * @param linkedNotebookGuid    The guid of the linked notebook to which
     *                              the created tag should belong; if empty,
     *                              the tag is supposed to belong to user's own
     *                              account
     * @param errorDescription      The textual description of the error if tag
     *                              was not created successfully
     * @return                      Either valid model index if tag was created
     *                              successfully or invalid model index
     *                              otherwise
     */
    [[nodiscard]] QModelIndex createTag(
        const QString & tagName, const QString & parentTagName,
        const QString & linkedNotebookGuid, ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column                The column which name needs to be returned
     * @return the name of the column
     */
    [[nodiscard]] QString columnName(Column column) const;

    /**
     * @brief allTagsListed
     * @return                      True if the tag model has received
     *                              the information about all tags stored
     *                              in the local storage by the moment; false
     *                              otherwise
     */
    [[nodiscard]] bool allTagsListed() const noexcept;

    /**
     * @brief favoriteTag - marks the tag pointed to by the index as favorited
     *
     * Favorited property of Tag class is not represented as a column within
     * the TagModel so this method doesn't change anything in the model but only
     * the underlying tag object persisted in the local storage
     *
     * @param index                 The index of the tag to be favorited
     */
    void favoriteTag(const QModelIndex & index);

    /**
     * @brief unfavoriteTag - removes the favorited mark from the tag pointed
     * to by the index; does nothing if the tag was not favorited prior to
     * the call
     *
     * Favorited property of Tag class is not represented as a column within
     * the TagModel so this method doesn't change anything in the model but only
     * the underlying tag object persisted in the local storage
     *
     * @param index                 The index of the tag to be unfavorited
     */
    void unfavoriteTag(const QModelIndex & index);

    /**
     * @brief tagHasSynchronizedChildTags - checks whether the tag with
     * specified local id contains child tags with non-empty guids
     * i.e. tags which have already been synchronized with Evernote
     *
     * @param tagLocalId           The local id of the tag which is being
     *                              checked for having synchronized child tags
     * @return                      True if the specified tag has synchronized
     *                              child tags, false otherwise
     */
    [[nodiscard]] bool tagHasSynchronizedChildTags(
        const QString & tagLocalId) const noexcept;

public:
    // AbstractItemModel interface
    [[nodiscard]] QString localIdForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QModelIndex indexForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QString itemNameForLocalId(
        const QString & localId) const override;

    [[nodiscard]] ItemInfo itemInfoForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QVector<LinkedNotebookInfo> linkedNotebooksInfo() const
        override;

    [[nodiscard]] QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] int nameColumn() const noexcept override
    {
        return static_cast<int>(Column::Name);
    }

    [[nodiscard]] int sortingColumn() const noexcept override
    {
        return static_cast<int>(m_sortedColumn);
    }

    [[nodiscard]] Qt::SortOrder sortOrder() const noexcept override
    {
        return m_sortOrder;
    }

    [[nodiscard]] bool allItemsListed() const noexcept override;

    [[nodiscard]] QModelIndex allItemsRootItemIndex() const override;

    [[nodiscard]] QString localIdForItemIndex(
        const QModelIndex & index) const override;

    [[nodiscard]] QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override;

public:
    // QAbstractItemModel interface
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex & index) const override;

    [[nodiscard]] QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    [[nodiscard]] int rowCount(const QModelIndex & parent = {}) const override;

    [[nodiscard]] int columnCount(
        const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex index(
        int row, int column, const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex parent(const QModelIndex & index) const override;

    [[nodiscard]] bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool insertRows(
        int row, int count, const QModelIndex & parent = {}) override;

    [[nodiscard]] bool removeRows(
        int row, int count, const QModelIndex & parent = {}) override;

    void sort(int column, Qt::SortOrder order) override;

    // Drag-n-drop interfaces
    [[nodiscard]] Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    [[nodiscard]] Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    [[nodiscard]] QStringList mimeTypes() const override;

    [[nodiscard]] QMimeData * mimeData(
        const QModelIndexList & indexes) const override;

    [[nodiscard]] bool dropMimeData(
        const QMimeData * data, Qt::DropAction action, int row, int column,
        const QModelIndex & parent) override;

Q_SIGNALS:
    void sortingChanged();
    void notifyError(ErrorString errorDescription);
    void notifyAllTagsListed();

    void notifyTagParentChanged(const QModelIndex & tagIndex);

    // Informative signals for views, so that they can prepare to the changes
    // in the tree of tags and do some recovery after that
    void aboutToAddTag();
    void addedTag(const QModelIndex & tagIndex);

    void aboutToUpdateTag(const QModelIndex & tagIndex);
    void updatedTag(const QModelIndex & tagIndex);

    void aboutToRemoveTags();
    void removedTags();

    // private signals
    void addTag(qevercloud::Tag tag, QUuid requestId);
    void updateTag(qevercloud::Tag tag, QUuid requestId);
    void findTag(qevercloud::Tag tag, QUuid requestId);

    void listTags(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void expungeTag(qevercloud::Tag tag, QUuid requestId);
    void findNotebook(qevercloud::Notebook notebook, QUuid requestId);

    void requestNoteCountPerTag(
        qevercloud::Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void requestNoteCountsForAllTags(
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void listAllTagsPerNote(
        qevercloud::Note note, LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void listAllLinkedNotebooks(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onAddTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Tag> tags,
        QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

    void onExpungeTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountPerTagComplete(
        int noteCount, qevercloud::Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerTagFailed(
        ErrorString errorDescription, qevercloud::Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountsPerAllTagsComplete(
        QHash<QString, int> noteCountsPerTagLocalId,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountsPerAllTagsFailed(
        ErrorString errorDescription,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId);

    void onFindNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onAddNoteComplete(
        qevercloud::Note note, QUuid requestId);

    void onNoteTagListChanged(
        QString noteLocalId, QStringList previousNoteTagLocalIds,
        QStringList newNoteTagLocalIds);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onUpdateLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onExpungeLinkedNotebookComplete(
        qevercloud::LinkedNotebook linkedNotebook, QUuid requestId);

    void onListAllTagsPerNoteComplete(
        QList<qevercloud::Tag> foundTags, qevercloud::Note note,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void onListAllTagsPerNoteFailed(
        qevercloud::Note note, LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListAllLinkedNotebooksComplete(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<qevercloud::LinkedNotebook> foundLinkedNotebooks,
        QUuid requestId);

    void onListAllLinkedNotebooksFailed(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestTagsList();
    void requestNoteCountForTag(const qevercloud::Tag & tag);
    void requestTagsPerNote(const qevercloud::Note & note);
    void requestNoteCountsPerAllTags();
    void requestLinkedNotebooksList();

    [[nodiscard]] QVariant dataImpl(
        const ITagModelItem & item, Column column) const;

    [[nodiscard]] QVariant dataAccessibleText(
        const ITagModelItem & item, Column column) const;

    [[nodiscard]] bool hasSynchronizableChildren(
        const ITagModelItem * item) const;

    void mapChildItems();
    void mapChildItems(ITagModelItem & item);

    [[nodiscard]] QString nameForNewTag(
        const QString & linkedNotebookGuid) const;

    void removeItemByLocalId(const QString & localId);

    void removeModelItemFromParent(ITagModelItem & item);

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    [[nodiscard]] int rowForNewItem(
        const ITagModelItem & parentItem, const ITagModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(ITagModelItem & item);
    void updatePersistentModelIndices();
    void updateTagInLocalStorage(const TagItem & item);

    void tagFromItem(const TagItem & item, qevercloud::Tag & tag) const;

    void setNoteCountForTag(const QString & tagLocalId, int noteCount);
    void setTagFavorited(const QModelIndex & index, bool favorited);

    void beginRemoveTags();
    void endRemoveTags();

    [[nodiscard]] ITagModelItem & findOrCreateLinkedNotebookModelItem(
        const QString & linkedNotebookGuid);

    void checkAndRemoveEmptyLinkedNotebookRootItem(ITagModelItem & modelItem);

    void checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem);

    [[nodiscard]] bool tagItemMatchesByLinkedNotebook(
        const TagItem & item, const QString & linkedNotebookGuid) const;

    void fixupItemParent(ITagModelItem & item);
    void setItemParent(ITagModelItem & item, ITagModelItem & parent);

private:
    struct ByLocalId
    {};

    struct ByParentLocalId
    {};

    struct ByNameUpper
    {};

    struct ByLinkedNotebookGuid
    {};

    using TagData = boost::multi_index_container<
        TagItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalId>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::localId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByParentLocalId>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::parentLocalId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    TagItem, QString, &TagItem::nameUpper>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByLinkedNotebookGuid>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::linkedNotebookGuid>>>>;

    using TagDataByLocalId = TagData::index<ByLocalId>::type;
    using TagDataByParentLocalId = TagData::index<ByParentLocalId>::type;
    using TagDataByNameUpper = TagData::index<ByNameUpper>::type;

    using TagDataByLinkedNotebookGuid =
        TagData::index<ByLinkedNotebookGuid>::type;

    using IndexId = quintptr;

    using IndexIdToLocalIdBimap = boost::bimap<IndexId, QString>;
    using IndexIdToLinkedNotebookGuidBimap = boost::bimap<IndexId, QString>;

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem & lhs,
            const ITagModelItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const ITagModelItem * pLhs,
            const ITagModelItem * pRhs) const noexcept;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const ITagModelItem & lhs,
            const ITagModelItem & rhs) const noexcept;

        [[nodiscard]] bool operator()(
            const ITagModelItem * pLhs,
            const ITagModelItem * pRhs) const noexcept;
    };

    using LinkedNotebookItems = QMap<QString, TagLinkedNotebookRootItem>;

    class RemoveRowsScopeGuard
    {
    public:
        RemoveRowsScopeGuard(TagModel & model);
        ~RemoveRowsScopeGuard();

    private:
        TagModel & m_model;
    };

    friend class RemoveRowsScopeGuard;

private:
    void onTagAddedOrUpdated(
        const qevercloud::Tag & tag,
        const QStringList * pTagNoteLocalIds = nullptr);

    void onTagAdded(
        const qevercloud::Tag & tag, const QStringList * pTagNoteLocalIds);

    void onTagUpdated(
        const qevercloud::Tag & tag, TagDataByLocalId::iterator it,
        const QStringList * pTagNoteLocalIds);

    void tagToItem(const qevercloud::Tag & tag, TagItem & item);
    [[nodiscard]] bool canUpdateTagItem(const TagItem & item) const;
    [[nodiscard]] bool canCreateTagItem(const ITagModelItem & parentItem) const;
    void updateRestrictionsFromNotebook(const qevercloud::Notebook & notebook);

    void onLinkedNotebookAddedOrUpdated(
        const qevercloud::LinkedNotebook & linkedNotebook);

    [[nodiscard]] ITagModelItem * itemForId(IndexId id) const;
    [[nodiscard]] IndexId idForItem(const ITagModelItem & item) const;

    void checkAndCreateModelRootItems();

private:
    TagData m_data;

    ITagModelItem * m_pInvisibleRootItem = nullptr;

    ITagModelItem * m_pAllTagsRootItem = nullptr;
    IndexId m_allTagsRootItemIndexId = 1;

    TagCache & m_cache;

    LinkedNotebookItems m_linkedNotebookItems;

    mutable IndexIdToLocalIdBimap m_indexIdToLocalIdBimap;
    mutable IndexIdToLinkedNotebookGuidBimap m_indexIdToLinkedNotebookGuidBimap;
    mutable IndexId m_lastFreeIndexId = 2;

    size_t m_listTagsOffset = 0;
    QUuid m_listTagsRequestId;
    QSet<QUuid> m_tagItemsNotYetInLocalStorageUids;

    QSet<QUuid> m_addTagRequestIds;
    QSet<QUuid> m_updateTagRequestIds;
    QSet<QUuid> m_expungeTagRequestIds;

    QSet<QUuid> m_noteCountPerTagRequestIds;
    QUuid m_noteCountsPerAllTagsRequestId;

    QSet<QUuid> m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findTagToPerformUpdateRequestIds;
    QSet<QUuid> m_findTagAfterNotelessTagsErasureRequestIds;

    QSet<QUuid> m_listTagsPerNoteRequestIds;

    QHash<QString, QString> m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids;
    size_t m_listLinkedNotebooksOffset = 0;
    QUuid m_listLinkedNotebooksRequestId;

    Column m_sortedColumn = Column::Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    struct Restrictions
    {
        bool m_canCreateTags = false;
        bool m_canUpdateTags = false;
    };

    QHash<QString, Restrictions> m_tagRestrictionsByLinkedNotebookGuid;

    using LinkedNotebookGuidWithFindNotebookRequestIdBimap =
        boost::bimap<QString, QUuid>;

    LinkedNotebookGuidWithFindNotebookRequestIdBimap
        m_findNotebookRequestForLinkedNotebookGuid;

    mutable int m_lastNewTagNameCounter = 0;
    mutable QMap<QString, int> m_lastNewTagNameCounterByLinkedNotebookGuid;

    bool m_allTagsListed = false;
    bool m_allLinkedNotebooksListed = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_MODEL_H
