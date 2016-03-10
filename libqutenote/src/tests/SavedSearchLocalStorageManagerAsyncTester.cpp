#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QThread>

namespace qute_note {
namespace test {

SavedSearchLocalStorageManagerAsyncTester::SavedSearchLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThreadWorker(Q_NULLPTR),
    m_pLocalStorageManagerThread(Q_NULLPTR),
    m_initialSavedSearch(),
    m_foundSavedSearch(),
    m_modifiedSavedSearch(),
    m_initialSavedSearches()
{}

SavedSearchLocalStorageManagerAsyncTester::~SavedSearchLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
    if (m_pLocalStorageManagerThread) {
        m_pLocalStorageManagerThread->quit();
        m_pLocalStorageManagerThread->wait();
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
    }
}

void SavedSearchLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "SavedSearchLocalStorageManagerAsyncTester";
    qint32 userId = 0;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = Q_NULLPTR;
    }

    if (m_pLocalStorageManagerThreadWorker) {
        delete m_pLocalStorageManagerThreadWorker;
        m_pLocalStorageManagerThreadWorker = Q_NULLPTR;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new QThread(this);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(username, userId, startFromScratch);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void SavedSearchLocalStorageManagerAsyncTester::onWorkerInitialized()
{
    m_initialSavedSearch = SavedSearch();
    m_initialSavedSearch.setGuid("00000000-0000-0000-c000-000000000046");
    m_initialSavedSearch.setUpdateSequenceNumber(1);
    m_initialSavedSearch.setName("Fake saved search name");
    m_initialSavedSearch.setQuery("Fake saved search query");
    m_initialSavedSearch.setQueryFormat(1);
    m_initialSavedSearch.setIncludeAccount(true);
    m_initialSavedSearch.setIncludeBusinessLinkedNotebooks(false);
    m_initialSavedSearch.setIncludePersonalLinkedNotebooks(true);

    QString errorDescription;
    if (!m_initialSavedSearch.checkParameters(errorDescription)) {
        QNWARNING("Found invalid SavedSearch: " << m_initialSavedSearch << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addSavedSearchRequest(m_initialSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: " \
                           "found wrong state"; \
        emit failure(errorDescription); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = "GetSavedSearchCount returned result different from the expected one (1): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeSavedSearchRequest(m_modifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = "GetSavedSearchCount returned result different from the expected one (0): ";
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        SavedSearch extraSavedSearch;
        extraSavedSearch.setGuid("00000000-0000-0000-c000-000000000001");
        extraSavedSearch.setUpdateSequenceNumber(1);
        extraSavedSearch.setName("Extra SavedSearch");
        extraSavedSearch.setQuery("Fake extra saved search query");
        extraSavedSearch.setQueryFormat(1);
        extraSavedSearch.setIncludeAccount(true);
        extraSavedSearch.setIncludeBusinessLinkedNotebooks(true);
        extraSavedSearch.setIncludePersonalLinkedNotebooks(true);

        m_state = STATE_SENT_ADD_EXTRA_SAVED_SEARCH_ONE_REQUEST;
        emit addSavedSearchRequest(extraSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountFailed(QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search in onAddSavedSearchCompleted slot doesn't match the original SavedSearch";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundSavedSearch = SavedSearch();
        m_foundSavedSearch.setLocalUid(search.localUid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findSavedSearchRequest(m_foundSavedSearch);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_SAVED_SEARCH_ONE_REQUEST)
    {
        m_initialSavedSearches << search;

        SavedSearch extraSavedSearch;
        extraSavedSearch.setGuid("00000000-0000-0000-c000-000000000002");
        extraSavedSearch.setUpdateSequenceNumber(2);
        extraSavedSearch.setName("Extra SavedSearch two");
        extraSavedSearch.setQuery("Fake extra saved search query two");
        extraSavedSearch.setQueryFormat(1);
        extraSavedSearch.setIncludeAccount(true);
        extraSavedSearch.setIncludeBusinessLinkedNotebooks(false);
        extraSavedSearch.setIncludePersonalLinkedNotebooks(true);

        m_state = STATE_SENT_ADD_EXTRA_SAVED_SEARCH_TWO_REQUEST;
        emit addSavedSearchRequest(extraSavedSearch);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_SAVED_SEARCH_TWO_REQUEST)
    {
        m_initialSavedSearches << search;

        m_state = STATE_SENT_LIST_SEARCHES_REQUEST;
        size_t limit = 0, offset = 0;
        LocalStorageManager::ListSavedSearchesOrder::type order = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
        LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;
        emit listAllSavedSearchesRequest(limit, offset, order, orderDirection);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search in onUpdateSavedSearchCompleted slot doesn't match "
                               "the original modified SavedSearch";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        m_foundSavedSearch = SavedSearch();
        m_foundSavedSearch.setLocalUid(search.localUid());

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findSavedSearchRequest(m_foundSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (search != m_initialSavedSearch) {
            errorDescription = "Added and found saved searches in local storage don't match";
            QNWARNING(errorDescription << ": SavedSearch added to LocalStorageManager: " << m_initialSavedSearch
                      << "\nSavedSearch found in LocalStorageManager: " << search);
            emit failure(errorDescription);
            return;
        }

        // Attempt to find saved search by name now
        SavedSearch searchToFindByName;
        searchToFindByName.unsetLocalUid();
        searchToFindByName.setName(search.name());

        m_state = STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST;
        emit findSavedSearchRequest(searchToFindByName);
    }
    else if (m_state == STATE_SENT_FIND_BY_NAME_AFTER_ADD_REQUEST)
    {
        if (search != m_initialSavedSearch) {
            errorDescription = "Added and found by name saved searches in local storage don't match";
            QNWARNING(errorDescription << ": SavedSearch added to LocalStorageManager: " << m_initialSavedSearch
                      << "\nSavedSearch found by name in LocalStorageManager: " << search);
            emit failure(errorDescription);
            return;
        }

        // Ok, found search is good, updating it now
        m_modifiedSavedSearch = m_initialSavedSearch;
        m_modifiedSavedSearch.setUpdateSequenceNumber(m_initialSavedSearch.updateSequenceNumber() + 1);
        m_modifiedSavedSearch.setName(m_initialSavedSearch.name() + "_modified");
        m_modifiedSavedSearch.setQuery(m_initialSavedSearch.query() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateSavedSearchRequest(m_modifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (search != m_modifiedSavedSearch) {
            errorDescription = "Updated and found saved searches in local storage don't match";
            QNWARNING(errorDescription << ": SavedSearch updated in LocalStorageManager: " << m_modifiedSavedSearch
                      << "\nSavedSearch found in LocalStorageManager: " << search);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getSavedSearchCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription = "Error: found saved search which should have been expunged from local storage";
        QNWARNING(errorDescription << ": SavedSearch expunged from LocalStorageManager: " << m_modifiedSavedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << search);
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getSavedSearchCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onListAllSavedSearchesCompleted(size_t limit, size_t offset,
                                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                                QList<SavedSearch> searches, QUuid requestId)
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(requestId)

    int numInitialSearches = m_initialSavedSearches.size();
    int numFoundSearches   = searches.size();

    QString errorDescription;

    if (numInitialSearches != numFoundSearches) {
        errorDescription = "Number of found saved searches does not correspond "
                           "to the number of original added saved searches";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    foreach(const SavedSearch & search, m_initialSavedSearches)
    {
        if (!searches.contains(search)) {
            errorDescription = "One of initial saved searches was not found "
                               "within found saved searches";
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }
    }

    emit success();
}

void SavedSearchLocalStorageManagerAsyncTester::onListAllSavedSearchedFailed(size_t limit, size_t offset,
                                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                                             QString errorDescription, QUuid requestId)
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)

    QNWARNING(errorDescription << ", requestId = " << requestId);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    QString errorDescription;

    if (m_modifiedSavedSearch != search) {
        errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                           "search in onExpungeSavedSearchCompleted slot doesn't match "
                           "the original expunged SavedSearch";
        QNWARNING(errorDescription << ", requestId = " << requestId);
        emit failure(errorDescription);
        return;
    }

    m_foundSavedSearch = SavedSearch();
    m_foundSavedSearch.setLocalUid(search.localUid());

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findSavedSearchRequest(m_foundSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchFailed(SavedSearch search,
                                                                           QString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << ", requestId = " << requestId << ", saved search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(started()), m_pLocalStorageManagerThreadWorker, SLOT(init()));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(finished()), m_pLocalStorageManagerThread, SLOT(deleteLater()));

    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(initialized()), this, SLOT(onWorkerInitialized()));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(getSavedSearchCountRequest(QUuid)), m_pLocalStorageManagerThreadWorker,
                     SLOT(onGetSavedSearchCountRequest(QUuid)));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(findSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this,
                     SIGNAL(listAllSavedSearchesRequest(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QUuid)),
                     m_pLocalStorageManagerThreadWorker,
                     SLOT(onListAllSavedSearchesRequest(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QUuid)));
    QObject::connect(this, SIGNAL(expungeSavedSearchRequest(SavedSearch,QUuid)),
                     m_pLocalStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch,QUuid)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getSavedSearchCountComplete(int,QUuid)),
                     this, SLOT(onGetSavedSearchCountCompleted(int,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(getSavedSearchCountFailed(QString,QUuid)),
                     this, SLOT(onGetSavedSearchCountFailed(QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onFindSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onFindSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     SIGNAL(listAllSavedSearchesComplete(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)),
                     this,
                     SLOT(onListAllSavedSearchesCompleted(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                          LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     SIGNAL(listAllSavedSearchesFailed(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListAllSavedSearchedFailed(size_t,size_t,LocalStorageManager::ListSavedSearchesOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeSavedSearchComplete(SavedSearch,QUuid)),
                     this, SLOT(onExpungeSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(m_pLocalStorageManagerThreadWorker, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString,QUuid)),
                     this, SLOT(onExpungeSavedSearchFailed(SavedSearch,QString,QUuid)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
