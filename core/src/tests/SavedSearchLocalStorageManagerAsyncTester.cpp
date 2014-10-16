#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

SavedSearchLocalStorageManagerAsyncTester::SavedSearchLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_initialSavedSearch(),
    m_foundSavedSearch(),
    m_modifiedSavedSearch(),
    m_initialSavedSearches()
{}

SavedSearchLocalStorageManagerAsyncTester::~SavedSearchLocalStorageManagerAsyncTester()
{
    // NOTE: shouldn't attempt to delete m_pLocalStorageManagerThread as Qt's parent-child system
    // should take care of that
}

void SavedSearchLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "SavedSearchLocalStorageManagerAsyncTester";
    qint32 userId = 0;
    bool startFromScratch = true;

    if (m_pLocalStorageManagerThread != nullptr) {
        delete m_pLocalStorageManagerThread;
        m_pLocalStorageManagerThread = nullptr;
    }

    m_state = STATE_UNINITIALIZED;

    m_pLocalStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    createConnections();

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

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountCompleted(int count)
{
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

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchCompleted(SavedSearch search)
{
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
        m_foundSavedSearch.setLocalGuid(search.localGuid());

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
        emit listAllSavedSearchesRequest();
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchFailed(SavedSearch search, QString errorDescription)
{
    QNWARNING(errorDescription << ", SavedSearch: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search)
{
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
        m_foundSavedSearch.setLocalGuid(search.localGuid());

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findSavedSearchRequest(m_foundSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription)
{
    QNWARNING(errorDescription << ", search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchCompleted(SavedSearch search)
{
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

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchFailed(SavedSearch search, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getSavedSearchCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", saved search: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onListAllSavedSearchesCompleted(QList<SavedSearch> searches)
{
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

void SavedSearchLocalStorageManagerAsyncTester::onListAllSavedSearchedFailed(QString errorDescription)
{
    QNWARNING(errorDescription);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchCompleted(SavedSearch search)
{
    QString errorDescription;

    if (m_modifiedSavedSearch != search) {
        errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                           "search in onExpungeSavedSearchCompleted slot doesn't match "
                           "the original expunged SavedSearch";
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_foundSavedSearch = SavedSearch();
    m_foundSavedSearch.setLocalGuid(search.localGuid());

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findSavedSearchRequest(m_foundSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchFailed(SavedSearch search,
                                                                           QString errorDescription)
{
    QNWARNING(errorDescription << ", SavedSearch: " << search);
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(failure(QString)),
                     this, SIGNAL(failure(QString)));

    // Request --> slot connections
    QObject::connect(this, SIGNAL(getSavedSearchCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetSavedSearchCountRequest()));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(findSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onFindSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(listAllSavedSearchesRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllSavedSearchesRequest()));
    QObject::connect(this, SIGNAL(expungeSavedSearchRequest(SavedSearch)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeSavedSearch(SavedSearch)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getSavedSearchCountComplete(int)),
                     this, SLOT(onGetSavedSearchCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getSavedSearchCountFailed(QString)),
                     this, SLOT(onGetSavedSearchCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchComplete(SavedSearch)),
                     this, SLOT(onAddSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onAddSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchComplete(SavedSearch)),
                     this, SLOT(onUpdateSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findSavedSearchComplete(SavedSearch)),
                     this, SLOT(onFindSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onFindSavedSearchFailed(SavedSearch,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)),
                     this, SLOT(onListAllSavedSearchesCompleted(QList<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSavedSearchesFailed(QString)),
                     this, SLOT(onListAllSavedSearchedFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeSavedSearchComplete(SavedSearch)),
                     this, SLOT(onExpungeSavedSearchCompleted(SavedSearch)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString)),
                     this, SLOT(onExpungeSavedSearchFailed(SavedSearch,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
