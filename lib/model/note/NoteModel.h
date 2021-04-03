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

#ifndef QUENTIER_LIB_MODEL_NOTE_MODEL_H
#define QUENTIER_LIB_MODEL_NOTE_MODEL_H

#include "NoteCache.h"
#include "NoteModelItem.h"

#include <lib/model/notebook/NotebookCache.h>
#include <lib/utility/IStartable.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/utility/SuppressWarnings.h>

#include <QAbstractItemModel>

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>

RESTORE_WARNINGS

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

#include <memory>

class QDebug;

namespace quentier {

class NoteModel final : public QAbstractItemModel, public IStartable
{
    Q_OBJECT
public:
    enum class IncludedNotes
    {
        All,
        NonDeleted,
        Deleted
    };

    friend QDebug & operator<<(QDebug & dbg, IncludedNotes includedNotes);

    enum class NoteSortingMode
    {
        CreatedAscending,
        CreatedDescending,
        ModifiedAscending,
        ModifiedDescending,
        TitleAscending,
        TitleDescending,
        SizeAscending,
        SizeDescending,
        None
    };

    friend QDebug & operator<<(QDebug & dbg, NoteSortingMode mode);

    enum class Column
    {
        CreationTimestamp,
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

    friend QDebug & operator<<(QDebug & dbg, Column column);

    class NoteFilters
    {
    public:
        NoteFilters() = default;

        [[nodiscard]] bool isEmpty() const noexcept;

        [[nodiscard]] const QStringList & filteredNotebookLocalIds() const
            noexcept;

        void setFilteredNotebookLocalIds(QStringList notebookLocalIds);
        void clearFilteredNotebookLocalIds();

        [[nodiscard]] const QStringList & filteredTagLocalIds() const noexcept;
        void setFilteredTagLocalIds(QStringList tagLocalIds);
        void clearFilteredTagLocalIds();

        [[nodiscard]] const QSet<QString> & filteredNoteLocalIds() const
            noexcept;

        [[nodiscard]] bool setFilteredNoteLocalIds(
            QSet<QString> noteLocalIds);

        [[nodiscard]] bool setFilteredNoteLocalIds(
            const QStringList & noteLocalIds);

        void clearFilteredNoteLocalIds();

    private:
        Q_DISABLE_COPY(NoteFilters);

    private:
        QStringList m_filteredNotebookLocalIds;
        QStringList m_filteredTagLocalIds;
        QSet<QString> m_filteredNoteLocalIds;
    };

    explicit NoteModel(
        Account account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent = nullptr,
        IncludedNotes includedNotes = IncludedNotes::NonDeleted,
        NoteSortingMode noteSortingMode = NoteSortingMode::ModifiedAscending,
        NoteFilters * pFilters = nullptr);

    ~NoteModel() override;

    [[nodiscard]] const Account & account() const noexcept
    {
        return m_account;
    }

    void setAccount(const Account & account);

    [[nodiscard]] Column sortingColumn() const noexcept;
    [[nodiscard]] Qt::SortOrder sortOrder() const noexcept;

    [[nodiscard]] QModelIndex indexForLocalId(const QString & localId) const;

    [[nodiscard]] const NoteModelItem * itemForLocalId(
        const QString & localId) const;

    [[nodiscard]] const NoteModelItem * itemAtRow(int row) const;

    [[nodiscard]] const NoteModelItem * itemForIndex(
        const QModelIndex & index) const;

public:
    // Note filtering API

    [[nodiscard]] bool hasFilters() const noexcept;

    [[nodiscard]] const QStringList & filteredNotebookLocalIds() const noexcept;
    void setFilteredNotebookLocalIds(QStringList notebookLocalIds);
    void clearFilteredNotebookLocalIds();

    [[nodiscard]] const QStringList & filteredTagLocalIds() const noexcept;
    void setFilteredTagLocalIds(QStringList tagLocalIds);
    void clearFilteredTagLocalIds();

    [[nodiscard]] const QSet<QString> & filteredNoteLocalIds() const noexcept;
    void setFilteredNoteLocalIds(QSet<QString> noteLocalIds);
    void setFilteredNoteLocalIds(const QStringList & noteLocalIds);
    void clearFilteredNoteLocalIds();

