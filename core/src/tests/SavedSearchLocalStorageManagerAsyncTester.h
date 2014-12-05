#ifndef __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/SavedSearch.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

namespace test {

class SavedSearchLocalStorageManagerAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit SavedSearchLocalStorageManagerAsyncTester(QObject * parent = nullptr);
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
    void listAllSavedSearchesRequest(QUuid requestId = QUuid());
    void expungeSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();
    void onGetSavedSearchCountCompleted(int count, QUuid requestId);
    void onGetSavedSearchCountFailed(QString errorDescription, QUuid requestId);
    void onAddSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onFindSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);
    void onListAllSavedSearchesCompleted(QList<SavedSearch> searches, QUuid requestId);
    void onListAllSavedSearchedFailed(QString errorDescription, QUuid requestId);
    void onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId);

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
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
