/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "ItemModel.h"
#include "TagModelItem.h"
#include "TagCache.h"

#include <quentier/types/Tag.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/LRUCache.hpp>

#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QHash>
#include <QStringList>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>

#define TAG_MODEL_MIME_TYPE                                                    \
    QStringLiteral("application/x-com.quentier.tagmodeldatalist")              \
// TAG_MODEL_MIME_TYPE

#define TAG_MODEL_MIME_DATA_MAX_COMPRESSION (9)

namespace quentier {

class TagModel: public ItemModel
{
    Q_OBJECT
public:
    explicit TagModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        TagCache & cache, QObject * parent = nullptr);

    virtual ~TagModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    struct Columns
    {
        enum type {
            Name = 0,
            Synchronizable,
            Dirty,
            FromLinkedNotebook,
            NumNotesPerTag
        };
    };

    const TagModelItem * itemForIndex(const QModelIndex & index) const;
    const TagModelItem * itemForLocalUid(const QString & localUid) const;
    QModelIndex indexForItem(const TagModelItem * item) const;
    QModelIndex indexForLocalUid(const QString & localUid) const;

    QModelIndex indexForTagName(
        const QString & tagName,
        const QString & linkedNotebookGuid = QString()) const;

    /**
     * @param               The linked notebook guid for which the corresponding
     *                      linked notebook item's index is required
     * @return              The model index of the TagLinkedNotebookRootItem
     *                      corresponding to the given linked notebook guid
     */
    QModelIndex indexForLinkedNotebookGuid(const QString & linkedNotebookGuid) const;

    /**
     * @brief promote - moves the tag item pointed by the index from its parent
     * to its grandparent, if both exist
     *
     * @param index         The index of the tag item to be promoted
     * @return              The index of the promoted tag item or invalid model
     *                      index if tag item could not be promoted
     */
    QModelIndex promote(const QModelIndex & index);

    /**
     * @brief demote - moves the tag item pointed to by the index from its parent
     * to its previous sibling within the current parent
     *
     * @param index         The index of the tag item to be demoted
     * @return              The index of the demoted tag item or invalid model
     *                      index if tag item could not be demoted
     */
    QModelIndex demote(const QModelIndex & index);

    /**
     * @return the list of indexes stored as persistent indexes in the model
     */
    QModelIndexList persistentIndexes() const;

    /**
     * @brief moveToParent - moves the tag item pointed to by the index under
     * the specified parent tag item
     *
     * @param index         The index of the tag item to be moved under the new
     *                      parent
     * @param parentTagName The name of the parent tag under which the tag item
     *                      is to be moved
     * @return              The index of the moved tag item or invalid model
     *                      index if something went wrong, for example, if
     *                      the passed in index did not really point to the tag item
     */
    QModelIndex moveToParent(
        const QModelIndex & index, const QString & parentTagName);

    /**
     * @brief removeFromParent - removes the tag item pointed to by the index
     * from its parent tag (if any)
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
     *                              returns true), the tag names would be returned
     *                              ignoring their belonging to user's own account
     *                              or linked notebook; if it's not null but empty
     *                              (i.e. linkedNotebookGuid.isEmpty() returns
     *                              true), only the names of tags from user's
     *                              own account would be returned. Otherwise only
     *                              the names of tags from the corresponding
     *                              linked notebook would be returned
     * @return                      The sorted (in case insensitive manner) list
     *                              of tag names existing within the tag model
     */
    QStringList tagNames(const QString & linkedNotebookGuid = QString()) const;

    /**
     * @brief createTag - convenience method to create a new tag within the model
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
     *                              successfully or invalid model index otherwise
     */
    QModelIndex createTag(
        const QString & tagName, const QString & parentTagName,
        const QString & linkedNotebookGuid, ErrorString & errorDescription);

    /**
     * @brief columnName
     * @param column                The column which name needs to be returned
     * @return the name of the column
     */
    QString columnName(const Columns::type column) const;

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

    virtual QString itemNameForLocalUid(
        const QString & localUid) const override;

    virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    virtual int nameColumn() const override { return Columns::Name; }
    virtual int sortingColumn() const override { return m_sortedColumn; }
    virtual Qt::SortOrder sortOrder() const override { return m_sortOrder; }
    virtual bool allItemsListed() const override;

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    virtual QVariant data(
        const QModelIndex & index,
        int role = Qt::DisplayRole) const override;

