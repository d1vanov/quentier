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

#ifndef QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
#define QUENTIER_LIB_MODEL_FAVORITES_MODEL_H

#include "FavoritesModelItem.h"
#include "NoteCache.h"
#include "NotebookCache.h"
#include "TagCache.h"
#include "SavedSearchCache.h"

#include <quentier/types/Account.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/LRUCache.hpp>

#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>
#include <QHash>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>

namespace quentier {

class FavoritesModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit FavoritesModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, SavedSearchCache & savedSearchCache,
        QObject * parent = nullptr);

    virtual ~FavoritesModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    struct Columns
    {
        enum type
        {
            Type = 0,
            DisplayName,
            NumNotesTargeted
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    QModelIndex indexForLocalUid(const QString & localUid) const;
    const FavoritesModelItem * itemForLocalUid(const QString & localUid) const;
    const FavoritesModelItem * itemAtRow(const int row) const;

    /**
     * @brief allItemsListed
     * @return      True if the favorites model has received the information
     *              about all favorited notes, notebooks, tags and saved searches
     *              stored in the local storage by the moment; false otherwise
     */
    bool allItemsListed() const { return m_allItemsListed; }

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
        const QVariant & value, int role = Qt::EditRole) override;

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

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void notifyAllItemsListed();

    // Informative signals for views, so that they can prepare to the changes
    // in the table of favorited items and do some recovery after that
    void aboutToAddItem();
    void addedItem(const QModelIndex & index);

    void aboutToUpdateItem(const QModelIndex & index);
    void updatedItem(const QModelIndex & index);

    void aboutToRemoveItems();
    void removedItems();

// private signals
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

    void updateNotebook(Notebook notebook, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

    void listTags(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void findSavedSearch(SavedSearch search, QUuid requestId);

    void listSavedSearches(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

    void noteCountPerNotebook(
        Notebook notebook, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void noteCountPerTag(
        Tag tag, LocalStorageManager::NoteCountOptions options, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage

    // For notes:
    void onAddNoteComplete(Note note, QUuid requestId);

    void onUpdateNoteComplete(
        Note note,
        LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onNoteMovedToAnotherNotebook(
        QString noteLocalUid, QString previousNotebookLocalUid,
        QString newNotebookLocalUid);

    void onNoteTagListChanged(
        QString noteLocalUid, QStringList previousNoteTagLocalUids,
        QStringList newNoteTagLocalUids);

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
        QString linkedNotebookGuid, QList<Note> foundNotes, QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNoteComplete(Note note, QUuid requestId);

    // For notebooks:
    void onAddNotebookComplete(Notebook notebook, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        Notebook notebook, ErrorString errorDescription, QUuid requestId);

    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    // For tags:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);
    void onFindTagComplete(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Tag> foundTags,
        QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeTagComplete(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    // For saved searches:
    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onFindSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onFindSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onListSavedSearchesComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<SavedSearch> foundSearches, QUuid requestId);

    void onListSavedSearchesFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

    // For note counts:
    void onGetNoteCountPerNotebookComplete(
        int noteCount, Notebook notebook,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerNotebookFailed(
        ErrorString errorDescription, Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerTagComplete(
        int noteCount, Tag tag,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerTagFailed(
        ErrorString errorDescription, Tag tag,
        LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotesList();
    void requestNotebooksList();
    void requestTagsList();
    void requestSavedSearchesList();

    struct NoteCountRequestOption
    {
        enum type {
            Force = 0,
            IfNotAlreadyRunning
        };
    };

    void requestNoteCountForNotebook(const QString & notebookLocalUid,
                                     const NoteCountRequestOption::type option);
    void requestNoteCountForAllNotebooks(const NoteCountRequestOption::type option);

    void checkAndIncrementNoteCountPerNotebook(const QString & notebookLocalUid);
    void checkAndDecrementNoteCountPerNotebook(const QString & notebookLocalUid);

    void checkAndAdjustNoteCountPerNotebook(
        const QString & notebookLocalUid, const bool increment);

    void requestNoteCountForTag(
        const QString & tagLocalUid, const NoteCountRequestOption::type option);

    void requestNoteCountForAllTags(const NoteCountRequestOption::type option);

    void checkAndIncrementNoteCountPerTag(const QString & tagLocalUid);
    void checkAndDecrementNoteCountPerTag(const QString & tagLocalUid);
    void checkAndAdjustNoteCountPerTag(const QString & tagLocalUid,
                                       const bool increment);

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    void removeItemByLocalUid(const QString & localUid);
    void updateItemRowWithRespectToSorting(const FavoritesModelItem & item);
    void updateItemInLocalStorage(const FavoritesModelItem & item);
    void updateNoteInLocalStorage(const FavoritesModelItem & item);
    void updateNotebookInLocalStorage(const FavoritesModelItem & item);
    void updateTagInLocalStorage(const FavoritesModelItem & item);
    void updateSavedSearchInLocalStorage(const FavoritesModelItem & item);

    bool canUpdateNote(const QString & localUid) const;
    bool canUpdateNotebook(const QString & localUid) const;
    bool canUpdateTag(const QString & localUid) const;

    void unfavoriteNote(const QString & localUid);
    void unfavoriteNotebook(const QString & localUid);
    void unfavoriteTag(const QString & localUid);
    void unfavoriteSavedSearch(const QString & localUid);

    void onNoteAddedOrUpdated(const Note & note, const bool tagsUpdated = true);
    void onNotebookAddedOrUpdated(const Notebook & notebook);
    void onTagAddedOrUpdated(const Tag & tag);
    void onSavedSearchAddedOrUpdated(const SavedSearch & search);

    void updateItemColumnInView(const FavoritesModelItem & item,
                                const Columns::type column);

    void checkAllItemsListed();

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
                boost::multi_index::const_mem_fun<
                    FavoritesModelItem,const QString&,&FavoritesModelItem::localUid>
            >
        >
    > FavoritesData;

    typedef FavoritesData::index<ByLocalUid>::type FavoritesDataByLocalUid;
    typedef FavoritesData::index<ByIndex>::type FavoritesDataByIndex;

    struct NotebookRestrictionsData
    {
        NotebookRestrictionsData() :
            m_canUpdateNotes(false),
            m_canUpdateNotebook(false),
            m_canUpdateTags(false)
        {}

        bool    m_canUpdateNotes;
        bool    m_canUpdateNotebook;
        bool    m_canUpdateTags;
    };

    class Comparator
    {
    public:
        Comparator(const Columns::type column,
                   const Qt::SortOrder sortOrder) :
            m_sortedColumn(column),
            m_sortOrder(sortOrder)
        {}

        bool operator()(const FavoritesModelItem & lhs,
                        const FavoritesModelItem & rhs) const;

    private:
        Columns::type   m_sortedColumn;
        Qt::SortOrder   m_sortOrder;
    };

    typedef boost::bimap<QString, QUuid> LocalUidToRequestIdBimap;

private:
    Account                 m_account;
    FavoritesData           m_data;
    NoteCache &             m_noteCache;
    NotebookCache &         m_notebookCache;
    TagCache &              m_tagCache;
    SavedSearchCache &      m_savedSearchCache;

    QSet<QString>           m_lowerCaseNotebookNames;
    QSet<QString>           m_lowerCaseTagNames;
    QSet<QString>           m_lowerCaseSavedSearchNames;

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
    QSet<QUuid>             m_findNoteToUnfavoriteRequestIds;

    QSet<QUuid>             m_updateNotebookRequestIds;
    QSet<QUuid>             m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNotebookToPerformUpdateRequestIds;
    QSet<QUuid>             m_findNotebookToUnfavoriteRequestIds;

    QSet<QUuid>             m_updateTagRequestIds;
    QSet<QUuid>             m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findTagToPerformUpdateRequestIds;
    QSet<QUuid>             m_findTagToUnfavoriteRequestIds;

    QSet<QUuid>             m_updateSavedSearchRequestIds;
    QSet<QUuid>             m_findSavedSearchToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findSavedSearchToPerformUpdateRequestIds;
    QSet<QUuid>             m_findSavedSearchToUnfavoriteRequestIds;

    QHash<QString, QString> m_tagLocalUidToLinkedNotebookGuid;
    QHash<QString, QString> m_notebookLocalUidToGuid;

    QHash<QString, QString> m_notebookLocalUidByNoteLocalUid;

    LocalUidToRequestIdBimap        m_notebookLocalUidToNoteCountRequestIdBimap;
    LocalUidToRequestIdBimap        m_tagLocalUidToNoteCountRequestIdBimap;

    QHash<QString, NotebookRestrictionsData>    m_notebookRestrictionsData;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    bool                    m_allItemsListed;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
