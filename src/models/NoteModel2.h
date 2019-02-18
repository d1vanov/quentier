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

    struct NoteSortingModes
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
        enum type {
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

    explicit NoteModel2(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        QObject * parent = Q_NULLPTR,
        const IncludedNotes::type includedNotes = IncludedNotes::NonDeleted,
        const NoteSortingModes::type noteSortingModes = NoteSortingModes::None);

    virtual ~NoteModel2();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    QModelIndex indexForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemForLocalUid(const QString & localUid) const;
    const NoteModelItem * itemAtRow(const int row) const;
    const NoteModelItem * itemForIndex(const QModelIndex & index) const;

public:
    // Note filtering API

    bool hasFilters() const;

    const QStringList & filteredNotebookLocalUids() const
    { return m_filteredNotebookLocalUids; }

    void setFilteredNotebookLocalUids(const QStringList & notebookLocalUids);

    const QStringList & filteredTagLocalUids() const
    { return m_filteredTagLocalUids; }

    void setFilteredTagLocalUids(const QStringList & tagLocalUids);

    const QStringList & filteredNoteLocalUids() const
    { return m_filteredNoteLocalUids; }

    void setFilteredNoteLocalUids(const QStringList & noteLocalUids);

    void beginUpdateFilter();
    void endUpdateFilter();

    /**
     * @brief Total number of notes confrming to the specified filters
     * within the local storage database (not necessarily equal to the number
     * of notes loaded by the model - it's typically smaller due to lazy
     * loading)
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
    void expungeNote(Note note, QUuid requestId);

    void findNotebook(Notebook notebook, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

private:
    Account                 m_account;
    IncludedNotes::type     m_includedNotes;
    NoteSortingModes::type  m_noteSortingModes;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    QStringList             m_filteredNotebookLocalUids;
    QStringList             m_filteredTagLocalUids;
    QStringList             m_filteredNoteLocalUids;

    QStringList             m_updatedFilteredNotebookLocalUids;
    QStringList             m_updatedFilteredTagLocalUids;
    QStringList             m_updatedFilteredNoteLocalUids;

    bool                    m_updatingFilters;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_MODEL2_H
