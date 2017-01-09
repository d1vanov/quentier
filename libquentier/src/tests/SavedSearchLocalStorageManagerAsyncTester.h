/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_TESTS_SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define LIB_QUENTIER_TESTS_SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/types/SavedSearch.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class SavedSearchLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit SavedSearchLocalStorageManagerAsyncTester(QObject * parent = Q_NULLPTR);
    ~SavedSearchLocalStorageManagerAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals:
    void getSavedSearchCountRequest(QUuid requestId = QUuid());
    void addSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());
    void updateSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());
    void findSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());
    void listAllSavedSearchesRequest(size_t limit, size_t offset,
                                     LocalStorageManager::ListSavedSearchesOrder::type order,
                                     LocalStorageManager::OrderDirection::type orderDirection,
                                     QUuid requestId = QUuid());
    void expungeSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetSavedSearchCountCompleted(int count, QUuid requestId);
    void onGetSavedSearchCountFailed(QNLocalizedString errorDescription, QUuid requestId);
    void onAddSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onFindSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);
    void onListAllSavedSearchesCompleted(size_t limit, size_t offset,
                                         LocalStorageManager::ListSavedSearchesOrder::type order,
                                         LocalStorageManager::OrderDirection::type orderDirection,
                                         QList<SavedSearch> searches, QUuid requestId);
    void onListAllSavedSearchedFailed(size_t limit, size_t offset,
                                      LocalStorageManager::ListSavedSearchesOrder::type order,
                                      LocalStorageManager::OrderDirection::type orderDirection,
                                      QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);

    void onFailure(QNLocalizedString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
        STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST,
        STATE_SENT_UPDATE_REQUEST,
        STATE_SENT_FIND_AFTER_UPDATE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST,
        STATE_SENT_EXPUNGE_REQUEST,
        STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST,
        STATE_SENT_ADD_EXTRA_SAVED_SEARCH_ONE_REQUEST,
        STATE_SENT_ADD_EXTRA_SAVED_SEARCH_TWO_REQUEST,
        STATE_SENT_LIST_SEARCHES_REQUEST
    };

    State   m_state;

    LocalStorageManagerThreadWorker   * m_pLocalStorageManagerThreadWorker;
    QThread *           m_pLocalStorageManagerThread;

    SavedSearch         m_initialSavedSearch;
    SavedSearch         m_foundSavedSearch;
    SavedSearch         m_modifiedSavedSearch;
    QList<SavedSearch>  m_initialSavedSearches;
};

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
