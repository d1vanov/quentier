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

#ifndef QUENTIER_LIB_MODEL_NOTE_MODEL_H
#define QUENTIER_LIB_MODEL_NOTE_MODEL_H

#include "NoteModelItem.h"
#include "NoteCache.h"
#include "NotebookCache.h"

#include <lib/utility/IStartable.h>

#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>

#include <QAbstractItemModel>
#include <QScopedPointer>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/bimap.hpp>

namespace quentier {

class NoteModel: public QAbstractItemModel, public IStartable
{
    Q_OBJECT
public:
    struct IncludedNotes
    {
        enum type
        {
            All = 0,
            NonDeleted,
            Deleted
        };
    };

    struct NoteSortingMode
    {
        enum type
        {
            CreatedAscending = 0,
            CreatedDescending,
            ModifiedAscending,
            ModifiedDescending,
            TitleAscending,
            TitleDescending,
            SizeAscending,
            SizeDescending,
            None
        };
    };

    struct Columns
    {
        enum type
        {
            CreationTimestamp = 0,
            ModificationTimestamp,
            DeletionTimestamp,
            Title,
            PreviewText,
            ThumbnailImage,
            NotebookName,
            TagNameList,
            Size,
            Synchronizable,
            Dirty,
            HasResources
        };
    };

    class NoteFilters
    {
    public:
        NoteFilters();

        bool isEmpty() const;

        const QStringList & filteredNotebookLocalUids() const;
        void setFilteredNotebookLocalUids(const QStringList & notebookLocalUids);
        void clearFilteredNotebookLocalUids();

        const QStringList & filteredTagLocalUids() const;
        void setFilteredTagLocalUids(const QStringList & tagLocalUids);
        void clearFilteredTagLocalUids();

        const QSet<QString> & filteredNoteLocalUids() const;
        bool setFilteredNoteLocalUids(const QSet<QString> & noteLocalUids);
        bool setFilteredNoteLocalUids(const QStringList & noteLocalUids);
        void clearFilteredNoteLocalUids();

    private:
        Q_DISABLE_COPY(NoteFilters);

    private:
        QStringList             m_filteredNotebookLocalUids;
        QStringList             m_filteredTagLocalUids;
        QSet<QString>           m_filteredNoteLocalUids;
    };


    explicit NoteModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent = nullptr,
        const IncludedNotes::type includedNotes = IncludedNotes::NonDeleted,
        const NoteSortingMode::type noteSortingMode = NoteSortingMode::ModifiedAscending,
        NoteFilters * pFilters = nullptr);

    virtual ~NoteModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    Columns::type sortingColumn() const;
    Qt::SortOrder sortOrder() const;

    QModelIndex indexForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemAtRow(const int row) const;
    const NoteModelItem * itemForIndex(const QModelIndex & index) const;

public:
    // Note filtering API

    bool hasFilters() const;

    const QStringList & filteredNotebookLocalUids() const;
    void setFilteredNotebookLocalUids(const QStringList & notebookLocalUids);
    void clearFilteredNotebookLocalUids();

    const QStringList & filteredTagLocalUids() const;
    void setFilteredTagLocalUids(const QStringList & tagLocalUids);
    void clearFilteredTagLocalUids();

    const QSet<QString> & filteredNoteLocalUids() const;
    void setFilteredNoteLocalUids(const QSet<QString> & noteLocalUids);
    void setFilteredNoteLocalUids(const QStringList & noteLocalUids);
    void clearFilteredNoteLocalUids();

    void beginUpdateFilter();
    void endUpdateFilter();

    /**
     * @brief Total number of notes conforming with the specified filters
     * within the local storage database (not necessarily equal to the number
     * of rows within the model - it's typically smaller due to lazy loading)
     */
    qint32 totalFilteredNotesCount() const;

    /**
     * @brief Total number of notes per account within the local storage
     * database not considering the filters set to the note model
     */
    qint32 totalAccountNotesCount() const;

public:
    /**
     * @brief createNoteItem - attempts to create a new note within the notebook
     * specified by local uid
     * @param notebookLocalUid      The local uid of notebook in which the new
     *                              note is to be created
     * @param errorDescription      Textual description of the error in case
     *                              the new note cannot be created
     * @return                      The model index of the newly created note
     *                              item or invalid modex index if the new note
     *                              could not be created
     */
    QModelIndex createNoteItem(
        const QString & notebookLocalUid, ErrorString & erroDescription);

