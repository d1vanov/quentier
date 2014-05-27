#ifndef __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H

#include <client/types/SavedSearch.h>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QThread)

namespace qute_note {
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
    void addSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void updateSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void findSavedSearchRequest(QSharedPointer<SavedSearch> search);
    void listAllSavedSearchesRequest();
    void expungeSavedSearchRequest(QSharedPointer<SavedSearch> search);

// private slots:
    void onGetSavedSearchCountCompleted();
    void onGetSavedSearchCountFailed(QString errorDescription);
    void onAddSavedSearchCompleted(QSharedPointer<SavedSearch> search);
    void onAddSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void onUpdateSavedSearchCompleted(QSharedPointer<SavedSearch> search);
    void onUpdateSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void onFindSavedSearchCompleted(QSharedPointer<SavedSearch> search);
    void onFindSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);
    void onListAllSavedSearchesCompleted(QList<SavedSearch> searches);
    void onListAllSavedSearchedFailed(QString errorDescription);
    void onExpungeSavedSearchCompleted(QSharedPointer<SavedSearch> search);
    void onExpungeSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription);

private:

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
        STATE_SENT_LIST_SEARCHES_REQUEST
    };

    State       m_state;
    QThread   * m_localStorageManagerThread;
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__SAVED_SEARCH_LOCAL_STORAGE_MANAGER_ASYNC_TESTER_H
