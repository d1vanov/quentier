#ifndef __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/SavedSearch.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThread)

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
    void getSavedSearchCountRequest();
    void addSavedSearchRequest(SavedSearch search);
    void updateSavedSearchRequest(SavedSearch search);
    void findSavedSearchRequest(SavedSearch search);
    void listAllSavedSearchesRequest();
    void expungeSavedSearchRequest(SavedSearch search);

private Q_SLOTS:
    void onGetSavedSearchCountCompleted(int count);
    void onGetSavedSearchCountFailed(QString errorDescription);
    void onAddSavedSearchCompleted(SavedSearch search);
    void onAddSavedSearchFailed(SavedSearch search, QString errorDescription);
    void onUpdateSavedSearchCompleted(SavedSearch search);
    void onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription);
    void onFindSavedSearchCompleted(SavedSearch search);
    void onFindSavedSearchFailed(SavedSearch search, QString errorDescription);
    void onListAllSavedSearchesCompleted(QList<SavedSearch> searches);
    void onListAllSavedSearchedFailed(QString errorDescription);
    void onExpungeSavedSearchCompleted(SavedSearch search);
    void onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription);

private:
    void createConnections();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_ADD_REQUEST,
        STATE_SENT_FIND_AFTER_ADD_REQUEST,
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
    LocalStorageManagerThread   * m_pLocalStorageManagerThread;
    SavedSearch         m_initialSavedSearch;
    SavedSearch         m_foundSavedSearch;
    SavedSearch         m_modifiedSavedSearch;
    QList<SavedSearch>  m_initialSavedSearches;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
