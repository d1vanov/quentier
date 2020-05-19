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

#ifndef QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_H
#define QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_H

#include "ItemModel.h"
#include "SavedSearchModelItem.h"
#include "SavedSearchCache.h"

#include <quentier/types/SavedSearch.h>
#include <quentier/types/Account.h>
#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/SuppressWarnings.h>

#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>

SAVE_WARNINGS
GCC_SUPPRESS_WARNING(-Wdeprecated-declarations)

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

RESTORE_WARNINGS

namespace quentier {

class SavedSearchModel: public ItemModel
{
    Q_OBJECT
public:
    explicit SavedSearchModel(
        const Account & account,
        LocalStorageManagerAsync & localStorageManagerAsync,
        SavedSearchCache & cache,
        QObject * parent = nullptr);

    virtual ~SavedSearchModel();

    const Account & account() const { return m_account; }
    void updateAccount(const Account & account);

    struct Columns
    {
        enum type {
            Name = 0,
            Query,
            Synchronizable,
            Dirty
        };
    };

    const SavedSearchModelItem * itemForIndex(const QModelIndex & index) const;
    QModelIndex indexForItem(const SavedSearchModelItem * item) const;
    QModelIndex indexForLocalUid(const QString & localUid) const;
    QModelIndex indexForSavedSearchName(const QString & savedSearchName) const;

    /**
     * @brief savedSearchNames
     * @return the sorted (in case insensitive manner) list of saved search names
     * existing within the saved search model
     */
    QStringList savedSearchNames() const;

    /**
     * @brief createSavedSearch - convenience method to create a new saved search
     * within the model
     * @param savedSearchName       The name of the new saved search
     * @param searchQuery           The search query being saved
     * @param errorDescription      The textual description of the error if
     *                              saved search was not created successfully
     * @return                      Either valid model index if saved search was
     *                              created successfully or invalid model index
     *                              otherwise
     */
    QModelIndex createSavedSearch(
        const QString & savedSearchName, const QString & searchQuery,
        ErrorString & errorDescription);

    /**
     * @brief allSavedSearchesListed
     * @return                      True if the saved search model has received
     *                              the information about all saved searches
     *                              stored in the local storage by the moment;
     *                              false otherwise
     */
    bool allSavedSearchesListed() const { return m_allSavedSearchesListed; }

    /**
     * @brief favoriteSavedSearch - marks the saved search pointed to by
     * the index as favorited
     *
     * Favorited property of SavedSearch class is not represented as a column
     * within the SavedSearchModel so this method doesn't change anything in
     * the model but only the underlying saved search object persisted in
     * the local storage
     *
     * @param index                 The index of the saved search to be favorited
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
     * @param index                 The index of the saved search to be unfavorited
     */
    void unfavoriteSavedSearch(const QModelIndex & index);

public:
    // ItemModel interface
    virtual QString localUidForItemName(
        const QString & itemName,
        const QString & linkedNotebookGuid) const override;

    virtual QString itemNameForLocalUid(
        const QString & localUid) const override;

    virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const override;

    virtual int nameColumn() const override { return Columns::Name; }
    virtual int sortingColumn() const override { return m_sortedColumn; }
    virtual Qt::SortOrder sortOrder() const override { return m_sortOrder; }
    virtual bool allItemsListed() const override
    { return m_allSavedSearchesListed; }

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
    void notifyAllSavedSearchesListed();

    // Informative signals for views, so that they can prepare to the changes
    // in the table of saved searches and do some recovery after that
    void aboutToAddSavedSearch();
    void addedSavedSearch(const QModelIndex & index);

    void aboutToUpdateSavedSearch(const QModelIndex & index);
    void updatedSavedSearch(const QModelIndex & index);

    void aboutToRemoveSavedSearches();
    void removedSavedSearches();

// private signals
    void addSavedSearch(SavedSearch search, QUuid requestId);
    void updateSavedSearch(SavedSearch search, QUuid requestId);
    void findSavedSearch(SavedSearch search, QUuid requestId);

    void listSavedSearches(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QUuid requestId);

    void expungeSavedSearch(SavedSearch search, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onAddSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onUpdateSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onUpdateSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onFindSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onFindSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

    void onListSavedSearchesComplete(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        QList<SavedSearch> foundSearches, QUuid requestId);

    void onListSavedSearchesFailed(
        LocalStorageManager::ListObjectsOptions flag,
        size_t limit, size_t offset,
        LocalStorageManager::ListSavedSearchesOrder order,
        LocalStorageManager::OrderDirection orderDirection,
        ErrorString errorDescription, QUuid requestId);

    void onExpungeSavedSearchComplete(SavedSearch search, QUuid requestId);

    void onExpungeSavedSearchFailed(
        SavedSearch search, ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);
    void requestSavedSearchesList();

    void onSavedSearchAddedOrUpdated(const SavedSearch & search);

    QVariant dataImpl(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    QString nameForNewSavedSearch() const;

    // Returns the appropriate row before which the new item should be inserted
    // according to the current sorting criteria and column
    int rowForNewItem(const SavedSearchModelItem & newItem) const;

    void updateRandomAccessIndexWithRespectToSorting(
        const SavedSearchModelItem & item);

    void updateSavedSearchInLocalStorage(const SavedSearchModelItem & item);

    void setSavedSearchFavorited(const QModelIndex & index, const bool favorited);

private:
    struct ByLocalUid{};
    struct ByIndex{};
    struct ByNameUpper{};

    typedef boost::multi_index_container<
        SavedSearchModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::member<
                    SavedSearchModelItem,QString,&SavedSearchModelItem::m_localUid>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByNameUpper>,
                boost::multi_index::const_mem_fun<
                    SavedSearchModelItem,QString,&SavedSearchModelItem::nameUpper>
            >
        >
    > SavedSearchData;

    typedef SavedSearchData::index<ByLocalUid>::type SavedSearchDataByLocalUid;
    typedef SavedSearchData::index<ByIndex>::type SavedSearchDataByIndex;
    typedef SavedSearchData::index<ByNameUpper>::type SavedSearchDataByNameUpper;

    struct LessByName
    {
        bool operator()(const SavedSearchModelItem & lhs,
                        const SavedSearchModelItem & rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const SavedSearchModelItem & lhs,
                        const SavedSearchModelItem & rhs) const;
    };

    QModelIndex indexForLocalUidIndexIterator(
        const SavedSearchDataByLocalUid::const_iterator it) const;

private:
    Account                 m_account;
    SavedSearchData         m_data;
    size_t                  m_listSavedSearchesOffset;
    QUuid                   m_listSavedSearchesRequestId;
    QSet<QUuid>             m_savedSearchItemsNotYetInLocalStorageUids;

    SavedSearchCache &      m_cache;

    QSet<QUuid>             m_addSavedSearchRequestIds;
    QSet<QUuid>             m_updateSavedSearchRequestIds;
    QSet<QUuid>             m_expungeSavedSearchRequestIds;

    QSet<QUuid>             m_findSavedSearchToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findSavedSearchToPerformUpdateRequestIds;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    mutable int             m_lastNewSavedSearchNameCounter;

    bool                    m_allSavedSearchesListed;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_SAVED_SEARCH_MODEL_H
