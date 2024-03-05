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

#include "SavedSearchCache.h"

#include "ISavedSearchModelItem.h"
#include "SavedSearchItem.h"

#include <lib/model/common/AbstractItemModel.h>
#include <lib/utility/IStartable.h>

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>
#include <quentier/types/Fwd.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/cancelers/Fwd.h>

#include <qevercloud/types/SavedSearch.h>

#include <QAbstractItemModel>
#include <QSet>

#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index_container.hpp>

class QDebug;
class QTextStream;

namespace quentier {

class SavedSearchModel final : public AbstractItemModel, public IStartable
{
    Q_OBJECT
public:
    explicit SavedSearchModel(
        Account account, local_storage::ILocalStoragePtr localStorage,
        SavedSearchCache & cache, QObject * parent = nullptr);

    ~SavedSearchModel() override;

    enum class Column
    {
        Name = 0,
        Query,
        Synchronizable,
        Dirty
    };

    friend QDebug & operator<<(QDebug & dbg, Column column);
    friend QTextStream & operator<<(QTextStream & strm, Column);

    [[nodiscard]] ISavedSearchModelItem * itemForIndex(
        const QModelIndex & index) const;

    [[nodiscard]] QModelIndex indexForItem(
        const ISavedSearchModelItem * item) const;

    [[nodiscard]] QModelIndex indexForSavedSearchName(
        const QString & savedSearchName) const;

    /**
     * @brief queryForLocalId
     * @return query corresponding to the saved search with the passed in local
     *         id or empty string if unable to find that saved search
     */
    [[nodiscard]] QString queryForLocalId(const QString & localId) const;

    /**
     * @brief savedSearchNames
     * @return the sorted (in case insensitive manner) list of saved search
     * names existing within the saved search model
     */
    [[nodiscard]] QStringList savedSearchNames() const;

    /**
     * @brief createSavedSearch - convenience method to create a new saved
     * search within the model
     * @param savedSearchName       Name of the new saved search
     * @param searchQuery           Search query being saved
     * @param errorDescription      Textual description of the error if
     *                              saved search was not created successfully
     * @return                      Either valid model index if saved search was
     *                              created successfully or invalid model index
     *                              otherwise
     */
    [[nodiscard]] QModelIndex createSavedSearch(
        const QString & savedSearchName, const QString & searchQuery,
        ErrorString & errorDescription);

    /**
     * @brief allSavedSearchesListed
     * @return                      True if saved search model has received
     *                              information about all saved searches stored
     *                              in the local storage by the moment; false
     *                              otherwise
     */
    [[nodiscard]] bool allSavedSearchesListed() const noexcept
    {
        return m_allSavedSearchesListed;
    }

    /**
     * @brief favoriteSavedSearch - marks the saved search pointed to by
     * the index as favorited
     *
     * Favorited property of SavedSearch class is not represented as a column
     * within the SavedSearchModel so this method doesn't change anything in
     * the model but only the underlying saved search object persisted in
     * the local storage
     *
     * @param index                 Index of the saved search to be favorited
     */
    void favoriteSavedSearch(const QModelIndex & index);

    /**
     * @brief unfavoriteSavedSearch - removes the favorited mark from the saved
     * search pointed to by the index; does nothing if the saved search was not
     * favorited prior to the call
     *
     * Favorited property of SavedSearch class is not represented as a column
     * within the SavedSearchModel so this method doesn't change anything in
     * the model but only the underlying saved search object persisted in
     * the local storage
     *
     * @param index                 Index of the saved search to be unfavorited
     */
    void unfavoriteSavedSearch(const QModelIndex & index);

public:
    // AbstractItemModel interface
    [[nodiscard]] QString localIdForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QModelIndex indexForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QString itemNameForLocalId(
        const QString & localId) const override;

    [[nodiscard]] ItemInfo itemInfoForLocalId(
        const QString & localId) const override;

    [[nodiscard]] QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] QList<LinkedNotebookInfo> linkedNotebooksInfo()
        const override;

    [[nodiscard]] QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const override;

    [[nodiscard]] int nameColumn() const noexcept override
    {
        return static_cast<int>(Column::Name);
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
        return m_allSavedSearchesListed;
    }