    virtual QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    virtual int rowCount(
        const QModelIndex & parent = QModelIndex()) const override;

    virtual int columnCount(
        const QModelIndex & parent = QModelIndex()) const override;

    virtual QModelIndex index(
        int row, int column,
        const QModelIndex & parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex & index) const override;

    virtual bool setHeaderData(
        int section, Qt::Orientation orientation,
        const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    virtual bool insertRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    virtual bool removeRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    virtual void sort(int column, Qt::SortOrder order) override;

    // Drag-n-drop interfaces
    virtual Qt::DropActions supportedDragActions() const override
    { return Qt::MoveAction; }

    virtual Qt::DropActions supportedDropActions() const override
    { return Qt::MoveAction; }

    virtual QStringList mimeTypes() const override;

    virtual QMimeData * mimeData(
        const QModelIndexList & indexes) const override;

    virtual bool dropMimeData(
        const QMimeData * data, Qt::DropAction action,
        int row, int column, const QModelIndex & parent) override;

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
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void expungeTag(Tag tag, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void requestNoteCountPerTag(
        Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void requestNoteCountsForAllTags(
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void listAllTagsPerNote(
        Note note, LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QUuid requestId);

    void listAllLinkedNotebooks(
        const size_t limit, const size_t offset,
        const LocalStorageManager::ListLinkedNotebooksOrder::type order,
        const LocalStorageManager::OrderDirection::type orderDirection,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QString linkedNotebookGuid, QList<Tag> tags, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
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
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QUuid requestId);

    void onListAllTagsPerNoteFailed(
        Note note, LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListAllLinkedNotebooksComplete(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QList<LinkedNotebook> foundLinkedNotebooks,
        QUuid requestId);

    void onListAllLinkedNotebooksFailed(
        size_t limit, size_t offset,
        LocalStorageManager::ListLinkedNotebooksOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestTagsList();
    void requestNoteCountForTag(const Tag & tag);
    void requestTagsPerNote(const Note & note);
    void requestNoteCountsPerAllTags();
    void requestLinkedNotebooksList();

    QVariant dataImpl(
        const TagModelItem & item, const Columns::type column) const;

    QVariant dataAccessibleText(
        const TagModelItem & item, const Columns::type column) const;

    bool hasSynchronizableChildren(const TagModelItem * item) const;

    void mapChildItems();
    void mapChildItems(const TagModelItem & item);

    QString nameForNewTag(const QString & linkedNotebookGuid) const;
    void removeItemByLocalUid(const QString & localUid);

    void removeModelItemFromParent(const TagModelItem & item);

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    int rowForNewItem(
        const TagModelItem & parentItem, const TagModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(const TagModelItem & item);
    void updatePersistentModelIndices();
    void updateTagInLocalStorage(const TagItem & item);

    void tagFromItem(const TagItem & item, Tag & tag) const;

    void setNoteCountForTag(const QString & tagLocalUid, const int noteCount);
    void setTagFavorited(const QModelIndex & index, const bool favorited);

    void beginRemoveTags();
    void endRemoveTags();

    const TagModelItem & findOrCreateLinkedNotebookModelItem(
        const QString & linkedNotebookGuid);

    const TagModelItem & modelItemForTagItem(const TagItem & tagItem);

    void checkAndRemoveEmptyLinkedNotebookRootItem(
        const TagModelItem & modelItem);

    void checkAndFindLinkedNotebookRestrictions(const TagItem & tagItem);

private:
    struct ByLocalUid{};
    struct ByParentLocalUid{};
    struct ByNameUpper{};
    struct ByLinkedNotebookGuid{};

    typedef boost::multi_index_container<
        TagItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<
                    TagItem,const QString&,&TagItem::localUid>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByParentLocalUid>,
                boost::multi_index::const_mem_fun<
                    TagItem,const QString&,&TagItem::parentLocalUid>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    TagItem,QString,&TagItem::nameUpper>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByLinkedNotebookGuid>,
                boost::multi_index::const_mem_fun<
                    TagItem,const QString&,&TagItem::linkedNotebookGuid>
            >
        >
    > TagData;

    typedef TagData::index<ByLocalUid>::type TagDataByLocalUid;
    typedef TagData::index<ByParentLocalUid>::type TagDataByParentLocalUid;
    typedef TagData::index<ByNameUpper>::type TagDataByNameUpper;
    typedef TagData::index<ByLinkedNotebookGuid>::type TagDataByLinkedNotebookGuid;

    typedef quintptr IndexId;

    typedef boost::bimap<IndexId, QString> IndexIdToLocalUidBimap;
    typedef boost::bimap<IndexId, QString> IndexIdToLinkedNotebookGuidBimap;

    struct LessByName
    {
        bool operator()(const TagModelItem & lhs, const TagModelItem & rhs) const;
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const TagModelItem & lhs, const TagModelItem & rhs) const;
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

    typedef QMap<QString, TagModelItem> ModelItems;
    typedef QMap<QString, TagLinkedNotebookRootItem> LinkedNotebookItems;

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
        const Tag & tag,
        const QStringList * pTagNoteLocalUids = nullptr);

    void onTagAdded(const Tag & tag, const QStringList * pTagNoteLocalUids);

    void onTagUpdated(
        const Tag & tag, TagDataByLocalUid::iterator it,
        const QStringList * pTagNoteLocalUids);

    void tagToItem(const Tag & tag, TagItem & item);
    bool canUpdateTagItem(const TagItem & item) const;
    bool canCreateTagItem(const TagModelItem & parentItem) const;
    void updateRestrictionsFromNotebook(const Notebook & notebook);

    void onLinkedNotebookAddedOrUpdated(const LinkedNotebook & linkedNotebook);

    const TagModelItem * itemForId(const IndexId id) const;
    IndexId idForItem(const TagModelItem & item) const;

private:
    Account                 m_account;
    TagData                 m_data;
    TagModelItem *          m_fakeRootItem;

    TagCache &              m_cache;

    ModelItems              m_modelItemsByLocalUid;
    ModelItems              m_modelItemsByLinkedNotebookGuid;

    LinkedNotebookItems     m_linkedNotebookItems;

    mutable IndexIdToLocalUidBimap              m_indexIdToLocalUidBimap;
    mutable IndexIdToLinkedNotebookGuidBimap    m_indexIdToLinkedNotebookGuidBimap;
    mutable IndexId                             m_lastFreeIndexId;

    size_t                  m_listTagsOffset;
    QUuid                   m_listTagsRequestId;
    QSet<QUuid>             m_tagItemsNotYetInLocalStorageUids;

    QSet<QUuid>             m_addTagRequestIds;
    QSet<QUuid>             m_updateTagRequestIds;
    QSet<QUuid>             m_expungeTagRequestIds;

    QSet<QUuid>             m_noteCountPerTagRequestIds;
    QUuid                   m_noteCountsPerAllTagsRequestId;

    QSet<QUuid>             m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findTagToPerformUpdateRequestIds;
    QSet<QUuid>             m_findTagAfterNotelessTagsErasureRequestIds;

    QSet<QUuid>             m_listTagsPerNoteRequestIds;

    QHash<QString,QString>  m_linkedNotebookOwnerUsernamesByLinkedNotebookGuids;
    size_t                  m_listLinkedNotebooksOffset;
    QUuid                   m_listLinkedNotebooksRequestId;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    struct Restrictions
    {
        Restrictions() :
            m_canCreateTags(false),
            m_canUpdateTags(false)
        {}

        bool    m_canCreateTags;
        bool    m_canUpdateTags;
    };

    QHash<QString, Restrictions>    m_tagRestrictionsByLinkedNotebookGuid;

    typedef boost::bimap<QString, QUuid> LinkedNotebookGuidWithFindNotebookRequestIdBimap;
    LinkedNotebookGuidWithFindNotebookRequestIdBimap    m_findNotebookRequestForLinkedNotebookGuid;

    mutable int                     m_lastNewTagNameCounter;
    mutable QMap<QString,int>       m_lastNewTagNameCounterByLinkedNotebookGuid;

    bool                            m_allTagsListed;
    bool                            m_allLinkedNotebooksListed;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_TAG_MODEL_H
