/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_NOTE_MODEL2_H
#define QUENTIER_MODELS_NOTE_MODEL2_H

#include "NoteModelItem.h"
#include "NoteCache.h"
#include "NotebookCache.h"
#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <QAbstractItemModel>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/bimap.hpp>
#endif

namespace quentier {

class NoteModel2: public QAbstractItemModel
{
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

        const QStringList & filteredTagLocalUids() const;
        void setFilteredTagLocalUids(const QStringList & tagLocalUids);

        const QStringList & filteredNoteLocalUids() const;
        void setFilteredNoteLocalUids(const QStringList & noteLocalUids);

        void beginUpdateFilter();
        void endUpdateFilter();

    private:
        Q_DISABLE_COPY(NoteFilters);

    private:
        QStringList             m_filteredNotebookLocalUids;
        QStringList             m_filteredTagLocalUids;
        QStringList             m_filteredNoteLocalUids;

        QStringList             m_updatedFilteredNotebookLocalUids;
        QStringList             m_updatedFilteredTagLocalUids;
        QStringList             m_updatedFilteredNoteLocalUids;

        bool                    m_updatingFilters;
    };


    explicit NoteModel2(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent = Q_NULLPTR,
        const IncludedNotes::type includedNotes = IncludedNotes::NonDeleted,
        const NoteSortingMode::type noteSortingMode = NoteSortingMode::ModifiedAscending,
        NoteFilters * pFilters = Q_NULLPTR);

    virtual ~NoteModel2();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    int sortingColumn() const;
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

    const QStringList & filteredTagLocalUids() const;
    void setFilteredTagLocalUids(const QStringList & tagLocalUids);

    const QStringList & filteredNoteLocalUids() const;
    void setFilteredNoteLocalUids(const QStringList & noteLocalUids);

    void beginUpdateFilter();
    void endUpdateFilter();

    /**
     * @brief Total number of notes confrming to the specified filters
     * within the local storage database (not necessarily equal to the number
     * of rows within the model - it's typically smaller due to lazy loading)
     */
    int totalNoteCount() const;

public:
    /**
     * @brief createNoteItem - attempts to create a new note within the notebook
     * specified by local uid
     * @param notebookLocalUid      The local uid of notebook in which the new
     *                              note is to be created
     * @return                      The model index of the newly created note
     *                              item or invalid modex index if the new note
     *                              could not be created
     */
    QModelIndex createNoteItem(const QString & notebookLocalUid);

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
     * @return                      True if the note was deleted successfully,
     *                              false otherwise
     */
    bool deleteNote(const QString & noteLocalUid);

    /**
     * @brief moveNoteToNotebook - attempts to move the note to a different notebook
     *
     * The method doesn't have a return code because it might have to do its job
     * asynchronously; if the error happens during the process (for example,
     * the target notebook was not found by name), notifyError signal is emitted
     *
     * @param noteLocalUid          The local uid of note to be moved to another
     *                              notebook
     * @param notebookName          The name of the notebook into which the note
     *                              needs to be moved
     */
    void moveNoteToNotebook(const QString & noteLocalUid,
                            const QString & notebookName);

    /**
     * @brief favoriteNote - attempts to mark the note with the specified local
     * uid as favorited
     *
     * Favorited property of Note class is not represented as a column within
     * the NoteModel2 so this method doesn't change anything in the model but
     * only the underlying note object persisted in the local storage
     *
     * @param noteLocalUid          The local uid of the note to be favorited
     */
    void favoriteNote(const QString & noteLocalUid);

