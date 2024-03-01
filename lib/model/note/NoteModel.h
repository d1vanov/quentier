/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include "NoteCache.h"
#include "NoteModelItem.h"

#include <lib/model/notebook/NotebookCache.h>
#include <lib/utility/IStartable.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/SuppressWarnings.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <qevercloud/types/Fwd.h>

#include <QAbstractItemModel>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

#include <optional>

namespace quentier {

class NoteModel : public QAbstractItemModel, public IStartable
{
    Q_OBJECT
public:
    enum class IncludedNotes
    {
        All = 0,
        NonDeleted,
        Deleted,
    };

    friend QDebug & operator<<(QDebug & dbg, IncludedNotes includedNotes);

    enum class NoteSortingMode
    {
        CreatedAscending = 0,
        CreatedDescending,
        ModifiedAscending,
        ModifiedDescending,
        TitleAscending,
        TitleDescending,
        SizeAscending,
        SizeDescending,
        None,
    };

    friend QDebug & operator<<(QDebug & dbg, NoteSortingMode mode);

    enum class Column
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

    friend QDebug & operator<<(QDebug & dbg, Column column);

    class NoteFilters
    {
    public:
        [[nodiscard]] bool isEmpty() const noexcept;

        [[nodiscard]] const QStringList & filteredNotebookLocalIds()
            const noexcept;

        void setFilteredNotebookLocalIds(QStringList notebookLocalIds) noexcept;
        void clearFilteredNotebookLocalIds();

        [[nodiscard]] const QStringList & filteredTagLocalIds() const noexcept;
        void setFilteredTagLocalIds(QStringList tagLocalIds) noexcept;
        void clearFilteredTagLocalIds();

        [[nodiscard]] const QSet<QString> & filteredNoteLocalIds()
            const noexcept;

        bool setFilteredNoteLocalIds(QSet<QString> noteLocalIds) noexcept;
        bool setFilteredNoteLocalIds(const QStringList & noteLocalIds);
        void clearFilteredNoteLocalIds();

    private:
        QStringList m_filteredNotebookLocalIds;
        QStringList m_filteredTagLocalIds;
        QSet<QString> m_filteredNoteLocalIds;
    };

    NoteModel(
        Account account,
        local_storage::ILocalStoragePtr localStorage,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent = nullptr,
        IncludedNotes includedNotes = IncludedNotes::NonDeleted,
        NoteSortingMode noteSortingMode = NoteSortingMode::ModifiedAscending,
        std::optional<NoteFilters> filters = std::nullopt);

    ~NoteModel() override;

    [[nodiscard]] const Account & account() const noexcept
    {
        return m_account;
    }

    void updateAccount(Account account);

    [[nodiscard]] Column sortingColumn() const noexcept;
    [[nodiscard]] Qt::SortOrder sortOrder() const noexcept;

    [[nodiscard]] QModelIndex indexForLocalId(const QString & localId) const;
    [[nodiscard]] const NoteModelItem * itemForLocalId(
        const QString & localId) const;

    [[nodiscard]] const NoteModelItem * itemAtRow(const int row) const;
    [[nodiscard]] const NoteModelItem * itemForIndex(
        const QModelIndex & index) const;

    [[nodiscard]] bool isMinimalNotesBatchLoaded() const noexcept;

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
    [[nodiscard]] qint32 totalFilteredNotesCount() const noexcept;

    /**
     * @brief Total number of notes per account within the local storage
     * database not considering the filters set to the note model
     */
    [[nodiscard]] qint32 totalAccountNotesCount() const noexcept;

public:
    /**
     * @brief createNoteItem - attempts to create a new note within the notebook
     * specified by local uid
     * @param notebookLocalId      The local uid of notebook in which the new
     *                              note is to be created
     * @param errorDescription      Textual description of the error in case
     *                              the new note cannot be created
     * @return                      The model index of the newly created note
     *                              item or invalid modex index if the new note
     *                              could not be created
     */
    [[nodiscard]] QModelIndex createNoteItem(
        const QString & notebookLocalId, ErrorString & erroDescription);

    /**
     * @brief deleteNote - attempts to mark the note with the specified
     * local uid as deleted.
     *
     * The model only looks at the notes contained in it, so if the model was
     * set up to include only already deleted notes or only non-deleted ones,
     * there's a chance the model wan't contain the note pointed to by the local
     * uid. In that case the deletion won't be successful.
     *
     * @param noteLocalId          The local uid of note to be marked as
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
     * @param noteLocalId          The local uid of note to be moved to another
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
     * @param noteLocalId          The local uid of the note to be favorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be favorited
     * @return                      True if the note was favorited successfully,
     *                              false otherwise
     */
    [[nodiscard]] bool favoriteNote(
        const QString & noteLocalId, ErrorString & errorDescription);

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
     * @param noteLocalId          The local uid of the note to be unfavorited
     * @param errorDescription      Textual description of the error if the note
     *                              could not be unfavorited
     * @return                      True if the note was unfavorited
     *                              successfully, false otherwise
     */
    [[nodiscard]] bool unfavoriteNote(
        const QString & noteLocalId, ErrorString & errorDescription);

public:
    // QAbstractItemModel interface
    Qt::ItemFlags flags(const QModelIndex & modelIndex) const override;

    QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    int rowCount(
        const QModelIndex & parent = QModelIndex()) const override;

    int columnCount(
        const QModelIndex & parent = QModelIndex()) const override;

    QModelIndex index(
        int row, int column,
        const QModelIndex & parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex & index) const override;

    bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    bool insertRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    bool removeRows(
        int row, int count,
        const QModelIndex & parent = QModelIndex()) override;

    void sort(int column, Qt::SortOrder order) override;

    bool canFetchMore(const QModelIndex & parent) const override;
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

private:
    void connectToLocalStorageEvents(
        local_storage::ILocalStorageNotifier * notifier);

    void disconnectFromLocalStorageEvents();

    enum class NoteSource
    {
        Listing,
        Event
    };

    friend QDebug & operator<<(QDebug & dbg, NoteSource noteSource);

    void onNotePut(const qevercloud::Note & note);
    void onNoteAdded(const qevercloud::Note & note);

    enum class NoteUpdate
    {
        WithTags,
        WithoutTags
    };

    friend QDebug & operator<<(QDebug & dbg, NoteUpdate noteUpdate);

    void onNoteUpdated(qevercloud::Note note, NoteUpdate noteUpdate);

    void onNoteAddedOrUpdated(
        const qevercloud::Note & note,
        NoteSource noteSource = NoteSource::Event);

    void noteToItem(const qevercloud::Note & note, NoteModelItem & item);
    bool noteConformsToFilter(const qevercloud::Note & note) const;
    void onListNotesCompleteImpl(const QList<qevercloud::Note> & foundNotes);

    void requestNotesListAndCount();
    void requestNotesList();
    void requestNotesCount();
    void requestTotalNotesCountPerAccount();
    void requestTotalFilteredNotesCount();

    void findNoteToRestoreFailedUpdate(const QString & noteLocalId);

    void clearModel();
    void resetModel();

    /**
     * @param newItem       New note model item about to be inserted into
     *                      the model
     * @return              The appropriate row before which the new item should
     *                      be inserted according to the current sorting
     *                      criteria and column
     */
    int rowForNewItem(const NoteModelItem & newItem) const;

    void processTagExpunging(const QString & tagLocalId);

    void removeItemByLocalId(const QString & localId);

    bool updateItemRowWithRespectToSorting(
        const NoteModelItem & item, ErrorString & errorDescription);

    void saveNoteInLocalStorage(
        const NoteModelItem & item, bool saveTags = false);

    QVariant dataImpl(int row, Column column) const;

    QVariant dataAccessibleText(
        int row, const Column column) const;

    bool setDataImpl(
        const QModelIndex & index, const QVariant & value,
        ErrorString & errorDescription);

    bool removeRowsImpl(int row, int count, ErrorString & errorDescription);

    bool canUpdateNoteItem(const NoteModelItem & item) const;
    bool canCreateNoteItem(const QString & notebookLocalId) const;
    void updateNotebookData(const qevercloud::Notebook & notebook);

    bool setNoteFavorited(
        const QString & noteLocalId, bool favorited,
        ErrorString & errorDescription);

    void setSortingColumnAndOrder(int column, Qt::SortOrder order);
    void setSortingOrder(Qt::SortOrder order);

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

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
        NoteComparator(
            const Column column, const Qt::SortOrder sortOrder) :
            m_sortedColumn{column},
            m_sortOrder{sortOrder}
        {}

        [[nodiscard]] bool operator()(
            const NoteModelItem & lhs, const NoteModelItem & rhs) const;

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

private:
    // WARNING: this method assumes the iterator passed to it is not end()
    bool moveNoteToNotebookImpl(
        NoteDataByLocalId::iterator it, const qevercloud::Notebook & notebook,
        ErrorString & errorDescription);

    void addOrUpdateNoteItem(
        NoteModelItem & item, const NotebookData & notebookData,
        NoteSource noteSource);

    void checkMaxNoteCountAndRemoveLastNoteIfNeeded();

    void checkAddedNoteItemsPendingNotebookData(
        const QString & notebookLocalId, const NotebookData & notebookData);

    void findTagNamesForItem(NoteModelItem & item);

    void updateTagData(const qevercloud::Tag & tag);

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    const IncludedNotes m_includedNotes;

    Account m_account;
    NoteSortingMode m_noteSortingMode;

    bool m_connectedToLocalStorage = false;
    bool m_isStarted = false;
    bool m_minimalNotesBatchLoaded = false;

    NoteData m_data;
    qint32 m_totalFilteredNotesCount = 0;

    NoteCache & m_cache;
    NotebookCache & m_notebookCache;

    std::optional<NoteFilters> m_filters;
    std::optional<NoteFilters> m_updatedNoteFilters;

    bool m_pendingFullNoteCountPerAccount = false;
    bool m_pendingNoteCount = false;
    bool m_pendingNotesList = false;
    QSet<QString> m_pendingTagLocalIds;

    utility::cancelers::ManualCancelerPtr m_canceler;

    // Upper bound for the amount of notes stored within the note model.
    // Can be increased through calls to fetchMore()
    quint64 m_maxNoteCount;

    quint64 m_listNotesOffset = 0;

    qint32 m_totalAccountNotesCount = 0;

    QHash<QString, NotebookData> m_notebookDataByNotebookLocalId;

    QSet<QString> m_localIdsOfNewNotesBeingAddedToLocalStorage;
    QSet<QString> m_notebookLocalIdsPendingFindingInLocalStorage;

    // The key is notebook local id
    QMultiHash<QString, NoteModelItem> m_noteItemsPendingNotebookDataUpdate;

    QHash<QString, TagData> m_tagDataByTagLocalId;

    QMultiHash<QString, QString> m_tagLocalIdToNoteLocalId;
};

} // namespace quentier
