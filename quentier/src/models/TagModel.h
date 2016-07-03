#ifndef QUENTIER_MODELS_TAG_MODEL_H
#define QUENTIER_MODELS_TAG_MODEL_H

#include "TagModelItem.h"
#include "TagCache.h"
#include <quentier/types/Tag.h>
#include <quentier/types/Notebook.h>
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
    explicit TagModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                      TagCache & cache, QObject * parent = Q_NULLPTR);
    virtual ~TagModel();

    struct Columns
    {
        enum type {
            Name = 0,
            Synchronizable,
            Dirty,
            FromLinkedNotebook
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    const TagModelItem * itemForIndex(const QModelIndex & index) const;
    QModelIndex indexForItem(const TagModelItem * item) const;
    QModelIndex indexForLocalUid(const QString & localUid) const;

    QModelIndex promote(const QModelIndex & index);
    QModelIndex demote(const QModelIndex & index);

    QStringList tagNames() const;

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
    void notifyError(QString errorDescription);

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

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId);
    void onListTagsComplete(LocalStorageManager::ListObjectsOptions flag,
                            size_t limit, size_t offset,
                            LocalStorageManager::ListTagsOrder::type order,
                            LocalStorageManager::OrderDirection::type orderDirection,
                            QString linkedNotebookGuid, QList<Tag> foundTags, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid, QString errorDescription, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestTagsList();

    QVariant dataImpl(const TagModelItem & item, const Columns::type column) const;
    QVariant dataAccessibleText(const TagModelItem & item, const Columns::type column) const;

    bool hasSynchronizableChildren(const TagModelItem * item) const;

    void mapChildItems();
    void mapChildItems(const TagModelItem & item);

    QString nameForNewTag() const;
    void removeItemByLocalUid(const QString & localUid);

    // Returns the appropriate row before which the new item should be inserted according to the current sorting criteria and column
    int rowForNewItem(const TagModelItem & parentItem, const TagModelItem & newItem) const;

    void updateItemRowWithRespectToSorting(const TagModelItem & item);
    void updatePersistentModelIndices();

    void updateTagInLocalStorage(const TagModelItem & item);

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

private:
    TagData                 m_data;
    TagModelItem *          m_fakeRootItem;

    TagCache &              m_cache;

    QSet<QString>           m_lowerCaseTagNames;

    size_t                  m_listTagsOffset;
    QUuid                   m_listTagsRequestId;
    QSet<QUuid>             m_tagItemsNotYetInLocalStorageUids;

    QSet<QUuid>             m_addTagRequestIds;
    QSet<QUuid>             m_updateTagRequestIds;
    QSet<QUuid>             m_expungeTagRequestIds;

    QSet<QUuid>             m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findTagToPerformUpdateRequestIds;

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
};

} // namespace quentier

#endif // QUENTIER_MODELS_TAG_MODEL_H
