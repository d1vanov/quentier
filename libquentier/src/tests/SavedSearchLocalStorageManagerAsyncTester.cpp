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

#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QThread>

namespace quentier {
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
    QString username = QStringLiteral("SavedSearchLocalStorageManagerAsyncTester");
    qint32 userId = 0;
    bool startFromScratch = true;
    bool overrideLock = false;

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
    Account account(username, Account::Type::Evernote, userId);
    m_pLocalStorageManagerThreadWorker = new LocalStorageManagerThreadWorker(account, startFromScratch, overrideLock);
    m_pLocalStorageManagerThreadWorker->moveToThread(m_pLocalStorageManagerThread);

    createConnections();

    m_pLocalStorageManagerThread->start();
}

void SavedSearchLocalStorageManagerAsyncTester::onWorkerInitialized()
{
    m_initialSavedSearch = SavedSearch();
    m_initialSavedSearch.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000046"));
    m_initialSavedSearch.setUpdateSequenceNumber(1);
    m_initialSavedSearch.setName(QStringLiteral("Fake saved search name"));
    m_initialSavedSearch.setQuery(QStringLiteral("Fake saved search query"));
    m_initialSavedSearch.setQueryFormat(1);
    m_initialSavedSearch.setIncludeAccount(true);
    m_initialSavedSearch.setIncludeBusinessLinkedNotebooks(false);
    m_initialSavedSearch.setIncludePersonalLinkedNotebooks(true);

    ErrorString errorDescription;
    if (!m_initialSavedSearch.checkParameters(errorDescription)) {
        QNWARNING(QStringLiteral("Found invalid SavedSearch: ") << m_initialSavedSearch << QStringLiteral(", error: ") << errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addSavedSearchRequest(m_initialSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountCompleted(int count, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription.base() = QStringLiteral("Internal error in SavedSearchLocalStorageManagerAsyncTester: found wrong state"); \
        emit failure(errorDescription.nonLocalizedString()); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription.base() = QStringLiteral("GetSavedSearchCount returned result different from the expected one (1)");
            errorDescription.details() = QString::number(count);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeSavedSearchRequest(m_modifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription.base() = QStringLiteral("GetSavedSearchCount returned result different from the expected one (0)");
            errorDescription.details() = QString::number(count);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        SavedSearch extraSavedSearch;
        extraSavedSearch.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000001"));
        extraSavedSearch.setUpdateSequenceNumber(1);
        extraSavedSearch.setName(QStringLiteral("Extra SavedSearch"));
        extraSavedSearch.setQuery(QStringLiteral("Fake extra saved search query"));
        extraSavedSearch.setQueryFormat(1);
        extraSavedSearch.setIncludeAccount(true);
        extraSavedSearch.setIncludeBusinessLinkedNotebooks(true);
        extraSavedSearch.setIncludePersonalLinkedNotebooks(true);

        m_state = STATE_SENT_ADD_EXTRA_SAVED_SEARCH_ONE_REQUEST;
        emit addSavedSearchRequest(extraSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountFailed(ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_initialSavedSearch != search) {
            errorDescription.base() = QStringLiteral("Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                                                     "search in onAddSavedSearchCompleted slot doesn't match the original SavedSearch");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
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
        extraSavedSearch.setGuid(QStringLiteral("00000000-0000-0000-c000-000000000002"));
        extraSavedSearch.setUpdateSequenceNumber(2);
        extraSavedSearch.setName(QStringLiteral("Extra SavedSearch two"));
        extraSavedSearch.setQuery(QStringLiteral("Fake extra saved search query two"));
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

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_modifiedSavedSearch != search) {
            errorDescription.base() = QStringLiteral("Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                                                     "search in onUpdateSavedSearchCompleted slot doesn't match "
                                                     "the original modified SavedSearch");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_foundSavedSearch = SavedSearch();
        m_foundSavedSearch.setLocalUid(search.localUid());

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findSavedSearchRequest(m_foundSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    Q_UNUSED(requestId)

    ErrorString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (search != m_initialSavedSearch) {
            errorDescription.base() = QStringLiteral("Added and found saved searches in local storage don't match");
            QNWARNING(errorDescription << QStringLiteral(": SavedSearch added to LocalStorageManager: ") << m_initialSavedSearch
                      << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << search);
            emit failure(errorDescription.nonLocalizedString());
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
            errorDescription.base() = QStringLiteral("Added and found by name saved searches in local storage don't match");
            QNWARNING(errorDescription << QStringLiteral(": SavedSearch added to LocalStorageManager: ") << m_initialSavedSearch
                      << QStringLiteral("\nSavedSearch found by name in LocalStorageManager: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        // Ok, found search is good, updating it now
        m_modifiedSavedSearch = m_initialSavedSearch;
        m_modifiedSavedSearch.setUpdateSequenceNumber(m_initialSavedSearch.updateSequenceNumber() + 1);
        m_modifiedSavedSearch.setName(m_initialSavedSearch.name() + QStringLiteral("_modified"));
        m_modifiedSavedSearch.setQuery(m_initialSavedSearch.query() + QStringLiteral("_modified"));

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateSavedSearchRequest(m_modifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (search != m_modifiedSavedSearch) {
            errorDescription.base() = QStringLiteral("Updated and found saved searches in local storage don't match");
            QNWARNING(errorDescription << QStringLiteral(": SavedSearch updated in LocalStorageManager: ") << m_modifiedSavedSearch
                      << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << search);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getSavedSearchCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        errorDescription.base() = QStringLiteral("Error: found saved search which should have been expunged from local storage");
        QNWARNING(errorDescription << QStringLiteral(": SavedSearch expunged from LocalStorageManager: ") << m_modifiedSavedSearch
                  << QStringLiteral("\nSavedSearch found in LocalStorageManager: ") << search);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchFailed(SavedSearch search, ErrorString errorDescription, QUuid requestId)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getSavedSearchCountRequest();
        return;
    }

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
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

    ErrorString errorDescription;

    if (numInitialSearches != numFoundSearches) {
        errorDescription.base() = QStringLiteral("Number of found saved searches does not correspond "
                                                 "to the number of original added saved searches");
        QNWARNING(errorDescription);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    foreach(const SavedSearch & search, m_initialSavedSearches)
    {
        if (!searches.contains(search)) {
            errorDescription.base() = QStringLiteral("One of initial saved searches was not found "
                                                     "within found saved searches");
            QNWARNING(errorDescription);
            emit failure(errorDescription.nonLocalizedString());
            return;
        }
    }

    emit success();
}

void SavedSearchLocalStorageManagerAsyncTester::onListAllSavedSearchedFailed(size_t limit, size_t offset,
                                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                                             ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)

    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    ErrorString errorDescription;

    if (m_modifiedSavedSearch != search) {
        errorDescription.base() = QStringLiteral("Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                                                 "search in onExpungeSavedSearchCompleted slot doesn't match "
                                                 "the original expunged SavedSearch");
        QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId);
        emit failure(errorDescription.nonLocalizedString());
        return;
    }

    m_foundSavedSearch = SavedSearch();
    m_foundSavedSearch.setLocalUid(search.localUid());

    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findSavedSearchRequest(m_foundSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchFailed(SavedSearch search,
                                                                           ErrorString errorDescription, QUuid requestId)
{
    QNWARNING(errorDescription << QStringLiteral(", requestId = ") << requestId << QStringLiteral(", saved search: ") << search);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::onFailure(ErrorString errorDescription)
{
    QNWARNING(QStringLiteral("SavedSearchLocalStorageManagerAsyncTester::onFailure: ") << errorDescription);
    emit failure(errorDescription.nonLocalizedString());
}

void SavedSearchLocalStorageManagerAsyncTester::createConnections()
{
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,failure,ErrorString),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onFailure,ErrorString));

    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,started),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,init));
    QObject::connect(m_pLocalStorageManagerThread, QNSIGNAL(QThread,finished),
                     m_pLocalStorageManagerThread, QNSLOT(QThread,deleteLater));

    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,initialized),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onWorkerInitialized));

    // Request --> slot connections
    QObject::connect(this, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,getSavedSearchCountRequest,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onGetSavedSearchCountRequest,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,addSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,updateSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,findSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this,
                     QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,listAllSavedSearchesRequest,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     m_pLocalStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListAllSavedSearchesRequest,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,QUuid));
    QObject::connect(this, QNSIGNAL(SavedSearchLocalStorageManagerAsyncTester,expungeSavedSearchRequest,SavedSearch,QUuid),
                     m_pLocalStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeSavedSearch,SavedSearch,QUuid));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getSavedSearchCountComplete,int,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onGetSavedSearchCountCompleted,int,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,getSavedSearchCountFailed,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onGetSavedSearchCountFailed,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onAddSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onAddSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onUpdateSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onFindSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onFindSavedSearchFailed,SavedSearch,ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllSavedSearchesComplete,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                              QList<SavedSearch>,QUuid),
                     this,
                     QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onListAllSavedSearchesCompleted,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                            QList<SavedSearch>,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllSavedSearchesFailed,size_t,size_t,
                              LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                              ErrorString,QUuid),
                     this,
                     QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onListAllSavedSearchedFailed,size_t,size_t,
                            LocalStorageManager::ListSavedSearchesOrder::type,LocalStorageManager::OrderDirection::type,
                            ErrorString,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onExpungeSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(m_pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,ErrorString,QUuid),
                     this, QNSLOT(SavedSearchLocalStorageManagerAsyncTester,onExpungeSavedSearchFailed,SavedSearch,ErrorString,QUuid));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace quentier
