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

#ifndef QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
#define QUENTIER_LIB_MODEL_FAVORITES_MODEL_H

#include "FavoritesModelItem.h"

#include <lib/model/common/AbstractItemModel.h>
#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/saved_search/SavedSearchCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/types/Account.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QSet>
#include <QUuid>

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

RESTORE_WARNINGS

class QDebug;

namespace quentier {

class FavoritesModel final : public AbstractItemModel
{
    Q_OBJECT
public:
    explicit FavoritesModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        NoteCache & noteCache, NotebookCache & notebookCache,
        TagCache & tagCache, SavedSearchCache & savedSearchCache,
        QObject * parent = nullptr);

    ~FavoritesModel() override;

    enum class Column
    {
        Type,
        DisplayName,
        NoteCount
    };

    friend QDebug & operator<<(QDebug & dbg, Column column);

    [[nodiscard]] const FavoritesModelItem * itemForLocalId(
        const QString & localId) const;

    [[nodiscard]] const FavoritesModelItem * itemAtRow(int row) const;

public:
    // AbstractItemModel interface
    [[nodiscard]] QString localIdForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's clients
        Q_UNUSED(itemName)
        Q_UNUSED(linkedNotebookGuid)
        return {};
    }

    [[nodiscard]] QModelIndex indexForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QString itemNameForLocalId(
        const QString & localId) const override;

    [[nodiscard]] ItemInfo itemInfoForLocalId(
        const QString & localId) const override;

    [[nodiscard]]  QStringList itemNames(
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        Q_UNUSED(linkedNotebookGuid);
        return {};
    }

    [[nodiscard]] QVector<LinkedNotebookInfo> linkedNotebooksInfo() const
        override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        return {};
    }

    [[nodiscard]] QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        Q_UNUSED(linkedNotebookGuid);
        return {};
    }

    [[nodiscard]] int nameColumn() const noexcept override
    {
        return static_cast<int>(Column::DisplayName);
    }

    [[nodiscard]] int sortingColumn() const noexcept override
    {
        return static_cast<int>(m_sortedColumn);
    }

    [[nodiscard]] Qt::SortOrder sortOrder() const noexcept override
    {
        return m_sortOrder;
    }

    [[nodiscard]] bool allItemsListed() const noexcept override
    {
        return m_allItemsListed;
    }

    [[nodiscard]] QString localIdForItemIndex(
        const QModelIndex & index) const override;

    [[nodiscard]] QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const override
    {
        Q_UNUSED(index)
        return {};
    }