    void beginUpdateFilter();
    void endUpdateFilter();

    /**
     * @brief Total number of notes conforming with the specified filters
     * within the local storage database (not necessarily equal to the number
     * of rows within the model - it's typically smaller due to lazy loading)
     */
    [[nodiscard]] qint32 totalFilteredNotesCount() const;

    /**
     * @brief Total number of notes per account within the local storage
     * database not considering the filters set to the note model
     */
    [[nodiscard]] qint32 totalAccountNotesCount() const;

public:
    /**
     * @brief createNoteItem - attempts to create a new note within the notebook
     * specified by local id
     * @param notebookLocalId       The local id of notebook in which the new
     *                              note is to be created
     * @param errorDescription      Textual description of the error in case
     *                              the new note cannot be created
     * @return                      The model index of the newly created note
     *                              item or invalid modex index if the new note
     *                              could not be created
     */
    [[nodiscard]] QModelIndex createNoteItem(
        const QString & notebookLocalId, ErrorString & errorDescription);

    /**
     * @brief deleteNote - attempts to mark the note with the specified
     * local id as deleted.
     *
     * The model only looks at the notes contained in it, so if the model was
     * set up to include only already deleted notes or only non-deleted ones,
     * there's a chance the model wan't contain the note pointed to by the local
     * id. In that case the deletion won't be successful.
     *
     * @param noteLocalId           The local id of note to be marked as
     *                              deleted
     * @param errorDescription      Textual description of the error if the note
     *                              could not be marked as deleted
     * @return                      True if the note was deleted successfully,
     *                              false otherwise
     */
    [[nodiscard]] bool deleteNote(
        const QString & noteLocalId, ErrorString & errorDescription);

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
     * @param noteLocalId           The local id of note to be moved to another
     *                              notebook
     * @param notebookName          The name of the notebook into which the note
     *                              needs to be moved
     * @param errorDescription      Textual description of the error if the note
     *                              could not be moved to the specified notebook
     * @return                      True if the note was moved to the specified
     *                              notebook successfully, false otherwise
     */
    [[nodiscard]] bool moveNoteToNotebook(
        const QString & noteLocalId, const QString & notebookName,
        ErrorString & errorDescription);

    /**
     * @brief favoriteNote - attempts to mark the note with the specified local
     * id as favorited
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
     * @param noteLocalId           The local id of the note to be favorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be favorited
     * @return                      True if the note was favorited successfully,
     *                              false otherwise
     */
    [[nodiscard]] bool favoriteNote(
        const QString & noteLocalId, ErrorString & errorDescription);

    /**
     * @brief unfavoriteNote - attempts to remove the favorited mark from
     * the note with the specified local id
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
     * @param noteLocalId           The local id of the note to be unfavorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be unfavorited
     * @return                      True if the note was unfavorited
     *                              successfully, false otherwise
     */
    [[nodiscard]] bool unfavoriteNote(
        const QString & noteLocalId, ErrorString & errorDescription);

public:
    // QAbstractItemModel interface
    [[nodiscard]] Qt::ItemFlags flags(
        const QModelIndex & modelIndex) const override;

    [[nodiscard]] QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    [[nodiscard]] int rowCount(
        const QModelIndex & parent = QModelIndex()) const override;

    [[nodiscard]] int columnCount(
        const QModelIndex & parent = QModelIndex()) const override;

    [[nodiscard]] QModelIndex index(
        int row, int column,
        const QModelIndex & parent = QModelIndex()) const override;

    [[nodiscard]] QModelIndex parent(const QModelIndex & index) const override;

    [[nodiscard]] bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool insertRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    [[nodiscard]] bool removeRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder order) override;

    [[nodiscard]] bool canFetchMore(const QModelIndex & parent) const override;
    void fetchMore(const QModelIndex & parent) override;

    // IStartable interface
    void start() override;

    [[nodiscard]] bool isStarted() const noexcept override
    {
        return m_isStarted;
    }

    void stop(StopMode stopMode) override;

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
    void addNote(qevercloud::Note note, QUuid requestId);

