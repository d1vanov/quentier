#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

SavedSearchLocalStorageManagerAsyncTester::SavedSearchLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_pLocalStorageManagerThread(nullptr),
    m_pInitialSavedSearch(),
    m_pFoundSavedSearch(),
    m_pModifiedSavedSearch(),
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

    m_pInitialSavedSearch = QSharedPointer<SavedSearch>(new SavedSearch);
    m_pInitialSavedSearch->setGuid("00000000-0000-0000-c000-000000000046");
    m_pInitialSavedSearch->setUpdateSequenceNumber(1);
    m_pInitialSavedSearch->setName("Fake saved search name");
    m_pInitialSavedSearch->setQuery("Fake saved search query");
    m_pInitialSavedSearch->setQueryFormat(1);
    m_pInitialSavedSearch->setIncludeAccount(true);
    m_pInitialSavedSearch->setIncludeBusinessLinkedNotebooks(false);
    m_pInitialSavedSearch->setIncludePersonalLinkedNotebooks(true);

    QString errorDescription;
    if (!m_pInitialSavedSearch->checkParameters(errorDescription)) {
        QNWARNING("Found invalid SavedSearch: " << *m_pInitialSavedSearch << ", error: " << errorDescription);
        emit failure(errorDescription);
        return;
    }

    m_state = STATE_SENT_ADD_REQUEST;
    emit addSavedSearchRequest(m_pInitialSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onGetSavedSearchCountCompleted(int count)
{
    QString errorDescription;

#define HANDLE_WRONG_STATE() \
    else { \
        errorDescription = QObject::tr("Internal error in SavedSearchLocalStorageManagerAsyncTester: " \
                                       "found wrong state"); \
        emit failure(errorDescription); \
        return; \
    }

    if (m_state == STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST)
    {
        if (count != 1) {
            errorDescription = QObject::tr("GetSavedSearchCount returned result different from the expected one (1): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_EXPUNGE_REQUEST;
        emit expungeSavedSearchRequest(m_pModifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST)
    {
        if (count != 0) {
            errorDescription = QObject::tr("GetSavedSearchCount returned result different from the expected one (0): ");
            errorDescription += QString::number(count);
            emit failure(errorDescription);
            return;
        }

        QSharedPointer<SavedSearch> extraSavedSearch = QSharedPointer<SavedSearch>(new SavedSearch);
        extraSavedSearch->setGuid("00000000-0000-0000-c000-000000000001");
        extraSavedSearch->setUpdateSequenceNumber(1);
        extraSavedSearch->setName("Extra SavedSearch");
        extraSavedSearch->setQuery("Fake extra saved search query");
        extraSavedSearch->setQueryFormat(1);
        extraSavedSearch->setIncludeAccount(true);
        extraSavedSearch->setIncludeBusinessLinkedNotebooks(true);
        extraSavedSearch->setIncludePersonalLinkedNotebooks(true);

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

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchCompleted(QSharedPointer<SavedSearch> search)
{
    Q_ASSERT_X(!search.isNull(), "SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchCompleted slot",
               "Found NULL shared pointer to SavedSearch");

    QString errorDescription;

    if (m_state == STATE_SENT_ADD_REQUEST)
    {
        if (m_pInitialSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search in onAddSavedSearchCompleted slot doesn't match the original SavedSearch";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_pFoundSavedSearch = QSharedPointer<SavedSearch>(new SavedSearch);
        m_pFoundSavedSearch->setLocalGuid(search->localGuid());

        m_state = STATE_SENT_FIND_AFTER_ADD_REQUEST;
        emit findSavedSearchRequest(m_pFoundSavedSearch);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_SAVED_SEARCH_ONE_REQUEST)
    {
        m_initialSavedSearches << *search;

        QSharedPointer<SavedSearch> extraSavedSearch = QSharedPointer<SavedSearch>(new SavedSearch);
        extraSavedSearch->setGuid("00000000-0000-0000-c000-000000000002");
        extraSavedSearch->setUpdateSequenceNumber(2);
        extraSavedSearch->setName("Extra SavedSearch two");
        extraSavedSearch->setQuery("Fake extra saved search query two");
        extraSavedSearch->setQueryFormat(1);
        extraSavedSearch->setIncludeAccount(true);
        extraSavedSearch->setIncludeBusinessLinkedNotebooks(false);
        extraSavedSearch->setIncludePersonalLinkedNotebooks(true);

        m_state = STATE_SENT_ADD_EXTRA_SAVED_SEARCH_TWO_REQUEST;
        emit addSavedSearchRequest(extraSavedSearch);
    }
    else if (m_state == STATE_SENT_ADD_EXTRA_SAVED_SEARCH_TWO_REQUEST)
    {
        m_initialSavedSearches << *search;

        m_state = STATE_SENT_LIST_SEARCHES_REQUEST;
        emit listAllSavedSearchesRequest();
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onAddSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription)
{
    QNWARNING(errorDescription << ", SavedSearch: " << (search.isNull() ? QString("NULL") : search->ToQString()));
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchCompleted(QSharedPointer<SavedSearch> search)
{
    Q_ASSERT_X(!search.isNull(), "SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchCompleted slot",
               "Found NULL shared pointer to SavedSearch");

    QString errorDescription;

    if (m_state == STATE_SENT_UPDATE_REQUEST)
    {
        if (m_pModifiedSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search pointer in onUpdateSavedSearchCompleted slot doesn't match "
                               "the pointer to the original modified SavedSearch";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_FIND_AFTER_UPDATE_REQUEST;
        emit findSavedSearchRequest(m_pFoundSavedSearch);
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onUpdateSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription)
{
    QNWARNING(errorDescription << ", search: " << (search.isNull() ? QString("NULL") : search->ToQString()));
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchCompleted(QSharedPointer<SavedSearch> search)
{
    Q_ASSERT_X(!search.isNull(), "SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchCompleted slot",
               "Found NULL shared pointer to SavedSearch");

    QString errorDescription;

    if (m_state == STATE_SENT_FIND_AFTER_ADD_REQUEST)
    {
        if (m_pFoundSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search pointer in onFindSavedSearchCompleted slot doesn't match "
                               "the pointer to the original SavedSearch";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pInitialSavedSearch.isNull());
        if (*m_pFoundSavedSearch != *m_pInitialSavedSearch) {
            errorDescription = "Added and found saved searches in local storage don't match";
            QNWARNING(errorDescription << ": SavedSearch added to LocalStorageManager: " << *m_pInitialSavedSearch
                      << "\nSavedSearch found in LocalStorageManager: " << *m_pFoundSavedSearch);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        // Ok, found search is good, updating it now
        m_pModifiedSavedSearch = QSharedPointer<SavedSearch>(new SavedSearch(*m_pInitialSavedSearch));
        m_pModifiedSavedSearch->setUpdateSequenceNumber(m_pInitialSavedSearch->updateSequenceNumber() + 1);
        m_pModifiedSavedSearch->setName(m_pInitialSavedSearch->name() + "_modified");
        m_pModifiedSavedSearch->setQuery(m_pInitialSavedSearch->query() + "_modified");

        m_state = STATE_SENT_UPDATE_REQUEST;
        emit updateSavedSearchRequest(m_pModifiedSavedSearch);
    }
    else if (m_state == STATE_SENT_FIND_AFTER_UPDATE_REQUEST)
    {
        if (m_pFoundSavedSearch != search) {
            errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                               "search pointer in onFindSavedSearchCompleted slot doesn't match "
                               "the pointer to the original modified SavedSearch";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        Q_ASSERT(!m_pModifiedSavedSearch.isNull());
        if (*m_pFoundSavedSearch != *m_pModifiedSavedSearch) {
            errorDescription = "Updated and found saved searches in local storage don't match";
            QNWARNING(errorDescription << ": SavedSearch updated in LocalStorageManager: " << *m_pModifiedSavedSearch
                      << "\nSavedSearch found in LocalStorageManager: " << *m_pFoundSavedSearch);
            errorDescription = QObject::tr(qPrintable(errorDescription));
            emit failure(errorDescription);
            return;
        }

        m_state = STATE_SENT_GET_COUNT_AFTER_UPDATE_REQUEST;
        emit getSavedSearchCountRequest();
    }
    else if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST)
    {
        Q_ASSERT(!m_pModifiedSavedSearch.isNull());
        errorDescription = "Error: found saved search which should have been expunged from local storage";
        QNWARNING(errorDescription << ": SavedSearch expunged from LocalStorageManager: " << *m_pModifiedSavedSearch
                  << "\nSavedSearch found in LocalStorageManager: " << *m_pFoundSavedSearch);
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }
    HANDLE_WRONG_STATE();
}

void SavedSearchLocalStorageManagerAsyncTester::onFindSavedSearchFailed(QSharedPointer<SavedSearch> search, QString errorDescription)
{
    if (m_state == STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST) {
        m_state = STATE_SENT_GET_COUNT_AFTER_EXPUNGE_REQUEST;
        emit getSavedSearchCountRequest();
        return;
    }

    QNWARNING(errorDescription << ", saved search: " << (search.isNull() ? QString("NULL") : search->ToQString()));
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
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }

    foreach(const SavedSearch & search, m_initialSavedSearches)
    {
        if (!searches.contains(search)) {
            errorDescription = "One of initial saved searches was not found "
                               "within found saved searches";
            QNWARNING(errorDescription);
            errorDescription = QObject::tr(qPrintable(errorDescription));
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

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchCompleted(QSharedPointer<SavedSearch> search)
{
    Q_ASSERT_X(!search.isNull(), "SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchCompleted slot",
               "Found NULL pointer to SavedSearch");

    QString errorDescription;

    if (m_pModifiedSavedSearch != search) {
        errorDescription = "Internal error in SavedSearchLocalStorageManagerAsyncTester: "
                           "search pointer in onExpungeSavedSearchCompleted slot doesn't match "
                           "the pointer to the original expunged SavedSearch";
        QNWARNING(errorDescription);
        errorDescription = QObject::tr(qPrintable(errorDescription));
        emit failure(errorDescription);
        return;
    }

    Q_ASSERT(!m_pFoundSavedSearch.isNull());
    m_state = STATE_SENT_FIND_AFTER_EXPUNGE_REQUEST;
    emit findSavedSearchRequest(m_pFoundSavedSearch);
}

void SavedSearchLocalStorageManagerAsyncTester::onExpungeSavedSearchFailed(QSharedPointer<SavedSearch> search,
                                                                           QString errorDescription)
{
    QNWARNING(errorDescription << ", SavedSearch: " << (search.isNull() ? QString("NULL") : search->ToQString()));
    emit failure(errorDescription);
}

void SavedSearchLocalStorageManagerAsyncTester::createConnections()
{
    // Request --> slot connections
    QObject::connect(this, SIGNAL(getSavedSearchCountRequest()), m_pLocalStorageManagerThread,
                     SLOT(onGetSavedSearchCountRequest()));
    QObject::connect(this, SIGNAL(addSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pLocalStorageManagerThread, SLOT(onAddSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(updateSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pLocalStorageManagerThread, SLOT(onUpdateSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(findSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pLocalStorageManagerThread, SLOT(onFindSavedSearchRequest(QSharedPointer<SavedSearch>)));
    QObject::connect(this, SIGNAL(listAllSavedSearchesRequest()), m_pLocalStorageManagerThread,
                     SLOT(onListAllSavedSearchesRequest()));
    QObject::connect(this, SIGNAL(expungeSavedSearchRequest(QSharedPointer<SavedSearch>)),
                     m_pLocalStorageManagerThread, SLOT(onExpungeSavedSearch(QSharedPointer<SavedSearch>)));

    // Slot <-- result connections
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getSavedSearchCountComplete(int)),
                     this, SLOT(onGetSavedSearchCountCompleted(int)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(getSavedSearchCountFailed(QString)),
                     this, SLOT(onGetSavedSearchCountFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SLOT(onAddSavedSearchCompleted(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(addSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SLOT(onAddSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SLOT(onUpdateSavedSearchCompleted(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(updateSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SLOT(onUpdateSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SLOT(onFindSavedSearchCompleted(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(findSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SLOT(onFindSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSavedSearchesComplete(QList<SavedSearch>)),
                     this, SLOT(onListAllSavedSearchesCompleted(QList<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(listAllSavedSearchesFailed(QString)),
                     this, SLOT(onListAllSavedSearchedFailed(QString)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeSavedSearchComplete(QSharedPointer<SavedSearch>)),
                     this, SLOT(onExpungeSavedSearchCompleted(QSharedPointer<SavedSearch>)));
    QObject::connect(m_pLocalStorageManagerThread, SIGNAL(expungeSavedSearchFailed(QSharedPointer<SavedSearch>,QString)),
                     this, SLOT(onExpungeSavedSearchFailed(QSharedPointer<SavedSearch>,QString)));
}

#undef HANDLE_WRONG_STATE

} // namespace test
} // namespace qute_note