public:
    // QAbstractItemModel interface
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex & index) const override;

    [[nodiscard]] QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    [[nodiscard]] int rowCount(const QModelIndex & parent = {}) const override;

    [[nodiscard]] int columnCount(
        const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex index(
        int row, int column, const QModelIndex & parent = {}) const override;

    [[nodiscard]] QModelIndex parent(const QModelIndex & index) const override;

    [[nodiscard]] bool setHeaderData(
        int section, Qt::Orientation orientation, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool setData(
        const QModelIndex & index, const QVariant & value,
        int role = Qt::EditRole) override;

    [[nodiscard]] bool insertRows(
        int row, int count, const QModelIndex & parent = {}) override;

    [[nodiscard]] bool removeRows(
        int row, int count, const QModelIndex & parent = {}) override;

    void sort(int column, Qt::SortOrder order) override;

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

    void updateNotebook(qevercloud::Notebook notebook, QUuid requestId);
    void findNotebook(qevercloud::Notebook notebook, QUuid requestId);

    void listNotebooks(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateTag(qevercloud::Tag tag, QUuid requestId);
    void findTag(qevercloud::Tag tag, QUuid requestId);

    void listTags(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QUuid requestId);

    void updateSavedSearch(qevercloud::SavedSearch search, QUuid requestId);
    void findSavedSearch(qevercloud::SavedSearch search, QUuid requestId);

    void listSavedSearches(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection, QUuid requestId);

    void noteCountPerNotebook(
        qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void noteCountPerTag(
        qevercloud::Tag tag, LocalStorageManager::NoteCountOptions options,
        QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage

    // For notes:
    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onNoteMovedToAnotherNotebook(
        QString noteLocalId, QString previousNotebookLocalId,
        QString newNotebookLocalId);

    void onNoteTagListChanged(
        QString noteLocalId, QStringList previousNoteTagLocalIds,
        QStringList newNoteTagLocalIds);

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

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    // For notebooks:
    void onAddNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onUpdateNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onFindNotebookComplete(qevercloud::Notebook notebook, QUuid requestId);

    void onFindNotebookFailed(
        qevercloud::Notebook notebook, ErrorString errorDescription,
        QUuid requestId);

    void onListNotebooksComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Notebook> foundNotebooks,
        QUuid requestId);

    void onListNotebooksFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListNotebooksOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    // For tags:
    void onAddTagComplete(qevercloud::Tag tag, QUuid requestId);
    void onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onUpdateTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onFindTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onFindTagFailed(
        qevercloud::Tag tag, ErrorString errorDescription, QUuid requestId);

    void onListTagsComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, QList<qevercloud::Tag> foundTags,
        QUuid requestId);

    void onListTagsFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListTagsOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QString linkedNotebookGuid, ErrorString errorDescription,
        QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

    // For saved searches:
    void onAddSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onFindSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    void onFindSavedSearchFailed(
        qevercloud::SavedSearch search, ErrorString errorDescription,
        QUuid requestId);

    void onListSavedSearchesComplete(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<qevercloud::SavedSearch> foundSearches, QUuid requestId);

    void onListSavedSearchesFailed(
        LocalStorageManager::ListObjectsOptions flag, size_t limit,
        size_t offset, LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeSavedSearchComplete(
        qevercloud::SavedSearch search, QUuid requestId);

    // For note counts:
    void onGetNoteCountPerNotebookComplete(
        int noteCount, qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerNotebookFailed(
        ErrorString errorDescription, qevercloud::Notebook notebook,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerTagComplete(
        int noteCount, qevercloud::Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

    void onGetNoteCountPerTagFailed(
        ErrorString errorDescription, qevercloud::Tag tag,
        LocalStorageManager::NoteCountOptions options, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestNotesList();
    void requestNotebooksList();
    void requestTagsList();
    void requestSavedSearchesList();

    enum class NoteCountRequestOption
    {
        Force = 0,
        IfNotAlreadyRunning
    };

    friend QDebug & operator<<(QDebug & dbg, NoteCountRequestOption option);

    void requestNoteCountForNotebook(
        const QString & notebookLocalId,
        NoteCountRequestOption option);

    void requestNoteCountForAllNotebooks(
        NoteCountRequestOption option);

    void checkAndIncrementNoteCountPerNotebook(
        const QString & notebookLocalId);

    void checkAndDecrementNoteCountPerNotebook(
        const QString & notebookLocalId);

    void checkAndAdjustNoteCountPerNotebook(
        const QString & notebookLocalId, bool increment);

    void requestNoteCountForTag(
        const QString & tagLocalId, NoteCountRequestOption option);

    void requestNoteCountForAllTags(NoteCountRequestOption option);

    void checkAndIncrementNoteCountPerTag(const QString & tagLocalId);
    void checkAndDecrementNoteCountPerTag(const QString & tagLocalId);

    void checkAndAdjustNoteCountPerTag(
        const QString & tagLocalId, bool increment);

    [[nodiscard]] QVariant dataImpl(int row, Column column) const;

    [[nodiscard]] QVariant dataAccessibleText(int row, Column column) const;

    void removeItemByLocalId(const QString & localId);
    void updateItemRowWithRespectToSorting(const FavoritesModelItem & item);
    void updateItemInLocalStorage(const FavoritesModelItem & item);
    void updateNoteInLocalStorage(const FavoritesModelItem & item);
    void updateNotebookInLocalStorage(const FavoritesModelItem & item);
    void updateTagInLocalStorage(const FavoritesModelItem & item);
    void updateSavedSearchInLocalStorage(const FavoritesModelItem & item);

    [[nodiscard]] bool canUpdateNote(const QString & localId) const;
    [[nodiscard]] bool canUpdateNotebook(const QString & localId) const;
    [[nodiscard]] bool canUpdateTag(const QString & localId) const;

    void unfavoriteNote(const QString & localId);
    void unfavoriteNotebook(const QString & localId);
    void unfavoriteTag(const QString & localId);
    void unfavoriteSavedSearch(const QString & localId);

    void onNoteAddedOrUpdated(
        const qevercloud::Note & note, bool tagsUpdated = true);

    void onNotebookAddedOrUpdated(const qevercloud::Notebook & notebook);
    void onTagAddedOrUpdated(const qevercloud::Tag & tag);
    void onSavedSearchAddedOrUpdated(const qevercloud::SavedSearch & search);

    void updateItemColumnInView(
        const FavoritesModelItem & item, Column column);

    void checkAllItemsListed();

private:
    struct ByLocalId
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
                boost::multi_index::tag<ByLocalId>,
                boost::multi_index::const_mem_fun<
                    FavoritesModelItem, const QString &,
                    &FavoritesModelItem::localId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByDisplayName>,
                boost::multi_index::const_mem_fun<
                    FavoritesModelItem, const QString &,
                    &FavoritesModelItem::displayName>>>>;

    using FavoritesDataByLocalId = FavoritesData::index<ByLocalId>::type;
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

        [[nodiscard]] bool operator()(
            const FavoritesModelItem & lhs,
            const FavoritesModelItem & rhs) const noexcept;

    private:
        Column m_sortedColumn;
        Qt::SortOrder m_sortOrder;
    };

    using LocalIdToRequestIdBimap = boost::bimap<QString, QUuid>;

private:
    FavoritesData m_data;
    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;
    SavedSearchCache & m_savedSearchCache;

    QSet<QString> m_lowerCaseNotebookNames;
    QSet<QString> m_lowerCaseTagNames;
    QSet<QString> m_lowerCaseSavedSearchNames;

    quint64 m_listNotesOffset = 0;
    QUuid m_listNotesRequestId;

    quint64 m_listNotebooksOffset = 0;
    QUuid m_listNotebooksRequestId;

    quint64 m_listTagsOffset = 0;
    QUuid m_listTagsRequestId;

    quint64 m_listSavedSearchesOffset = 0;
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

    QHash<QString, QString> m_tagLocalIdToLinkedNotebookGuid;
    QHash<QString, QString> m_notebookLocalIdToGuid;

    QHash<QString, QString> m_notebookLocalIdByNoteLocalId;

    LocalIdToRequestIdBimap m_notebookLocalIdToNoteCountRequestIdBimap;
    LocalIdToRequestIdBimap m_tagLocalIdToNoteCountRequestIdBimap;

    QHash<QString, NotebookRestrictionsData> m_notebookRestrictionsData;

    Column m_sortedColumn = Column::DisplayName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    bool m_allItemsListed = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_FAVORITES_MODEL_H