    /**
     * @brief deleteNote - attempts to mark the note with the specified
     * local uid as deleted.
     *
     * The model only looks at the notes contained in it, so if the model was
     * set up to include only already deleted notes or only non-deleted ones,
     * there's a chance the model wan't contain the note pointed to by the local
     * uid. In that case the deletion won't be successful.
     *
     * @param noteLocalUid          The local uid of note to be marked as deleted
     * @param errorDescription      Textual description of the error if the note
     *                              could not be marked as deleted
     * @return                      True if the note was deleted successfully,
     *                              false otherwise
     */
    bool deleteNote(
        const QString & noteLocalUid, ErrorString & errorDescription);

    /**
     * @brief moveNoteToNotebook - attempts to move the note to a different
     * notebook
     *
     * The method only checks the prerequisites and in case of no obstacles
     * sends the request to update the note-notebook binding within the local
     * storage but returns before the completion of this request. So if the
     * method returned successfully, it doesn't necessarily mean the note was
     * actually moved to another notebook - it only means the request to do so
     * was sent. If that request yields failure as a result, notifyError signal
     * is emitted upon the local storage response receipt.
     *
     * @param noteLocalUid          The local uid of note to be moved to another
     *                              notebook
     * @param notebookName          The name of the notebook into which the note
     *                              needs to be moved
     * @param errorDescription      Textual description of the error if the note
     *                              could not be moved to the specified notebook
     * @return                      True if the note was moved to the specified
     *                              notebook successfully, false otherwise
     */
    bool moveNoteToNotebook(
        const QString & noteLocalUid, const QString & notebookName,
        ErrorString & errorDescription);

    /**
     * @brief favoriteNote - attempts to mark the note with the specified local
     * uid as favorited
     *
     * Favorited property of Note class is not represented as a column within
     * the NoteModel so this method doesn't change anything in the model but
     * only the underlying note object persisted in the local storage.
     *
     * The method only checks the prerequisites and in case of no obstacles
     * sends the request to update the note's favorited state within the local
     * storage but returns before the completion of this request. So if
     * the method returned successfully, it doesn't necessarily mean the note
     * was actually favorited - it only means the request to do so was sent. If
     * that request yields failure as a result, notifyError signal is emitted
     * upon the local storage response receipt.
     *
     * @param noteLocalUid          The local uid of the note to be favorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be favorited
     * @return                      True if the note was favorited successfully,
     *                              false otherwise
     */
    bool favoriteNote(
        const QString & noteLocalUid, ErrorString & errorDescription);

    /**
     * @brief unfavoriteNote - attempts to remove the favorited mark from
     * the note with the specified local uid
     *
     * Favorited property of Note class is not represented as a column within
     * the NoteModel so this method doesn't change anything in the model but
     * only the underlying note object persisted in the local storage.
     *
     * The method only checks the prerequisites and in case of no obstacles
     * sends the request to update the note's favorited state within the local
     * storage but returns before the completion of this request. So if
     * the method returned successfully, it doesn't necessarily mean the note
     * was actually unfavorited - it only means the request to do so was sent.
     * If that request yields failure as a result, notifyError signal is emitted
     * upon the local storage response receipt.
     *
     * @param noteLocalUid          The local uid of the note to be unfavorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be unfavorited
     * @return                      True if the note was unfavorited successfully,
     *                              false otherwise
     */
    bool unfavoriteNote(
        const QString & noteLocalUid, ErrorString & errorDescription);

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(
        const QModelIndex & modelIndex) const override;

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
        int section, Qt::Orientation orientation, const QVariant & value,
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

    virtual bool canFetchMore(const QModelIndex & parent) const override;
    virtual void fetchMore(const QModelIndex & parent) override;

    // IStartable interface
    virtual void start() override;
    virtual bool isStarted() const override { return m_isStarted; }
    virtual void stop(const StopMode::type stopMode) override;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    /**
     * @brief noteCountPerAccountUpdated signal is emitted when NoteModel
     * detects the change of total notes count per account considering
     * included notes setting (deleted/non-deleted/all)
     *
     * @param noteCount         Updated total number of notes per account
     */
    void noteCountPerAccountUpdated(qint32 noteCount);