    [[nodiscard]] QModelIndex allItemsRootItemIndex() const override;

    [[nodiscard]] QString localIdForItemIndex(
        const QModelIndex & index) const override;

    [[nodiscard]] QString linkedNotebookGuidForItemIndex(
        const QModelIndex &) const override
    {
        return {};
    }

public: // IStartable interface
    void start() override;

    [[nodiscard]] bool isStarted() const noexcept override
    {
        return m_isStarted;
    }

    void stop(StopMode stopMode) override;

public: // QAbstractItemModel interface
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
    void notifyAllSavedSearchesListed();

    // Informative signals for views, so that they can prepare to the changes
    // in the table of saved searches and do some recovery after that
    void aboutToAddSavedSearch();
    void addedSavedSearch(const QModelIndex & index);

    void aboutToUpdateSavedSearch(const QModelIndex & index);
    void updatedSavedSearch(const QModelIndex & index);

    void aboutToRemoveSavedSearches();
    void removedSavedSearches();

private:
    void connectToLocalStorageEvents();
    void disconnectFromLocalStorageEvents();

    void requestSavedSearchesList();
    void onSavedSearchesListed(
        const QList<qevercloud::SavedSearch> & savedSearches);

    void onSavedSearchAddedOrUpdated(const qevercloud::SavedSearch & search);
    void onFailedToExpungeSavedSearch(const QString & localId);

    [[nodiscard]] QVariant dataImpl(int row, Column column) const;
    [[nodiscard]] QVariant dataAccessibleText(int row, Column column) const;
    [[nodiscard]] QString nameForNewSavedSearch() const;

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    [[nodiscard]] int rowForNewItem(const SavedSearchItem & newItem) const;

    void updateRandomAccessIndexWithRespectToSorting(
        const SavedSearchItem & item);

    void updateSavedSearchInLocalStorage(const SavedSearchItem & item);

    void setSavedSearchFavorited(const QModelIndex & index, bool favorited);

    void checkAndCreateModelRootItems();

    void removeSavedSearchItem(const QString & localId);
    void restoreSavedSearchItemFromLocalStorage(const QString & localId);

    void clearModel();

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

private:
    struct ByLocalId
    {};

    struct ByIndex
    {};

    struct ByNameUpper
    {};

    using SavedSearchData = boost::multi_index_container<
        SavedSearchItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<boost::multi_index::tag<ByIndex>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalId>,
                boost::multi_index::const_mem_fun<
                    SavedSearchItem, const QString &,
                    &SavedSearchItem::localId>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    SavedSearchItem, QString, &SavedSearchItem::nameUpper>>>>;

    using SavedSearchDataByLocalId = SavedSearchData::index<ByLocalId>::type;
    using SavedSearchDataByIndex = SavedSearchData::index<ByIndex>::type;

    using SavedSearchDataByNameUpper =
        SavedSearchData::index<ByNameUpper>::type;

    using IndexId = quintptr;

    struct LessByName
    {
        [[nodiscard]] bool operator()(
            const SavedSearchItem & lhs,
            const SavedSearchItem & rhs) const;
    };

    struct GreaterByName
    {
        [[nodiscard]] bool operator()(
            const SavedSearchItem & lhs,
            const SavedSearchItem & rhs) const;
    };

    [[nodiscard]] QModelIndex indexForLocalIdIndexIterator(
        const SavedSearchDataByLocalId::const_iterator it) const;

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    SavedSearchData m_data;

    ISavedSearchModelItem * m_invisibleRootItem = nullptr;

    ISavedSearchModelItem * m_allSavedSearchesRootItem = nullptr;
    IndexId m_allSavedSearchesRootItemIndexId = 1;

    bool m_connectedToLocalStorage = false;
    bool m_isStarted = false;

    quint64 m_listSavedSearchesOffset = 0;
    QSet<QString> m_savedSearchItemsNotYetInLocalStorageIds;

    utility::cancelers::ManualCancelerPtr m_canceler;

    SavedSearchCache & m_cache;

    Column m_sortedColumn = Column::Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    mutable int m_lastNewSavedSearchNameCounter = 0;
    bool m_allSavedSearchesListed = false;
};

} // namespace quentier