    void updateNote(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void findNote(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void listNotes(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void listNotesPerNotebooksAndTags(
        QStringList notebookLocalIds, QStringList tagLocalIds,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void listNotesByLocalIds(
        QStringList noteLocalIds, LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void getNoteCount(
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void getNoteCountPerNotebooksAndTags(
        QStringList notebookLocalIds, QStringList tagLocalIds,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void expungeNote(qevercloud::Note note, QUuid requestId);

    void findNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void findTag(qevercloud::Tag tag, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onUpdateNoteFailed(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onFindNoteComplete(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        QUuid requestId);

    void onFindNoteFailed(
        qevercloud::Note note, LocalStorageManager::GetNoteOptions options,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesComplete(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Note> foundNotes,
        QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onListNotesPerNotebooksAndTagsComplete(
        QStringList notebookLocalIds, QStringList tagLocalIds,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<qevercloud::Note> foundNotes, QUuid requestId);

    void onListNotesPerNotebooksAndTagsFailed(
        QStringList notebookLocalIds, QStringList tagLocalIds,
        LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onListNotesByLocalIdsComplete(
        QStringList noteLocalIds, LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<qevercloud::Note> foundNotes, QUuid requestId);

    void onListNotesByLocalIdsFailed(
        QStringList noteLocalIds, LocalStorageManager::GetNoteOptions options,
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onGetNoteCountComplete(
        int noteCount, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountFailed(
        ErrorString errorDescription,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebooksAndTagsComplete(
        int noteCount, QStringList notebookLocalIds, QStringList tagLocalIds,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebooksAndTagsFailed(
        ErrorString errorDescription, QStringList notebookLocalIds,
        QStringList tagLocalIds, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    void onExpungeNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onFindTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onAddTagComplete(qevercloud::Tag tag, QUuid requestId);
    void onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

private:
    void connectToLocalStorage();
    void disconnectFromLocalStorage();

    void onNoteAddedOrUpdated(
        const qevercloud::Note & note, bool fromNotesListing = false);

    void noteToItem(const qevercloud::Note & note, NoteModelItem & item);

    [[nodiscard]] bool noteConformsToFilter(
        const qevercloud::Note & note) const;

    void onListNotesCompleteImpl(QList<qevercloud::Note> foundNotes);

    void requestNotesListAndCount();
    void requestNotesList();
    void requestNotesCount();
    void requestTotalNotesCountPerAccount();
    void requestTotalFilteredNotesCount();

    void findNoteToRestoreFailedUpdate(const qevercloud::Note & note);

    void clearModel();
    void resetModel();

    [[nodiscard]] LocalStorageManager::NoteCountOptions noteCountOptions()
        const;

    /**
     * @param newItem       New note model item about to be inserted into
     *                      the model
     * @return              The appropriate row before which the new item should
     *                      be inserted according to the current sorting
     *                      criteria and column
     */
    [[nodiscard]] int rowForNewItem(const NoteModelItem & newItem) const;

    void processTagExpunging(const QString & tagLocalId);

    void removeItemByLocalId(const QString & localId);

    [[nodiscard]] bool updateItemRowWithRespectToSorting(
        const NoteModelItem & item, ErrorString & errorDescription);

    void saveNoteInLocalStorage(
        const NoteModelItem & item, bool saveTags = false);

    [[nodiscard]] QVariant dataImpl(int row, Column column) const;

    [[nodiscard]] QVariant dataAccessibleText(int row, Column column) const;

    [[nodiscard]] bool setDataImpl(
        const QModelIndex & index, const QVariant & value,
        ErrorString & errorDescription);

    [[nodiscard]] bool removeRowsImpl(
        int row, int count, ErrorString & errorDescription);

    [[nodiscard]] bool canUpdateNoteItem(const NoteModelItem & item) const;
    [[nodiscard]] bool canCreateNoteItem(const QString & notebookLocalId) const;
    void updateNotebookData(const qevercloud::Notebook & notebook);

    [[nodiscard]] bool setNoteFavorited(
        const QString & noteLocalId, bool favorited,
        ErrorString & errorDescription);

    void setSortingColumnAndOrder(int column, Qt::SortOrder order);
    void setSortingOrder(Qt::SortOrder order);

private:
    struct ByIndex
    {};

    struct ByLocalId
    {};

    struct ByNotebookLocalId
    {};

    using NoteData = boost::multi_index_container<
        NoteModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<boost::multi_index::tag<ByIndex>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalId>,
                boost::multi_index::const_mem_fun<
                    NoteModelItem, const QString &, &NoteModelItem::localId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByNotebookLocalId>,
                boost::multi_index::const_mem_fun<
                    NoteModelItem, const QString &,
                    &NoteModelItem::notebookLocalId>>>>;

    using NoteDataByIndex = NoteData::index<ByIndex>::type;
    using NoteDataByLocalId = NoteData::index<ByLocalId>::type;

    using NoteDataByNotebookLocalId =
        NoteData::index<ByNotebookLocalId>::type;

    class NoteComparator
    {
    public:
        NoteComparator(Column column, Qt::SortOrder sortOrder) :
            m_sortedColumn(column),
            m_sortOrder(sortOrder)
        {}

        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs,
            const NoteModelItem & rhs) const noexcept;

    private:
        Column m_sortedColumn;
        Qt::SortOrder m_sortOrder;
    };

    struct NotebookData
    {
        QString m_name;
        QString m_guid;
        bool m_canCreateNotes = false;
        bool m_canUpdateNotes = false;
    };

    struct TagData
    {
        QString m_name;
        QString m_guid;
    };

    using LocalIdToRequestIdBimap = boost::bimap<QString, QUuid>;

private:
    // WARNING: this method assumes the iterator passed to it is not end()
    [[nodiscard]] bool moveNoteToNotebookImpl(
        NoteDataByLocalId::iterator it, const qevercloud::Notebook & notebook,
        ErrorString & errorDescription);

    void addOrUpdateNoteItem(
        NoteModelItem & item, const NotebookData & notebookData,
        bool fromNotesListing);

    void checkMaxNoteCountAndRemoveLastNoteIfNeeded();

    void checkAddedNoteItemsPendingNotebookData(
        const QString & notebookLocalId, const NotebookData & notebookData);

    void findTagNamesForItem(NoteModelItem & item);

    void updateTagData(const qevercloud::Tag & tag);

private:
    Account m_account;
    const IncludedNotes m_includedNotes;
    NoteSortingMode m_noteSortingMode;

    LocalStorageManagerAsync & m_localStorageManagerAsync;
    bool m_connectedToLocalStorage = false;

    bool m_isStarted = false;

    NoteData m_data;
    qint32 m_totalFilteredNotesCount = 0;

    NoteCache & m_cache;
    NotebookCache & m_notebookCache;

    std::unique_ptr<NoteFilters> m_pFilters;
    std::unique_ptr<NoteFilters> m_pUpdatedNoteFilters;

    // Upper bound for the amount of notes stored within the note model.
    // Can be increased through calls to fetchMore()
    size_t m_maxNoteCount;

    size_t m_listNotesOffset = 0;
    QUuid m_listNotesRequestId;
    QUuid m_getNoteCountRequestId;

    qint32 m_totalAccountNotesCount = 0;
    QUuid m_getFullNoteCountPerAccountRequestId;

    QHash<QString, NotebookData> m_notebookDataByNotebookLocalId;
    LocalIdToRequestIdBimap m_findNotebookRequestForNotebookLocalId;

    QSet<QUuid> m_localIdsOfNewNotesBeingAddedToLocalStorage;

    QSet<QUuid> m_addNoteRequestIds;
    QSet<QUuid> m_updateNoteRequestIds;
    QSet<QUuid> m_expungeNoteRequestIds;

    QSet<QUuid> m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findNoteToPerformUpdateRequestIds;

    // The key is notebook local uid
    QMultiHash<QString, NoteModelItem> m_noteItemsPendingNotebookDataUpdate;

    LocalIdToRequestIdBimap
        m_noteLocalIdToFindNotebookRequestIdForMoveNoteToNotebookBimap;

    QHash<QString, TagData> m_tagDataByTagLocalId;

    LocalIdToRequestIdBimap m_findTagRequestForTagLocalId;
    QMultiHash<QString, QString> m_tagLocalIdToNoteLocalId;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTE_MODEL_H
