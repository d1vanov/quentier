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

#ifndef QUENTIER_LIB_MODEL_TAG_MODEL_H
#define QUENTIER_LIB_MODEL_TAG_MODEL_H

#include "../ItemModel.h"
#include "TagCache.h"

#include "ITagModelItem.h"
#include "TagItem.h"
#include "TagLinkedNotebookRootItem.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Tag.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

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

class TagModel : public ItemModel
{
    Q_OBJECT
public:
    explicit TagModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync, TagCache & cache,
        QObject * parent = nullptr);

    virtual ~TagModel() override;

    enum class Column
    {
        Name = 0,
        Synchronizable,
        Dirty,
        FromLinkedNotebook,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, const Column column);

    ITagModelItem * itemForIndex(const QModelIndex & index) const;
    ITagModelItem * itemForLocalUid(const QString & localUid) const;

    QModelIndex indexForItem(const ITagModelItem * item) const;

    QModelIndex indexForTagName(
        const QString & tagName, const QString & linkedNotebookGuid = {}) const;

    /**
     * @param               The linked notebook guid for which the corresponding
     *                      linked notebook item's index is required
     * @return              The model index of the TagLinkedNotebookRootItem
     *                      corresponding to the given linked notebook guid
     */
    QModelIndex indexForLinkedNotebookGuid(
        const QString & linkedNotebookGuid) const;

    /**
     * @brief promote moves the tag item pointed by the index from its parent
     * to its grandparent, if both exist
     *
     * @param index         The index of the tag item to be promoted
     * @return              The index of the promoted tag item or invalid model
     *                      index if tag item could not be promoted
     */
    QModelIndex promote(const QModelIndex & index);

    /**
     * @brief demote moves the tag item pointed to by the index from its parent
     * to its previous sibling within the current parent
     *
     * @param index         The index of the tag item to be demoted
     * @return              The index of the demoted tag item or invalid model
     *                      index if tag item could not be demoted
     */
    QModelIndex demote(const QModelIndex & index);

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
    QModelIndex moveToParent(
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
    QModelIndex removeFromParent(const QModelIndex & index);

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
    QStringList tagNames(const QString & linkedNotebookGuid = {}) const;

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
    QModelIndex createTag(
        const QString & tagName, const QString & parentTagName,
        const QString & linkedNotebookGuid, ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column                The column which name needs to be returned
     * @return the name of the column
     */
    QString columnName(const Column column) const;

    /**
     * @brief allTagsListed
     * @return                      True if the tag model has received
     *                              the information about all tags stored
     *                              in the local storage by the moment; false
     *                              otherwise
     */
    bool allTagsListed() const;

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
     * specified local uid contains child tags with non-empty guids
     * i.e. tags which have already been synchronized with Evernote
     *
     * @param tagLocalUid           The local uid of the tag which is being
     *                              checked for having synchronized child tags
     * @return                      True if the specified tag has synchronized
     *                              child tags, false otherwise
     */
    bool tagHasSynchronizedChildTags(const QString & tagLocalUid) const;

public:
    // ItemModel interface
    virtual QString localUidForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    virtual QModelIndex indexForLocalUid(
        const QString & localUid) const override;

    virtual QString itemNameForLocalUid(
        const QString & localUid) const override;

    virtual ItemInfo itemInfoForLocalUid(
        const QString & localUid) const override;

    virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    virtual QVector<LinkedNotebookInfo> linkedNotebooksInfo() const override;

    virtual QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override;

    virtual int nameColumn() const override
    {
        return static_cast<int>(Column::Name);
    }

    virtual int sortingColumn() const override
    {
        return static_cast<int>(m_sortedColumn);
    }

    virtual Qt::SortOrder sortOrder() const override
    {
        return m_sortOrder;
    }

    virtual bool allItemsListed() const override;

    virtual QModelIndex allItemsRootItemIndex() const override;

    virtual QString localUidForItemIndex(
        const QModelIndex & index) const override;

    virtual QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override;

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    virtual QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    virtual QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    virtual int rowCount(const QModelIndex & parent = {}) const override;

    virtual int columnCount(const QModelIndex & parent = {}) const override;

    virtual QModelIndex index(
        int row, int column, const QModelIndex & parent = {}) const override;

    virtual QModelIndex parent(const QModelIndex & index) const override;

    virtual bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool insertRows(
        int row, int count, const QModelIndex & parent = {}) override;

    virtual bool removeRows(
        int row, int count, const QModelIndex & parent = {}) override;

    virtual void sort(int column, Qt::SortOrder order) override;

    // Drag-n-drop interfaces
    virtual Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    virtual Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    virtual QStringList mimeTypes() const override;

    virtual QMimeData * mimeData(
        const QModelIndexList & indexes) const override;

    virtual bool dropMimeData(
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
    void addTag(Tag tag, QUuid requestId);
    void updateTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

    void listTags(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void expungeTag(Tag tag, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void requestNoteCountPerTag(
        Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void requestNoteCountsForAllTags(
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void listAllTagsPerNote(
        Note note, LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void listAllLinkedNotebooks(
        const size_t limit, const size_t offset,
        const LocalStorageManager::ListLinkedNotebooksOrder order,
        const LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddTagComplete(Tag tag, QUuid requestId);

    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onUpdateTagComplete(Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);

    void onFindTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Tag> tags, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeTagComplete(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    void onExpungeTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountPerTagComplete(
        int noteCount, Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerTagFailed(
        ErrorString errorDescription, Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountsPerAllTagsComplete(
        QHash<QString, int> noteCountsPerTagLocalUid,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountsPerAllTagsFailed(
        ErrorString errorDescription,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);

    void onNoteTagListChanged(
        QString noteLocalUid, QStringList previousNoteTagLocalUids,
        QStringList newNoteTagLocalUids);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onAddLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onUpdateLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onExpungeLinkedNotebookComplete(
        LinkedNotebook linkedNotebook, QUuid requestId);

    void onListAllTagsPerNoteComplete(
        QList<Tag> foundTags, Note note,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void onListAllTagsPerNoteFailed(
        Note note, LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListAllLinkedNotebooksComplete(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<LinkedNotebook> foundLinkedNotebooks, QUuid requestId);

    void onListAllLinkedNotebooksFailed(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestTagsList();
    void requestNoteCountForTag(const Tag & tag);
    void requestTagsPerNote(const Note & note);
    void requestNoteCountsPerAllTags();
    void requestLinkedNotebooksList();

    QVariant dataImpl(const ITagModelItem & item, const Column column) const;

    QVariant dataAccessibleText(
        const ITagModelItem & item, const Column column) const;

    bool hasSynchronizableChildren(const ITagModelItem * item) const;

    void mapChildItems();
    void mapChildItems(ITagModelItem & item);

    QString nameForNewTag(const QString & linkedNotebookGuid) const;
    void removeItemByLocalUid(const QString & localUid);

    void removeModelItemFromParent(ITagModelItem & item);

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    int rowForNewItem(
        const ITagModelItem & parentItem, const ITagModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(ITagModelItem & item);
    void updatePersistentModelIndices();
    void updateTagInLocalStorage(const TagItem & item);

    void tagFromItem(const TagItem & item, Tag & tag) const;

    void setNoteCountForTag(const QString & tagLocalUid, const int noteCount);
    void setTagFavorited(const QModelIndex & index, const bool favorited);

    void beginRemoveTags();
    void endRemoveTags();

    ITagModelItem & findOrCreateLinkedNotebookModelItem(
        const QString & linkedNotebookGuid);

    void checkAndRemoveEmptyLinkedNotebookRootItem(ITagModelItem & modelItem);

    void checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem);

    bool tagItemMatchesByLinkedNotebook(
        const TagItem & item, const QString & linkedNotebookGuid) const;

    void fixupItemParent(ITagModelItem & item);
    void setItemParent(ITagModelItem & item, ITagModelItem & parent);

private:
    struct ByLocalUid
    {};
    struct ByParentLocalUid
    {};
    struct ByNameUpper
    {};
    struct ByLinkedNotebookGuid
    {};

    using TagData = boost::multi_index_container<
        TagItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::localUid>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByParentLocalUid>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::parentLocalUid>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    TagItem, QString, &TagItem::nameUpper>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByLinkedNotebookGuid>,
                boost::multi_index::const_mem_fun<
                    TagItem, const QString &, &TagItem::linkedNotebookGuid>>>>;

    using TagDataByLocalUid = TagData::index<ByLocalUid>::type;
    using TagDataByParentLocalUid = TagData::index<ByParentLocalUid>::type;
    using TagDataByNameUpper = TagData::index<ByNameUpper>::type;

    using TagDataByLinkedNotebookGuid =
        TagData::index<ByLinkedNotebookGuid>::type;

    using IndexId = quintptr;

    using IndexIdToLocalUidBimap = boost::bimap<IndexId, QString>;
    using IndexIdToLinkedNotebookGuidBimap = boost::bimap<IndexId, QString>;

    struct LessByName
    {
        bool operator()(
            const ITagModelItem & lhs, const ITagModelItem & rhs) const;

        bool operator()(
            const ITagModelItem * pLhs, const ITagModelItem * pRhs) const;
    };

    struct GreaterByName
    {
        bool operator()(
            const ITagModelItem & lhs, const ITagModelItem & rhs) const;

        bool operator()(
            const ITagModelItem * pLhs, const ITagModelItem * pRhs) const;
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
        const Tag & tag, const QStringList * pTagNoteLocalUids = nullptr);

    void onTagAdded(const Tag & tag, const QStringList * pTagNoteLocalUids);

    void onTagUpdated(
        const Tag & tag, TagDataByLocalUid::iterator it,
        const QStringList * pTagNoteLocalUids);

    void tagToItem(const Tag & tag, TagItem & item);
    bool canUpdateTagItem(const TagItem & item) const;
    bool canCreateTagItem(const ITagModelItem & parentItem) const;
    void updateRestrictionsFromNotebook(const Notebook & notebook);

    void onLinkedNotebookAddedOrUpdated(const LinkedNotebook & linkedNotebook);

    ITagModelItem * itemForId(const IndexId id) const;
    IndexId idForItem(const ITagModelItem & item) const;

    void checkAndCreateModelRootItems();

private:
    TagData m_data;

    ITagModelItem * m_pInvisibleRootItem = nullptr;

    ITagModelItem * m_pAllTagsRootItem = nullptr;
    IndexId m_allTagsRootItemIndexId = 1;

    TagCache & m_cache;

    LinkedNotebookItems m_linkedNotebookItems;

    mutable IndexIdToLocalUidBimap m_indexIdToLocalUidBimap;
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
