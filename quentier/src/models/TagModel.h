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

#ifndef QUENTIER_MODELS_TAG_MODEL_H
#define QUENTIER_MODELS_TAG_MODEL_H

#include "TagModelItem.h"
#include "TagCache.h"
#include <quentier/types/Tag.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>
#endif

#define TAG_MODEL_MIME_TYPE "application/x-com.quentier.tagmodeldatalist"
#define TAG_MODEL_MIME_DATA_MAX_COMPRESSION (9)

namespace quentier {

class TagModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TagModel(const Account & account, LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                      TagCache & cache, QObject * parent = Q_NULLPTR);
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

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    const TagModelItem * itemForIndex(const QModelIndex & index) const;
    const TagModelItem * itemForLocalUid(const QString & localUid) const;
    QModelIndex indexForItem(const TagModelItem * item) const;
    QModelIndex indexForLocalUid(const QString & localUid) const;
    QModelIndex indexForTagName(const QString & tagName) const;

    QModelIndex promote(const QModelIndex & index);
    QModelIndex demote(const QModelIndex & index);

    QModelIndexList persistentIndexes() const;

    /**
     * @brief moveToParent - moves the tag item pointed to by the index under the specified parent tag item
     * @param index - the index of the tag item to be moved under the new parent
     * @param parentTagName - the name of the parent tag under which the tag item is to be moved
     * @return the index of the moved tag item or invalid model index if something went wrong,
     * for example, if the passed in index did not really point to the tag item
     */
    QModelIndex moveToParent(const QModelIndex & index, const QString & parentTagName);

    /**
     * @brief removeFromParent - removes the tag item pointed to by the index from its parent tag (if any)
     * @param index - the index of the tag to be removed from its parent
     * @return the index of the tag item removed from its parent or invalid index
     * if something went wrong, for example if the passed in index did not really point to the tag item
     */
    QModelIndex removeFromParent(const QModelIndex & index);

    /**
     * @brief tagNames
     * @return the sorted (in case insensitive manner) list of tag names existing within the tag model
     */
    QStringList tagNames() const;

    /**
     * @brief createTag - convenience method to create a new tag within the model
     * @param tagName - the name of the new tag
     * @param parentTagName - the name of the parent tag for the new tag (empty for no parent tag)
     * @param errorDescription - the textual description of the error if tag was not created successfully
     * @return either valid model index if tag was created successfully or invalid model index otherwise
     */
    QModelIndex createTag(const QString & tagName, const QString & parentTagName,
                          QNLocalizedString & errorDescription);

    /**
     * @brief columnName
     * @param column - the column which name needs to be returned
     * @return the name of the column
     */
    QString columnName(const Columns::type column) const;