    /**
     * @brief unfavoriteNote - attempts to remove the favorited mark from
     * the note with the specified local uid
     *
     * Favorited property of Note class is not represented as a column within
     * the NoteModel2 so this method doesn't change anything in the model but
     * only the underlying note object persisted in the local storage
     *
     * @param noteLocalUid          The local uid of the note to be unfavorited
     */
    void unfavoriteNote(const QString & noteLocalUid);

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index,
                          int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section,
                                Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column,
                              const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex parent(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual bool setHeaderData(int section,
                               Qt::Orientation orientation,
                               const QVariant & value,
                               int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index,
                         const QVariant & value,
                         int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool insertRows(int row,
                            int count,
                            const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row,
                            int count,
                            const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    virtual void sort(int column, Qt::SortOrder order) Q_DECL_OVERRIDE;

    virtual bool canFetchMore(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual void fetchMore(const QModelIndex & parent) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

// private signals
    void addNote(Note note, QUuid requestId);
    void updateNote(Note note,
                    LocalStorageManager::UpdateNoteOptions options,
                    QUuid requestId);
    void findNote(Note note,
                  LocalStorageManager::GetNoteOptions options,
                  QUuid requestId);

    void listNotes(LocalStorageManager::ListObjectsOptions flag,
                   LocalStorageManager::GetNoteOptions options,
                   size_t limit, size_t offset,
                   LocalStorageManager::ListNotesOrder::type order,
                   LocalStorageManager::OrderDirection::type orderDirection,
                   QString linkedNotebookGuid, QUuid requestId);

    void listNotesPerNotebooksAndTags(
        QStringList notebookLocalUids, QStringList tagLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QUuid requestId);

    void listNotesByLocalUids(
        QStringList noteLocalUids,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QUuid requestId);

    void getNoteCount(LocalStorageManager::NoteCountOptions options,
                      QUuid requestId);

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
    void onAddNoteFailed(Note note, ErrorString errorDescription,
                         QUuid requestId);

    void onUpdateNoteComplete(Note note,
                              LocalStorageManager::UpdateNoteOptions options,
                              QUuid requestId);
    void onUpdateNoteFailed(Note note,
                            LocalStorageManager::UpdateNoteOptions options,
                            ErrorString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note,
                            LocalStorageManager::GetNoteOptions options,
                            QUuid requestId);
    void onFindNoteFailed(Note note,
                          LocalStorageManager::GetNoteOptions options,
                          ErrorString errorDescription, QUuid requestId);

    void onListNotesComplete(LocalStorageManager::ListObjectsOptions flag,
                             LocalStorageManager::GetNoteOptions options,
                             size_t limit, size_t offset,
                             LocalStorageManager::ListNotesOrder::type order,
                             LocalStorageManager::OrderDirection::type orderDirection,
                             QString linkedNotebookGuid, QList<Note> foundNotes,
                             QUuid requestId);

    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag,
                           LocalStorageManager::GetNoteOptions options,
                           size_t limit, size_t offset,
                           LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QString linkedNotebookGuid, ErrorString errorDescription,
                           QUuid requestId);

    void onListNotesPerNotebooksAndTagsComplete(
        QStringList notebookLocalUids,
        QStringList tagLocalUids,
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QList<Note> foundNotes, QUuid requestId);

    void onListNotesPerNotebooksAndTagsFailed(
        QStringList notebookLocalUids,
        QStringList tagLocalUids,
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesByLocalUidsComplete(
        QStringList noteLocalUids,
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
        QList<Note> foundNotes, QUuid requestId);

    void onListNotesByLocalUidsFailed(
        QStringList noteLocalUids,
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder::type order,
        LocalStorageManager::OrderDirection::type orderDirection,
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
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onExpungeNoteComplete(Note note, QUuid requestId);
    void onExpungeNoteFailed(Note note, ErrorString errorDescription,
                             QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription,
                              QUuid requestId);
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onAddTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids,
                              QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotesListAndCount();

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

private:
    Account                 m_account;
    IncludedNotes::type     m_includedNotes;
    NoteSortingMode::type   m_noteSortingMode;

    NoteData                m_data;

    NoteCache &             m_cache;
    NotebookCache &         m_notebookCache;

    NoteFilters *           m_pFilters;

    size_t                  m_listNotesOffset;
    QUuid                   m_listNotesRequestId;

};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_MODEL2_H
