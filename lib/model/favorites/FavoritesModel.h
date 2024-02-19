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

#include "FavoritesModelItem.h"

#include <lib/model/common/AbstractItemModel.h>
#include <lib/model/note/NoteCache.h>
#include <lib/model/notebook/NotebookCache.h>
#include <lib/model/saved_search/SavedSearchCache.h>
#include <lib/model/tag/TagCache.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/SavedSearch.h>
#include <qevercloud/types/Tag.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QSet>

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

RESTORE_WARNINGS

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

class FavoritesModel final : public AbstractItemModel
{
    Q_OBJECT
public:
    FavoritesModel(
        Account account, local_storage::ILocalStoragePtr localStorage,
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

    friend QDebug & operator<<(QDebug & dbg, const Column column);

    [[nodiscard]] const FavoritesModelItem * itemForLocalId(
        const QString & localId) const;

    [[nodiscard]] const FavoritesModelItem * itemAtRow(const int row) const;

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

    [[nodiscard]]  QModelIndex indexForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QString itemNameForLocalId(
        const QString & localId) const override;

    [[nodiscard]] ItemInfo itemInfoForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QStringList itemNames(
        const QString & linkedNotebookGuid) const override
    {
        // NOTE: deliberately not implemented as this method won't be used by
        // FavoritesModel's client
        Q_UNUSED(linkedNotebookGuid);
        return {};
    }

    [[nodiscard]] QList<LinkedNotebookInfo> linkedNotebooksInfo() const override
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
    Qt::ItemFlags flags(const QModelIndex & index) const override;

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

private:
    void connectToLocalStorageEvents(
        local_storage::ILocalStorageNotifier * notifier);

    void requestNotesList();
    void requestNotebooksList();
    void requestTagsList();
    void requestSavedSearchesList();

    enum class NoteCountRequestOption
    {
        Force,
        IfNotAlreadyRunning
    };

    friend QDebug & operator<<(QDebug & dbg, NoteCountRequestOption option);

    void requestNoteCountForNotebook(
        const QString & notebookLocalId,
        const NoteCountRequestOption option);

    void requestNoteCountForAllNotebooks(
        const NoteCountRequestOption option);

    void requestNoteCountForTag(
        const QString & tagLocalId, NoteCountRequestOption option);

    void requestNoteCountForAllTags(NoteCountRequestOption option);

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

    enum class NoteUpdate
    {
        WithTags,
        WithoutTags
    };

    friend QDebug & operator<<(QDebug & dbg, NoteUpdate noteUpdate);

    void onNoteAddedOrUpdated(
        const qevercloud::Note & note, NoteUpdate noteUpdate);

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

private:
    const local_storage::ILocalStoragePtr m_localStorage;

    FavoritesData m_data;
    NoteCache & m_noteCache;
    NotebookCache & m_notebookCache;
    TagCache & m_tagCache;
    SavedSearchCache & m_savedSearchCache;

    QSet<QString> m_lowerCaseNotebookNames;
    QSet<QString> m_lowerCaseTagNames;
    QSet<QString> m_lowerCaseSavedSearchNames;

    bool m_pendingNotesList = false;
    bool m_pendingNotebooksList = false;
    bool m_pendingTagsList = false;
    bool m_pendingSavedSearchesList = false;

    quint64 m_listNotesOffset = 0;
    quint64 m_listNotebooksOffset = 0;
    quint64 m_listTagsOffset = 0;
    quint64 m_listSavedSearchesOffset = 0;

    QSet<QString> m_notebookLocalIdsPendingNoteCounts;
    QSet<QString> m_tagLocalIdsPendingNoteCounts;

    QHash<QString, QString> m_tagLocalIdToLinkedNotebookGuid;
    QHash<QString, QString> m_notebookLocalIdToGuid;
    QHash<QString, QString> m_notebookLocalIdByNoteLocalId;

    QHash<QString, NotebookRestrictionsData> m_notebookRestrictionsData;

    Column m_sortedColumn = Column::DisplayName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    bool m_allItemsListed = false;
};

} // namespace quentier
