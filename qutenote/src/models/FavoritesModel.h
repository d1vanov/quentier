#ifndef QUTE_NOTE_MODELS_FAVORITES_MODEL_H
#define QUTE_NOTE_MODELS_FAVORITES_MODEL_H

#include "FavoritesModelItem.h"
#include "NoteCache.h"
#include "NotebookCache.h"
#include "TagCache.h"
#include "SavedSearchCache.h"
#include <qute_note/types/Notebook.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/Tag.h>
#include <qute_note/types/SavedSearch.h>
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QHash>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#endif

namespace qute_note {

class FavoritesModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FavoritesModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                            NoteCache & noteCache, NotebookCache & notebookCache, TagCache & tagCache,
                            SavedSearchCache & savedSearchCache, QObject * parent = Q_NULLPTR);
    virtual ~FavoritesModel();

    struct Columns
    {
        enum type
        {
            Type = 0,
            DisplayName,
            NumTargetedItems
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    QModelIndex indexForLocalUid(const QString & localUid) const;
    const FavoritesModelItem * itemForLocalUid(const QString & localUid) const;
    const FavoritesModelItem * itemAtRow(const int row) const;

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

Q_SIGNALS:
    void notifyError(QString errorDescription);

// private signals
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void listNotes(LocalStorageManager::ListObjectsOptions flag,
                   bool withResourceBinaryData, size_t limit, size_t offset,
                   LocalStorageManager::ListNotesOrder::type order,
                   LocalStorageManager::OrderDirection::type orderDirection,
                   QUuid requestId);

    void updateNotebook(Notebook notebook, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);
    void listNotebooks(LocalStorageManager::ListObjectsOptions flag,
                       size_t limit, size_t offset,
                       LocalStorageManager::ListNotebooksOrder::type order,
                       LocalStorageManager::OrderDirection::type orderDirection,
                       QString linkedNotebookGuid, QUuid requestId);

    void updateTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);
    void listTags(LocalStorageManager::ListObjectsOptions flag,
                  size_t limit, size_t offset,
                  LocalStorageManager::ListTagsOrder::type order,
                  LocalStorageManager::OrderDirection::type orderDirection,
                  QString linkedNotebookGuid, QUuid requestId);

    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void findSavedSearch(SavedSearch search, QUuid requestId);
    void listSavedSearches(LocalStorageManager::ListObjectsOptions flag,
                           size_t limit, size_t offset,
                           LocalStorageManager::ListSavedSearchesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage

    // For notes:
    void onAddNoteComplete(Note note, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, QString errorDescription, QUuid requestId);
    void onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                             size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                             LocalStorageManager::OrderDirection::type orderDirection,
                             QList<Note> foundNotes, QUuid requestId);
    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                           size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    // For notebooks:
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                 size_t limit, size_t offset,
                                 LocalStorageManager::ListNotebooksOrder::type order,
                                 LocalStorageManager::OrderDirection::type orderDirection,
                                 QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
                                 QUuid requestId);
    void onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                               size_t limit, size_t offset,
                               LocalStorageManager::ListNotebooksOrder::type order,
                               LocalStorageManager::OrderDirection::type orderDirection,
                               QString linkedNotebookGuid, QString errorDescription, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    // For tags:
    void onAddTagComplete(Tag tag, QUuid requestId);
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

    // For saved searches:
    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onFindSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onListSavedSearchesComplete(LocalStorageManager::ListObjectsOptions flag,
                                     size_t limit, size_t offset,
                                     LocalStorageManager::ListSavedSearchesOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QList<SavedSearch> foundSearches, QUuid requestId);
    void onListSavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                   size_t limit, size_t offset,
                                   LocalStorageManager::ListSavedSearchesOrder::type order,
                                   LocalStorageManager::OrderDirection::type orderDirection,
                                   QString errorDescription, QUuid requestId);
    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestNotesList();
    void requestNotebooksList();
    void requestTagsList();
    void requestSavedSearchesList();

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    void removeItemByLocalUid(const QString & localUid);
    void updateItemRowWithRespectToSorting(const FavoritesModelItem & item);
    void updateItemInLocalStorage(const FavoritesModelItem & item);

    bool canUpdateNote(const QString & localUid) const;
    bool canUpdateNotebook(const QString & localUid) const;
    bool canUpdateTag(const QString & localUid) const;

private:
    struct ByLocalUid{};
    struct ByIndex{};

    typedef boost::multi_index_container<
        FavoritesModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<FavoritesModelItem,const QString&,&FavoritesModelItem::localUid>
            >
        >
    > FavoritesData;

    typedef FavoritesData::index<ByLocalUid>::type FavoritesDataByLocalUid;
    typedef FavoritesData::index<ByIndex>::type FavoritesDataByIndex;

    struct NotebookRestrictionsData
    {
        NotebookRestrictionsData() :
            m_guid(),
            m_canUpdateNotes(false),
            m_canUpdateNotebook(false),
            m_canUpdateTags(false)
        {}

        QString m_guid;
        bool    m_canUpdateNotes;
        bool    m_canUpdateNotebook;
        bool    m_canUpdateTags;
    };

    QHash<QString, QString>     m_tagLocalUidToLinkedNotebookGuid;
    QHash<QString, QString>     m_notebookLocalUidToGuid;
    QHash<QString, QString>     m_noteLocalUidToNotebookGuid;

private:
    FavoritesData           m_data;
    NoteCache &             m_noteCache;
    NotebookCache &         m_notebookCache;
    TagCache &              m_tagCache;
    SavedSearchCache &      m_savedSearchCache;

    size_t                  m_listNotesOffset;
    QUuid                   m_listNotesRequestId;

    size_t                  m_listNotebooksOffset;
    QUuid                   m_listNotebooksRequestId;

    size_t                  m_listTagsOffset;
    QUuid                   m_listTagsRequestId;

    size_t                  m_listSavedSearchesOffset;
    QUuid                   m_listSavedSearchesRequestId;

    QSet<QUuid>             m_updateNoteRequestIds;
    QSet<QUuid>             m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNoteToPerformUpdateRequestIds;

    QSet<QUuid>             m_updateNotebookRequestIds;
    QSet<QUuid>             m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNotebookToPerformUpdateRequestIds;

    QSet<QUuid>             m_updateTagRequestIds;
    QSet<QUuid>             m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findTagToPerformUpdateRequestIds;

    QSet<QUuid>             m_updateSavedSearchRequestIds;
    QSet<QUuid>             m_findSavedSearchToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findSavedSearchToPerformUpdateRequestIds;

    int                     m_sortedColumn;
    Qt::SortOrder           m_sortOrder;
};

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_FAVORITES_MODEL_H