    /**
     * @brief filteredNotesCountUpdated signal is emitted when NoteModel
     * detects the change of filtered notes according to the current filtering
     * criteria
     *
     * @param noteCount         Updated filtered notes count
     */
    void filteredNotesCountUpdated(qint32 noteCount);

    /**
     * @brief minimalNotesBatchLoaded signal is emitted when NoteModel has
     * loaded a bunch of notes corresponding to the specified included notes
     * condition and filters (if any) and stopped loading the rest of conformant
     * notes - these would be loaded as necessary with the help of canFetchMore
     * and fetchMore methods
     */
    void minimalNotesBatchLoaded();

// private signals
    void addNote(Note note, QUuid requestId);

    void updateNote(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void findNote(
        Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void listNotes(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void listNotesPerNotebooksAndTags(
        QStringList notebookLocalUids, QStringList tagLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

    void listNotesByLocalUids(
        QStringList noteLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

    void getNoteCount(
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void getNoteCountPerNotebooksAndTags(
        QStringList notebookLocalUids,
        QStringList tagLocalUids,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void expungeNote(Note note, QUuid requestId);

    void findNotebook(Notebook notebook, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNoteComplete(Note note, QUuid requestId);

    void onAddNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onUpdateNoteComplete(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onUpdateNoteFailed(
        Note note, LocalStorageManager::UpdateNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onFindNoteComplete(
        Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesComplete(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Note> foundNotes,
        QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onListNotesPerNotebooksAndTagsComplete(
        QStringList notebookLocalUids,
        QStringList tagLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<Note> foundNotes, QUuid requestId);

    void onListNotesPerNotebooksAndTagsFailed(
        QStringList notebookLocalUids,
        QStringList tagLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesByLocalUidsComplete(
        QStringList noteLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<Note> foundNotes, QUuid requestId);

    void onListNotesByLocalUidsFailed(
        QStringList noteLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountComplete(
        int noteCount, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountFailed(
        ErrorString errorDescription,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerNotebooksAndTagsComplete(
        int noteCount, QStringList notebookLocalUids, QStringList tagLocalUids,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerNotebooksAndTagsFailed(
        ErrorString errorDescription,
        QStringList notebookLocalUids, QStringList tagLocalUids,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    void onExpungeNoteFailed(
        Note note, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onAddTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);

    void onExpungeTagComplete(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void onNoteAddedOrUpdated(
        const Note & note, const bool fromNotesListing = false);

    void noteToItem(const Note & note, NoteModelItem & item);
    bool noteConformsToFilter(const Note & note) const;
    void onListNotesCompleteImpl(const QList<Note> foundNotes);

    void requestNotesListAndCount();
    void requestNotesList();
    void requestNotesCount();
    void requestTotalNotesCountPerAccount();
    void requestTotalFilteredNotesCount();

    void findNoteToRestoreFailedUpdate(const Note & note);

    void clearModel();
    void resetModel();

    LocalStorageManager::NoteCountOptions noteCountOptions() const;

    /**
     * @param newItem       New note model item about to be inserted into
     *                      the model
     * @return              The appropriate row before which the new item should
     *                      be inserted according to the current sorting criteria
     *                      and column
     */
    int rowForNewItem(const NoteModelItem & newItem) const;

    void processTagExpunging(const QString & tagLocalUid);

    void removeItemByLocalUid(const QString & localUid);

    bool updateItemRowWithRespectToSorting(
        const NoteModelItem & item, ErrorString & errorDescription);

    void saveNoteInLocalStorage(
        const NoteModelItem & item, const bool saveTags = false);

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    bool setDataImpl(const QModelIndex & index, const QVariant & value,
                     ErrorString & errorDescription);

    bool removeRowsImpl(int row, int count, ErrorString & errorDescription);

    bool canUpdateNoteItem(const NoteModelItem & item) const;
    bool canCreateNoteItem(const QString & notebookLocalUid) const;
    void updateNotebookData(const Notebook & notebook);

    bool setNoteFavorited(
        const QString & noteLocalUid, const bool favorited,
        ErrorString & errorDescription);

    void setSortingColumnAndOrder(const int column, const Qt::SortOrder order);
    void setSortingOrder(const Qt::SortOrder order);

private:
    struct ByIndex{};
    struct ByLocalUid{};
    struct ByNotebookLocalUid{};

    typedef boost::multi_index_container<
        NoteModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<
                    NoteModelItem,const QString&,&NoteModelItem::localUid>
            >,
            boost::multi_index::hashed_non_unique<
                boost::multi_index::tag<ByNotebookLocalUid>,
                boost::multi_index::const_mem_fun<
                    NoteModelItem,const QString&,&NoteModelItem::notebookLocalUid>
            >
        >
    > NoteData;

    typedef NoteData::index<ByIndex>::type NoteDataByIndex;
    typedef NoteData::index<ByLocalUid>::type NoteDataByLocalUid;
    typedef NoteData::index<ByNotebookLocalUid>::type NoteDataByNotebookLocalUid;

    class NoteComparator
    {
    public:
        NoteComparator(const Columns::type column,
                       const Qt::SortOrder sortOrder) :
            m_sortedColumn(column),
            m_sortOrder(sortOrder)
        {}

        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;

    private:
        Columns::type   m_sortedColumn;
        Qt::SortOrder   m_sortOrder;
    };

    struct NotebookData
    {
        NotebookData() :
            m_name(),
            m_guid(),
            m_canCreateNotes(false),
            m_canUpdateNotes(false)
        {}

        QString m_name;
        QString m_guid;
        bool    m_canCreateNotes;
        bool    m_canUpdateNotes;
    };

    struct TagData
    {
        QString m_name;
        QString m_guid;
    };

    typedef boost::bimap<QString, QUuid> LocalUidToRequestIdBimap;

private:
    // WARNING: this method assumes the iterator passed to it is not end()
    bool moveNoteToNotebookImpl(
        NoteDataByLocalUid::iterator it, const Notebook & notebook,
        ErrorString & errorDescription);

    void addOrUpdateNoteItem(
        NoteModelItem & item, const NotebookData & notebookData,
        const bool fromNotesListing);

    void checkMaxNoteCountAndRemoveLastNoteIfNeeded();

    void checkAddedNoteItemsPendingNotebookData(
        const QString & notebookLocalUid, const NotebookData & notebookData);

    void findTagNamesForItem(NoteModelItem & item);

    void updateTagData(const Tag & tag);

private:
    Account                     m_account;
    const IncludedNotes::type   m_includedNotes;
    NoteSortingMode::type       m_noteSortingMode;

    LocalStorageManagerAsync &  m_localStorageManagerAsync;
    bool                        m_connectedToLocalStorage;

    bool                        m_isStarted;

    NoteData                    m_data;
    qint32                      m_totalFilteredNotesCount;

    NoteCache &                 m_cache;
    NotebookCache &             m_notebookCache;

    QScopedPointer<NoteFilters> m_pFilters;
    QScopedPointer<NoteFilters> m_pUpdatedNoteFilters;

    // Upper bound for the amount of notes stored within the note model.
    // Can be increased through calls to fetchMore()
    size_t                      m_maxNoteCount;

    size_t                      m_listNotesOffset;
    QUuid                       m_listNotesRequestId;
    QUuid                       m_getNoteCountRequestId;

    qint32                      m_totalAccountNotesCount;
    QUuid                       m_getFullNoteCountPerAccountRequestId;

    QHash<QString, NotebookData>    m_notebookDataByNotebookLocalUid;
    LocalUidToRequestIdBimap        m_findNotebookRequestForNotebookLocalUid;

    QSet<QUuid>                 m_localUidsOfNewNotesBeingAddedToLocalStorage;

    QSet<QUuid>                 m_addNoteRequestIds;
    QSet<QUuid>                 m_updateNoteRequestIds;
    QSet<QUuid>                 m_expungeNoteRequestIds;

    QSet<QUuid>                 m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid>                 m_findNoteToPerformUpdateRequestIds;

    // The key is notebook local uid
    QMultiHash<QString, NoteModelItem>  m_noteItemsPendingNotebookDataUpdate;

    LocalUidToRequestIdBimap    m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap;

    QHash<QString, TagData>     m_tagDataByTagLocalUid;

    LocalUidToRequestIdBimap        m_findTagRequestForTagLocalUid;
    QMultiHash<QString, QString>    m_tagLocalUidToNoteLocalUid;
};

} // namespace quentier

inline std::size_t hash_value(QString x) { return qHash(x); }

#endif // QUENTIER_LIB_MODEL_NOTE_MODEL_H
