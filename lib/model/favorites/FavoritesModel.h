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

#ifndef QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
#define QUENTIER_LIB_MODEL_FAVORITES_MODEL_H

#include "FavoritesModelItem.h"

#include <lib/model/common/ItemModel.h>
#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/saved_search/SavedSearchCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/types/Tag.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QSet>
#include <QUuid>

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class FavoritesModel final: public ItemModel
{
    Q_OBJECT
public:
    explicit FavoritesModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, SavedSearchCache & savedSearchCache,
        QObject * parent = nullptr);

    virtual ~FavoritesModel() override;

    enum class Column
    {
        Type = 0,
        DisplayName,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, const Column column);

    const FavoritesModelItem * itemForLocalUid(const QString & localUid) const;

    const FavoritesModelItem * itemAtRow(const int row) const;

public:
    // ItemModel interface
    virtual QString localUidForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's clients
        Q_UNUSED(itemName)
        Q_UNUSED(linkedNotebookGuid)
        return {};
    }

    virtual QModelIndex indexForLocalUid(
        const QString & localUid) const override;

    virtual QString itemNameForLocalUid(
        const QString & localUid) const override;

    virtual ItemInfo itemInfoForLocalUid(
        const QString & localUid) const override;

    virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        Q_UNUSED(linkedNotebookGuid);
        return {};
    }

    virtual QVector<LinkedNotebookInfo> linkedNotebooksInfo() const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        return {};
    }

    virtual QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        Q_UNUSED(linkedNotebookGuid);
        return {};
    }

    virtual int nameColumn() const override
    {
        return static_cast<int>(Column::DisplayName);
    }

    virtual int sortingColumn() const override
    {
        return static_cast<int>(m_sortedColumn);
    }

    virtual Qt::SortOrder sortOrder() const override
    {
        return m_sortOrder;
    }

    virtual bool allItemsListed() const override
    {
        return m_allItemsListed;
    }

    virtual QString localUidForItemIndex(
        const QModelIndex & index) const override;

    virtual QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override
    {
        Q_UNUSED(index)
        return {};
    }

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    virtual QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

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

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

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
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateNotebook(Notebook notebook, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateTag(Tag tag, QUuid requestId);
    void findTag(Tag tag, QUuid requestId);

    void listTags(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void findSavedSearch(SavedSearch search, QUuid requestId);

    void listSavedSearches(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void noteCountPerNotebook(
        Notebook notebook, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void noteCountPerTag(
        Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage

    // For notes:
    void onAddNoteComplete(Note note, QUuid requestId);

    void onUpdateNoteComplete(
        Note note, LocalStorageManager::UpdateNoteOptions options,
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
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Note> foundNotes, QUuid requestId);

    void onListNotesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        LocalStorageManager::GetNoteOptions options, size_t limit,
        size_t offset, LocalStorageManager::ListNotesOrder order,
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
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    // For tags:
    void onAddTagComplete(Tag tag, QUuid requestId);
    void onUpdateTagComplete(Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(Tag tag, QUuid requestId);

    void onFindTagFailed(
        Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<Tag> foundTags, QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
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
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebookFailed(
        ErrorString errorDescription, Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerTagComplete(
        int noteCount, Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

    void onGetNoteCountPerTagFailed(
        ErrorString errorDescription, Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotesList();
    void requestNotebooksList();
    void requestTagsList();
    void requestSavedSearchesList();

    struct NoteCountRequestOption
    {
        enum type
        {
            Force = 0,
            IfNotAlreadyRunning
        };
    };

    void requestNoteCountForNotebook(
        const QString & notebookLocalUid,
        const NoteCountRequestOption::type option);

    void requestNoteCountForAllNotebooks(
        const NoteCountRequestOption::type option);

    void checkAndIncrementNoteCountPerNotebook(
        const QString & notebookLocalUid);

    void checkAndDecrementNoteCountPerNotebook(
        const QString & notebookLocalUid);

    void checkAndAdjustNoteCountPerNotebook(
        const QString & notebookLocalUid, const bool increment);

    void requestNoteCountForTag(
        const QString & tagLocalUid, const NoteCountRequestOption::type option);

    void requestNoteCountForAllTags(const NoteCountRequestOption::type option);

    void checkAndIncrementNoteCountPerTag(const QString & tagLocalUid);
    void checkAndDecrementNoteCountPerTag(const QString & tagLocalUid);

    void checkAndAdjustNoteCountPerTag(
        const QString & tagLocalUid, const bool increment);

    QVariant dataImpl(const int row, const Column column) const;

    QVariant dataAccessibleText(const int row, const Column column) const;

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

    void updateItemColumnInView(
        const FavoritesModelItem & item, const Column column);

    void checkAllItemsListed();

private:
    struct ByLocalUid
    {};

    struct ByIndex
    {};

    struct ByDisplayName
    {};

    using FavoritesData = boost::multi_index_container<
        FavoritesModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<boost::multi_index::tag<ByIndex>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<
                    FavoritesModelItem, const QString &,
                    &FavoritesModelItem::localUid>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByDisplayName>,
                boost::multi_index::const_mem_fun<
                    FavoritesModelItem, const QString &,
                    &FavoritesModelItem::displayName>>>>;

    using FavoritesDataByLocalUid = FavoritesData::index<ByLocalUid>::type;
    using FavoritesDataByIndex = FavoritesData::index<ByIndex>::type;

    struct NotebookRestrictionsData
    {
        bool m_canUpdateNotes = false;
        bool m_canUpdateNotebook = false;
        bool m_canUpdateTags = false;
    };

    class Comparator
    {
    public:
        Comparator(const Column column, const Qt::SortOrder sortOrder) :
            m_sortedColumn(column), m_sortOrder(sortOrder)
        {}

        bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const;

    private:
        Column m_sortedColumn;
        Qt::SortOrder m_sortOrder;
    };

    using LocalUidToRequestIdBimap = boost::bimap<QString, QUuid>;

private:
    FavoritesData m_data;
    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;
    SavedSearchCache & m_savedSearchCache;

    QSet<QString> m_lowerCaseNotebookNames;
    QSet<QString> m_lowerCaseTagNames;
    QSet<QString> m_lowerCaseSavedSearchNames;

    size_t m_listNotesOffset = 0;
    QUuid m_listNotesRequestId;

    size_t m_listNotebooksOffset = 0;
    QUuid m_listNotebooksRequestId;

    size_t m_listTagsOffset = 0;
    QUuid m_listTagsRequestId;

    size_t m_listSavedSearchesOffset = 0;
    QUuid m_listSavedSearchesRequestId;

    QSet<QUuid> m_updateNoteRequestIds;
    QSet<QUuid> m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findNoteToPerformUpdateRequestIds;
    QSet<QUuid> m_findNoteToUnfavoriteRequestIds;

    QSet<QUuid> m_updateNotebookRequestIds;
    QSet<QUuid> m_findNotebookToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findNotebookToPerformUpdateRequestIds;
    QSet<QUuid> m_findNotebookToUnfavoriteRequestIds;

    QSet<QUuid> m_updateTagRequestIds;
    QSet<QUuid> m_findTagToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findTagToPerformUpdateRequestIds;
    QSet<QUuid> m_findTagToUnfavoriteRequestIds;

    QSet<QUuid> m_updateSavedSearchRequestIds;
    QSet<QUuid> m_findSavedSearchToRestoreFailedUpdateRequestIds;
    QSet<QUuid> m_findSavedSearchToPerformUpdateRequestIds;
    QSet<QUuid> m_findSavedSearchToUnfavoriteRequestIds;

    QHash<QString, QString> m_tagLocalUidToLinkedNotebookGuid;
    QHash<QString, QString> m_notebookLocalUidToGuid;

    QHash<QString, QString> m_notebookLocalUidByNoteLocalUid;

    LocalUidToRequestIdBimap m_notebookLocalUidToNoteCountRequestIdBimap;
    LocalUidToRequestIdBimap m_tagLocalUidToNoteCountRequestIdBimap;

    QHash<QString, NotebookRestrictionsData> m_notebookRestrictionsData;

    Column m_sortedColumn = Column::DisplayName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    bool m_allItemsListed = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