    /**
     * @brief allTagsListed
     * @return true if the tag model has received the information about all tags
     * stored in the local storage by the moment; false otherwise
     */
    bool allTagsListed() const { return m_allTagsListed; }

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex parent(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    virtual void sort(int column, Qt::SortOrder order) Q_DECL_OVERRIDE;

    // Drag-n-drop interfaces
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE { return Qt::MoveAction; }
#else
    Qt::DropActions supportedDragActions() const { return Qt::MoveAction; }
#endif

    virtual Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE { return Qt::MoveAction; }
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QMimeData * mimeData(const QModelIndexList & indexes) const Q_DECL_OVERRIDE;
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action,
                              int row, int column, const QModelIndex & parent) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void sortingChanged();
    void notifyError(QNLocalizedString errorDescription);
    void notifyAllTagsListed();

// private signals
    void addTag(Tag tag, QUuid requestId);
    void updateTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);
    void listTags(LocalStorageManager::ListObjectsOptions flag,
                  size_t limit, size_t offset,
                  LocalStorageManager::ListTagsOrder::type order,
                  LocalStorageManager::OrderDirection::type orderDirection,
                  QString linkedNotebookGuid, QUuid requestId);
    void expungeTag(Tag tag, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);
    void requestNoteCountPerTag(Tag tag, QUuid requestId);
    void listAllTagsPerNote(Note note, LocalStorageManager::ListObjectsOptions flag,
                            size_t limit, size_t offset,
                            LocalStorageManager::ListTagsOrder::type order,
                            LocalStorageManager::OrderDirection::type orderDirection,
                            QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onListTagsComplete(LocalStorageManager::ListObjectsOptions flag,
                            size_t limit, size_t offset,
                            LocalStorageManager::ListTagsOrder::type order,
                            LocalStorageManager::OrderDirection::type orderDirection,
                            QString linkedNotebookGuid, QList<Tag> foundTags, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onNoteCountPerTagComplete(int noteCount, Tag tag, QUuid requestId);
    void onNoteCountPerTagFailed(QNLocalizedString errorDescription, Tag tag, QUuid requestId);

    void onExpungeNotelessTagsFromLinkedNotebooksComplete(QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onAddNoteComplete(Note note, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onListAllTagsPerNoteComplete(QList<Tag> foundTags, Note note,
                                      LocalStorageManager::ListObjectsOptions flag,
                                      size_t limit, size_t offset,
                                      LocalStorageManager::ListTagsOrder::type order,
                                      LocalStorageManager::OrderDirection::type orderDirection,
                                      QUuid requestId);
    void onListAllTagsPerNoteFailed(Note note, LocalStorageManager::ListObjectsOptions flag,
                                    size_t limit, size_t offset,
                                    LocalStorageManager::ListTagsOrder::type order,
                                    LocalStorageManager::OrderDirection::type orderDirection,
                                    QNLocalizedString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestTagsList();
    void requestNoteCountForTag(const Tag & tag);
    void requestTagsPerNote(const Note & note);
    void requestNoteCountForAllTags();

    QVariant dataImpl(const TagModelItem & item, const Columns::type column) const;
    QVariant dataAccessibleText(const TagModelItem & item, const Columns::type column) const;

    bool hasSynchronizableChildren(const TagModelItem * item) const;

    void mapChildItems();
    void mapChildItems(const TagModelItem & item);

    QString nameForNewTag() const;
    void removeItemByLocalUid(const QString & localUid);

    void removeModelItemFromParent(const TagModelItem & item);

    // Returns the appropriate row before which the new item should be inserted according to the current sorting criteria and column
    int rowForNewItem(const TagModelItem & parentItem, const TagModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(const TagModelItem & item);
    void updatePersistentModelIndices();
    void updateTagInLocalStorage(const TagModelItem & item);

    void tagFromItem(const TagModelItem & item, Tag & tag) const;

private:
    struct ByLocalUid{};
    struct ByParentLocalUid{};
    struct ByNameUpper{};

    typedef boost::multi_index_container<
        TagModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<TagModelItem,const QString&,&TagModelItem::localUid>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByParentLocalUid>,
                boost::multi_index::const_mem_fun<TagModelItem,const QString&,&TagModelItem::parentLocalUid>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<TagModelItem,QString,&TagModelItem::nameUpper>
            >
        >
    > TagData;

    typedef TagData::index<ByLocalUid>::type TagDataByLocalUid;
    typedef TagData::index<ByParentLocalUid>::type TagDataByParentLocalUid;
    typedef TagData::index<ByNameUpper>::type TagDataByNameUpper;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    typedef quint32 IndexId;
#else
    typedef quintptr IndexId;
#endif

    typedef boost::bimap<IndexId, QString> IndexIdToLocalUidBimap;

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

private:
    void onTagAddedOrUpdated(const Tag & tag);
    void onTagAdded(const Tag & tag);
    void onTagUpdated(const Tag & tag, TagDataByLocalUid::iterator it);
    void tagToItem(const Tag & tag, TagModelItem & item);
    bool canUpdateTagItem(const TagModelItem & item) const;
    bool canCreateTagItem(const TagModelItem & parentItem) const;
    void updateRestrictionsFromNotebook(const Notebook & notebook);

    const TagModelItem * itemForId(const IndexId id) const;
    IndexId idForItem(const TagModelItem & item) const;

private:
    Account                 m_account;
    TagData                 m_data;
    TagModelItem *          m_fakeRootItem;

    TagCache &              m_cache;

    mutable IndexIdToLocalUidBimap  m_indexIdToLocalUidBimap;
    mutable IndexId                 m_lastFreeIndexId;

    QSet<QString>           m_lowerCaseTagNames;

    size_t                  m_listTagsOffset;
    QUuid                   m_listTagsRequestId;
    QSet<QUuid>             m_tagItemsNotYetInLocalStorageUids;

    QSet<QUuid>             m_addTagRequestIds;
    QSet<QUuid>             m_updateTagRequestIds;
    QSet<QUuid>             m_expungeTagRequestIds;

    QSet<QUuid>             m_noteCountPerTagRequestIds;

    QSet<QUuid>             m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findTagToPerformUpdateRequestIds;
    QSet<QUuid>             m_findTagAfterNotelessTagsErasureRequestIds;

    QSet<QUuid>             m_listTagsPerNoteRequestIds;

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

    mutable int             m_lastNewTagNameCounter;

    bool                    m_allTagsListed;
};

} // namespace quentier

#endif // QUENTIER_MODELS_TAG_MODEL_H
