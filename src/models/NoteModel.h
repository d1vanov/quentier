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

#ifndef QUENTIER_MODELS_NOTE_MODEL_H
#define QUENTIER_MODELS_NOTE_MODEL_H

#include "NoteModelItem.h"
#include "NoteCache.h"
#include "NotebookCache.h"
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QMultiHash>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>
#endif

namespace quentier {

class NoteModel: public QAbstractItemModel
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

    explicit NoteModel(const Account & account,  LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                       NoteCache & noteCache, NotebookCache & notebookCache, QObject * parent = Q_NULLPTR,
                       const IncludedNotes::type includedNotes = IncludedNotes::NonDeleted);
    virtual ~NoteModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    struct Columns
    {
        enum type {
            CreationTimestamp = 0,
            ModificationTimestamp,
            DeletionTimestamp,
            Title,
            PreviewText,
            ThumbnailImageFilePath,
            NotebookName,
            TagNameList,
            Size,
            Synchronizable,
            Dirty
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    QModelIndex indexForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemAtRow(const int row) const;
    const NoteModelItem * itemForIndex(const QModelIndex & index) const;

    /**
     * @brief createNoteItem - attempts to create a new note within the notebook specified by local uid
     * @param notebookLocalUid - the local uid of notebook in which the new note is to be created
     * @return the model index of the newly created note item or invalid modex index if the new note could not be
     * created
     */
    QModelIndex createNoteItem(const QString & notebookLocalUid);

    bool allNotesListed() const { return m_allNotesListed; }

    /**
     * @brief deleteNote - attempts to mark the note with the specified local uid as deleted.
     *
     * The model only looks at the notes contained in it, so if the model was set up to include only
     * already deleted notes or only non-deleted ones, there's a chance the model wan't contain the note
     * pointed to by the local uid. In that case the deletion won't be successful.
     *
     * @param noteLocalUid - the local uid of note to be marked as deleted
     * @return true if the note was deleted successfully, false otherwise
     */
    bool deleteNote(const QString & noteLocalUid);

    /**
     * @brief moveNoteToNotebook - attempts to move the note to a different notebook
     *
     * The method doesn't have a return code because it might have to do its job asynchronously;
     * if the error happens during the process (for example, the target notebook was not found by name),
     * @link notifyError @endlink signal is emitted
     *
     * @param noteLocalUid - the local uid of note to be moved to another notebook
     * @param notebookName - the name of the notebook into which the note needs to be moved
     */
    void moveNoteToNotebook(const QString & noteLocalUid, const QString & notebookName);

    /**
     * @brief favoriteNote - attempts to mark the note with the specified local uid as favorited
     *
     * Favorited property of @link Note @endlink class is not represented as a column within
     * the @link NoteModel @endlink so this method doesn't change anything in the model but only
     * the underlying note object persisted in the local storage
     *
     * @param noteLocalUid - the local uid of the note to be favorited
     */
    void favoriteNote(const QString & noteLocalUid);

    /**
     * @brief unfavoriteNote - attempts to remove the favorited mark from the note with the specified local uid
     *
     * Favorited property of @link Note @endlink class is not represented as a column within
     * the @link NoteModel @endlink so this method doesn't change anything in the model but only
     * the underlying note object persisted in the local storage
     *
     * @param noteLocalUid - the local uid of the note to be unfavorited
     */
    void unfavoriteNote(const QString & noteLocalUid);

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
    void notifyError(ErrorString errorDescription);

    void notifyAllNotesListed();

// private signals
    void addNote(Note note, QUuid requestId);
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void listNotes(LocalStorageManager::ListObjectsOptions flag,
                   bool withResourceBinaryData, size_t limit, size_t offset,
                   LocalStorageManager::ListNotesOrder::type order,
                   LocalStorageManager::OrderDirection::type orderDirection,
                   QUuid requestId);
    void expungeNote(Note note, QUuid requestId);

    void findNotebook(Notebook notebook, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, ErrorString errorDescription, QUuid requestId);
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            ErrorString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId);
    void onFindNoteFailed(Note note, bool withResourceBinaryData, ErrorString errorDescription, QUuid requestId);
    void onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                             size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                             LocalStorageManager::OrderDirection::type orderDirection,
                             QList<Note> foundNotes, QUuid requestId);
    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                           size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           ErrorString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);
    void onExpungeNoteFailed(Note note, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, ErrorString errorDescription, QUuid requestId);
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestNotesList();

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    void removeItemByLocalUid(const QString & localUid);
    void updateItemRowWithRespectToSorting(const NoteModelItem & item);
    void updateNoteInLocalStorage(const NoteModelItem & item, const bool updateTags = false);

    // Returns the appropriate row before which the new item should be inserted according to the current sorting criteria and column
    int rowForNewItem(const NoteModelItem & newItem) const;

    bool canUpdateNoteItem(const NoteModelItem & item) const;
    bool canCreateNoteItem(const QString & notebookLocalUid) const;
    void updateNotebookData(const Notebook & notebook);

    void updateTagData(const Tag & tag);

    void setNoteFavorited(const QString & noteLocalUid, const bool favorited);

private:
    struct ByLocalUid{};
    struct ByIndex{};

    typedef boost::multi_index_container<
        NoteModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<NoteModelItem,const QString&,&NoteModelItem::localUid>
            >
        >
    > NoteData;

    typedef NoteData::index<ByLocalUid>::type NoteDataByLocalUid;
    typedef NoteData::index<ByIndex>::type NoteDataByIndex;

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
    void onNoteAddedOrUpdated(const Note & note);
    void noteToItem(const Note & note, NoteModelItem & item);
    void checkAddedNoteItemsPendingNotebookData(const QString & notebookLocalUid, const NotebookData & notebookData);
    void addOrUpdateNoteItem(NoteModelItem & item, const NotebookData & notebookData);

    void moveNoteToNotebookImpl(NoteDataByLocalUid::iterator it, const Notebook & notebook);

    void checkAndNotifyAllNotesListed();

private:
    Account                 m_account;
    IncludedNotes::type     m_includedNotes;
    NoteData                m_data;
    size_t                  m_listNotesOffset;
    QUuid                   m_listNotesRequestId;
    QSet<QUuid>             m_noteItemsNotYetInLocalStorageUids;

    NoteCache &             m_cache;
    NotebookCache &         m_notebookCache;

    // NOTE: it would only corresond to m_data.size() if m_includedNotes == IncludedNotes::All
    qint32                  m_numberOfNotesPerAccount;

    QSet<QUuid>             m_addNoteRequestIds;
    QSet<QUuid>             m_updateNoteRequestIds;
    QSet<QUuid>             m_expungeNoteRequestIds;

    QSet<QUuid>             m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNoteToPerformUpdateRequestIds;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    QHash<QString, NotebookData>        m_notebookDataByNotebookLocalUid;
    LocalUidToRequestIdBimap            m_findNotebookRequestForNotebookLocalUid;
    QMultiHash<QString, NoteModelItem>  m_noteItemsPendingNotebookDataUpdate;   // The key is notebook local uid

    LocalUidToRequestIdBimap            m_noteLocalUidToFindNotebookRequestIdForMoveNoteToNotebookBimap;

    QHash<QString, TagData>             m_tagDataByTagLocalUid;
    LocalUidToRequestIdBimap            m_findTagRequestForTagLocalUid;
    QMultiHash<QString, QString>        m_tagLocalUidToNoteLocalUid;

    bool                    m_allNotesListed;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_MODEL_H
