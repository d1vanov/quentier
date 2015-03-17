#include "SendLocalChangesManager.h"
#include <client/Utility.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>
#include <QTimerEvent>

namespace qute_note {

SendLocalChangesManager::SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                 QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                 QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_noteStore(pNoteStore),
    m_lastUpdateCount(0),
    m_lastUpdateCountByLinkedNotebookGuid(),
    m_shouldRepeatIncrementalSync(false),
    m_paused(false),
    m_requestedToStop(false),
    m_connectedToLocalStorage(false),
    m_receivedDirtyLocalStorageObjectsFromUsersAccount(false),
    m_receivedAllDirtyLocalStorageObjects(false),
    m_listDirtyTagsRequestId(),
    m_listDirtySavedSearchesRequestId(),
    m_listDirtyNotebooksRequestId(),
    m_listDirtyNotesRequestId(),
    m_listLinkedNotebooksRequestId(),
    m_listDirtyTagsFromLinkedNotebooksRequestIds(),
    m_listDirtyNotebooksFromLinkedNotebooksRequestIds(),
    m_listDirtyNotesFromLinkedNotebooksRequestIds(),
    m_tags(),
    m_savedSearches(),
    m_notebooks(),
    m_notes(),
    m_linkedNotebookGuidsAndShareKeys(),
    m_lastProcessedLinkedNotebookGuidIndex(-1),
    m_authenticationTokensByLinkedNotebookGuid(),
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid(),
    m_updateTagRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_updateNotebookRequestIds(),
    m_updateNoteRequestIds(),
    m_findNotebookRequestIds(),
    m_notebooksByGuidsCache(),
    m_sendTagsPostponeTimerId(0),
    m_sendSavedSearchesPostponeTimerId(0),
    m_sendNotebooksPostponeTimerId(0),
    m_sendNotesPostponeTimerId(0)
{}

void SendLocalChangesManager::start(const qint32 updateCount, QHash<QString,qint32> updateCountByLinkedNotebookGuid)
{
    QNDEBUG("SendLocalChangesManager::start: update count = " << updateCount
            << ", update count by linked notebook guid = " << updateCountByLinkedNotebookGuid);

    if (m_paused) {
        m_lastUpdateCount = updateCount;
        m_lastUpdateCountByLinkedNotebookGuid = updateCountByLinkedNotebookGuid;
        resume();
        return;
    }

    clear();
    m_lastUpdateCount = updateCount;
    m_lastUpdateCountByLinkedNotebookGuid = updateCountByLinkedNotebookGuid;

    requestStuffFromLocalStorage();
}

void SendLocalChangesManager::pause()
{
    QNDEBUG("SendLocalChangesManager::pause");
    m_paused = true;
    emit paused(/* pending authentication = */ false);
}

void SendLocalChangesManager::stop()
{
    QNDEBUG("SendLocalChangesManager::stop");
    m_requestedToStop = true;
    emit stopped();
}

void SendLocalChangesManager::resume()
{
    QNDEBUG("SendLocalChangesManager::resume");

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused)
    {
        m_paused = false;

        if (m_receivedAllDirtyLocalStorageObjects) {
            sendLocalChanges();
        }
        else {
            start(m_lastUpdateCount, m_lastUpdateCountByLinkedNotebookGuid);
        }
    }
}

void SendLocalChangesManager::onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString,QString> authenticationTokensByLinkedNotebookGuid,
                                                                               QHash<QString,qevercloud::Timestamp> authenticationTokenExpirationTimesByLinkedNotebookGuid)
{
    QNDEBUG("SendLocalChangesManager::onAuthenticationTokensForLinkedNotebooksReceived");

    m_authenticationTokensByLinkedNotebookGuid = authenticationTokensByLinkedNotebookGuid;
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid = authenticationTokenExpirationTimesByLinkedNotebookGuid;

    sendLocalChanges();
}

#define CHECK_PAUSED() \
    if (m_paused && !m_requestedToStop) { \
        QNDEBUG("SendLocalChangesManager is being paused, returning without any actions"); \
        return; \
    }

#define CHECK_STOPPED() \
    if (m_requestedToStop && !hasPendingRequests()) { \
        QNDEBUG("SendLocalChangesManager is requested to stop and has no pending requests, finishing sending the local changes"); \
        finalize(); \
        return; \
    }

void SendLocalChangesManager::onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                       size_t limit, size_t offset,
                                                       LocalStorageManager::ListTagsOrder::type order,
                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                       QList<Tag> tags, QString linkedNotebookGuid, QUuid requestId)
{
    CHECK_PAUSED();

    bool userTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!userTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyTagsCompleted: flag = " << flag
                << ", limit = " << limit << ", offset = " << offset << ", order = " << order
                << ", orderDirection = " << orderDirection << ", linked notebook guid = " << linkedNotebookGuid
                << ", requestId = " << requestId);

        m_tags << tags;

        if (userTagsListCompleted) {
            QNTRACE("User's tags list is completed");
            m_listDirtyTagsRequestId = QUuid();
        }
        else {
            QNTRACE("Tags list is completed for one of linked notebooks");
            Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                    size_t limit, size_t offset,
                                                    LocalStorageManager::ListTagsOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!userTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyTagsFailed: flag = " << flag
                << ", limit = " << limit << ", offset = " << offset << ", order = "
                << order << ", orderDirection = " << orderDirection << ", linked notebook guid = "
                << linkedNotebookGuid << ", error description = " << errorDescription << ", requestId = " << requestId);

        if (userTagsListCompleted) {
            m_listDirtyTagsRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        emit failure(QT_TR_NOOP("Error listing dirty tags from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                                size_t limit, size_t offset,
                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QList<SavedSearch> savedSearches, QString linkedNotebookGuid, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onListDirtySavedSearchesCompleted: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid << ", requestId = " << requestId);

    m_savedSearches << savedSearches;
    m_listDirtySavedSearchesRequestId = QUuid();

    CHECK_STOPPED();

    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNWARNING("SendLocalChangesManager::onListDirtySavedSearchesFailed: flag = " << flag << ", limit = " << limit
              << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
              << ", linkedNotebookGuid = " << linkedNotebookGuid << ", errorDescription = " << errorDescription
              << ", requestId = " << requestId);

    m_listDirtySavedSearchesRequestId = QUuid();

    CHECK_STOPPED();

    emit failure(QT_TR_NOOP("Error listing dirty saved searches from local storage: ") + errorDescription);
}

void SendLocalChangesManager::onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                            size_t limit, size_t offset,
                                                            LocalStorageManager::ListNotebooksOrder::type order,
                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                            QList<Notebook> notebooks, QString linkedNotebookGuid, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!userNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotebooksCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notebooks << notebooks;

        if (userNotebooksListCompleted) {
            m_listDirtyNotebooksRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                         size_t limit, size_t offset,
                                                         LocalStorageManager::ListNotebooksOrder::type order,
                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                         QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!userNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyNotebooksFailed: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", errorDescription = " << errorDescription);

        if (userNotebooksListCompleted) {
            m_listDirtyNotebooksRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        emit failure(QT_TR_NOOP("Error listing dirty notebooks from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListNotesOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QList<Note> notes, QString linkedNotebookGuid, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!userNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotesCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notes << notes;

        if (userNotesListCompleted) {
            m_listDirtyNotesRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                                                     LocalStorageManager::ListNotesOrder::type order,
                                                     LocalStorageManager::OrderDirection::type orderDirection,
                                                     QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool userNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!userNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (userNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyNotesFailed: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        if (userNotesListCompleted) {
            m_listDirtyNotesRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.erase(it));
        }

        CHECK_STOPPED();

        emit failure(QT_TR_NOOP("Error listing dirty notes from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QList<LinkedNotebook> linkedNotebooks, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onListLinkedNotebooksCompleted: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
            << ", requestId = " << requestId);

    const int numLinkedNotebooks = linkedNotebooks.size();
    m_linkedNotebookGuidsAndShareKeys.reserve(std::max(numLinkedNotebooks, 0));

    QString shareKey;
    for(int i = 0; i < numLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = linkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QString error = QT_TR_NOOP("Found linked notebook without guid set in local storage");
            emit failure(error);
            QNWARNING(error << ", linked notebook: " << linkedNotebook);
            return;
        }

        shareKey.clear();
        if (linkedNotebook.hasShareKey()) {
            shareKey = linkedNotebook.shareKey();
        }

        m_linkedNotebookGuidsAndShareKeys << QPair<QString, QString>(linkedNotebook.guid(), shareKey);
    }

    m_listLinkedNotebooksRequestId = QUuid();
    CHECK_STOPPED();
    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                          size_t limit, size_t offset,
                                                          LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                          LocalStorageManager::OrderDirection::type orderDirection,
                                                          QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNWARNING("SendLocalChangesManager::onListLinkedNotebooksFailed: flag = " << flag << ", limit = " << limit
              << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
              << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    m_listLinkedNotebooksRequestId = QUuid();
    CHECK_STOPPED();

    emit failure(QT_TR_NOOP("Error listing linked notebooks from local storage: ") + errorDescription);
}

void SendLocalChangesManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateTagCompleted: tag = " << tag << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateTagRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateTagRequestIds.erase(it));
    CHECK_STOPPED();

    QString error = QT_TR_NOOP("Couldn't update tag in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateSavedSearchCompleted: search = " << savedSearch << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it));
    CHECK_STOPPED();

    QString error = QT_TR_NOOP("Couldn't update saved search in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateNotebookCompleted: notebook = " << notebook << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateNotebookRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateNotebookRequestIds.erase(it));
    CHECK_STOPPED();

    QString error = QT_TR_NOOP("Couldn't update notebook in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(notebook);

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateNoteCompleted: note = " << note << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateNoteRequestIds.erase(it));

    CHECK_STOPPED();
}

void SendLocalChangesManager::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook);

    CHECK_PAUSED();

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_updateNoteRequestIds.erase(it));
    CHECK_STOPPED();

    QString error = QT_TR_NOOP("Couldn't update note in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onFindNotebookCompleted: notebook = " << notebook << "\nrequest id = " << requestId);

    if (!notebook.hasGuid()) {
        QString errorDescription = QT_TR_NOOP("Internal error: found notebook without guid within notebooks requested "
                                              "from local storage by guid");
        QNWARNING(errorDescription << ", notebook: " << notebook);
        emit failure(errorDescription);
        return;
    }

    m_notebooksByGuidsCache[notebook.guid()] = notebook;
    Q_UNUSED(m_findNotebookRequestIds.erase(it));

    CHECK_STOPPED();

    if (m_findNotebookRequestIds.isEmpty()) {
        checkAndSendNotes();
    }
}

void SendLocalChangesManager::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    Q_UNUSED(m_findNotebookRequestIds.erase(it));
    CHECK_STOPPED();

    QString error = QT_TR_NOOP("Can't find notebook in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG("SendLocalChangesManager::timerEvent");

    if (!pEvent) {
        QString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    if (m_paused) {
        QNDEBUG("SendLocalChangesManager is being paused");
        killAllTimers();
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);
    QNDEBUG("Killed timer with id " << timerId);

    if (timerId == m_sendTagsPostponeTimerId)
    {
        m_sendTagsPostponeTimerId = 0;
        sendTags();
    }
    else if (timerId == m_sendSavedSearchesPostponeTimerId)
    {
        m_sendSavedSearchesPostponeTimerId = 0;
        sendSavedSearches();
    }
    else if (timerId == m_sendNotebooksPostponeTimerId)
    {
        m_sendNotebooksPostponeTimerId = 0;
        sendNotebooks();
    }
    else if (timerId == m_sendNotesPostponeTimerId)
    {
        m_sendNotesPostponeTimerId = 0;
        checkAndSendNotes();
    }
}

void SendLocalChangesManager::createConnections()
{
    QNDEBUG("SendLocalChangesManager::createConnections");

    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                           LocalStorageManager::ListTagsOrder::type,
                                                           LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListTagsRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                            LocalStorageManager::ListTagsOrder::type,
                                            LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                                    LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListNotesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListNotesRequest(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                             LocalStorageManager::ListNotesOrder::type,
                                             LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLinkedNotebooksList(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                       LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListLinkedNotebooksRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                       LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));

    QObject::connect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                             LocalStorageManager::ListTagsOrder::type,
                                             LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid)),
                     this,
                     SLOT(onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                   LocalStorageManager::ListTagsOrder::type,
                                                   LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                           LocalStorageManager::ListTagsOrder::type,
                                           LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                     this,
                     SLOT(onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListTagsOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListSavedSearchesOrder::type,
                                                      LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListSavedSearchesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListSavedSearchesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                  LocalStorageManager::ListNotebooksOrder::type,
                                                  LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListNotebooksOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListNotebooksOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListNotebooksOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesComplete(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                              LocalStorageManager::ListNotesOrder::type,
                                              LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListNotesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesFailed(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                            LocalStorageManager::ListNotesOrder::type,
                                            LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                 LocalStorageManager::ListNotesOrder::type,
                                                 LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listLinkedNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                        LocalStorageManager::OrderDirection::type,
                                                        QList<LinkedNotebook>,QString,QUuid)),
                     this,
                     SLOT(onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,
                                                         QList<LinkedNotebook>,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                      LocalStorageManager::OrderDirection::type,
                                                      QString,QString,QUuid)),
                     this,
                     SLOT(onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                      LocalStorageManager::OrderDirection::type,
                                                      QString,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)), this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));

    m_connectedToLocalStorage = true;
}

void SendLocalChangesManager::disconnectFromLocalStorage()
{
    QNDEBUG("SendLocalChangesManager::disconnectFromLocalStorage");

    // Disconnect local signals from localStorageManagerThread's slots
    QObject::disconnect(this,
                        SIGNAL(requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                              LocalStorageManager::ListTagsOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid)),
                        &m_localStorageManagerThreadWorker,
                        SLOT(onListTagsRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                            LocalStorageManager::ListTagsOrder::type,
                                            LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::disconnect(this,
                        SIGNAL(requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                       LocalStorageManager::ListSavedSearchesOrder::type,
                                                                       LocalStorageManager::OrderDirection::type,QString,QUuid)),
                        &m_localStorageManagerThreadWorker,
                        SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListSavedSearchesOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::disconnect(this,
                        SIGNAL(requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                   LocalStorageManager::ListNotebooksOrder::type,
                                                                   LocalStorageManager::OrderDirection::type,QString,QUuid)),
                        &m_localStorageManagerThreadWorker,
                        SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListSavedSearchesOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::disconnect(this,
                        SIGNAL(requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                               LocalStorageManager::ListNotesOrder::type,
                                                               LocalStorageManager::OrderDirection::type,QString,QUuid)),
                        &m_localStorageManagerThreadWorker,
                        SLOT(onListNotesRequest(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                LocalStorageManager::ListNotesOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::disconnect(this,
                        SIGNAL(requestLinkedNotebooksList(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                          LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                          LocalStorageManager::OrderDirection::type,QString,QUuid)),
                        &m_localStorageManagerThreadWorker,
                        SLOT(onListLinkedNotebooksRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                          LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                          LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::disconnect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::disconnect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::disconnect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));

    QObject::disconnect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));

    // Disconnect localStorageManagerThread's signals from local slots
    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listTagsComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListTagsOrder::type,
                                                LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid)),
                        this,
                        SLOT(onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListTagsOrder::type,
                                                      LocalStorageManager::OrderDirection::type,QList<Tag>,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                              LocalStorageManager::ListTagsOrder::type,
                                              LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                        this,
                        SLOT(onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                   LocalStorageManager::ListTagsOrder::type,
                                                   LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listSavedSearchesComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListSavedSearchesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid)),
                        this,
                        SLOT(onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                               LocalStorageManager::ListSavedSearchesOrder::type,
                                                               LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listSavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                       LocalStorageManager::ListSavedSearchesOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                        this,
                        SLOT(onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListSavedSearchesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListNotebooksOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid)),
                        this,
                        SLOT(onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                           LocalStorageManager::ListNotebooksOrder::type,
                                                           LocalStorageManager::OrderDirection::type,QList<Notebook>,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                   LocalStorageManager::ListNotebooksOrder::type,
                                                   LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                        this,
                        SLOT(onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListNotebooksOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listNotesComplete(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                                 LocalStorageManager::ListNotesOrder::type,
                                                 LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid)),
                        this,
                        SLOT(onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                       LocalStorageManager::ListNotesOrder::type,
                                                       LocalStorageManager::OrderDirection::type,QList<Note>,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listNotesFailed(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                               LocalStorageManager::ListNotesOrder::type,
                                               LocalStorageManager::OrderDirection::type,QString,QString,QUuid)),
                        this,
                        SLOT(onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListNotesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QString,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listLinkedNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                           LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                           LocalStorageManager::OrderDirection::type,
                                                           QList<LinkedNotebook>,QString,QUuid)),
                        this,
                        SLOT(onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                            LocalStorageManager::OrderDirection::type,
                                                            QList<LinkedNotebook>,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        SIGNAL(listLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,
                                                         QString,QString,QUuid)),
                        this,
                        SLOT(onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListLinkedNotebooksOrder::type,
                                                         LocalStorageManager::OrderDirection::type,
                                                         QString,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)), this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onFindNotebookFailed(Notebook,QString,QUuid)));

    m_connectedToLocalStorage = false;
}

void SendLocalChangesManager::requestStuffFromLocalStorage(const QString & linkedNotebookGuid)
{
    QNDEBUG("SendLocalChangesManager::requestStuffFromLocalStorage: linked notebook guid = " << linkedNotebookGuid);

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    bool emptyLinkedNotebookGuid = linkedNotebookGuid.isEmpty();

    // WARNING: this flag assumes that all local-but-not-yet-synchronized objects would have dirty flag set!
    LocalStorageManager::ListObjectsOptions listDirtyObjectsFlag =
            LocalStorageManager::ListDirty | LocalStorageManager::ListNonLocal;

    size_t limit = 0, offset = 0;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

    LocalStorageManager::ListTagsOrder::type tagsOrder = LocalStorageManager::ListTagsOrder::NoOrder;
    QUuid listDirtyTagsRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyTagsRequestId = listDirtyTagsRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyTagsFromLinkedNotebooksRequestIds.insert(listDirtyTagsRequestId));
    }
    emit requestLocalUnsynchronizedTags(listDirtyObjectsFlag, limit, offset, tagsOrder,
                                        orderDirection, linkedNotebookGuid, listDirtyTagsRequestId);

    if (emptyLinkedNotebookGuid)
    {
        LocalStorageManager::ListSavedSearchesOrder::type savedSearchesOrder = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
        m_listDirtySavedSearchesRequestId = QUuid::createUuid();
        emit requestLocalUnsynchronizedSavedSearches(listDirtyObjectsFlag, limit, offset, savedSearchesOrder,
                orderDirection, linkedNotebookGuid, m_listDirtySavedSearchesRequestId);
    }

    LocalStorageManager::ListNotebooksOrder::type notebooksOrder = LocalStorageManager::ListNotebooksOrder::NoOrder;
    QUuid listDirtyNotebooksRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyNotebooksRequestId = listDirtyNotebooksRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyNotebooksFromLinkedNotebooksRequestIds.insert(listDirtyNotebooksRequestId));
    }
    emit requestLocalUnsynchronizedNotebooks(listDirtyObjectsFlag, limit, offset, notebooksOrder,
                                             orderDirection, linkedNotebookGuid, listDirtyNotebooksRequestId);

    LocalStorageManager::ListNotesOrder::type notesOrder = LocalStorageManager::ListNotesOrder::NoOrder;
    QUuid listDirtyNotesRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyNotesRequestId = listDirtyNotesRequestId;
    }
    else {
        Q_UNUSED(m_listDirtyNotesFromLinkedNotebooksRequestIds.insert(listDirtyNotesRequestId));
    }
    emit requestLocalUnsynchronizedNotes(listDirtyObjectsFlag, limit, offset, notesOrder,
                                         orderDirection, linkedNotebookGuid, listDirtyNotesRequestId);

    if (emptyLinkedNotebookGuid)
    {
        LocalStorageManager::ListObjectsOptions linkedNotebooksListOption = LocalStorageManager::ListObjectsOption::ListAll;
        LocalStorageManager::ListLinkedNotebooksOrder::type linkedNotebooksOrder = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
        m_listLinkedNotebooksRequestId = QUuid::createUuid();
        emit requestLinkedNotebooksList(linkedNotebooksListOption, limit, offset, linkedNotebooksOrder, orderDirection, m_listLinkedNotebooksRequestId);
    }
}

void SendLocalChangesManager::checkListLocalStorageObjectsCompletion()
{
    QNDEBUG("SendLocalChangesManager::checkListLocalStorageObjectsCompletion");

    if (!m_listDirtyTagsRequestId.isNull()) {
        QNTRACE("The last request for the list of new and dirty tags was not processed yet");
        return;
    }

    if (!m_listDirtySavedSearchesRequestId.isNull()) {
        QNTRACE("The last request for the list of new and dirty saved searches was not processed yet");
        return;
    }

    if (!m_listDirtyNotebooksRequestId.isNull()) {
        QNTRACE("The last request for the list of new and dirty notebooks was not processed yet");
        return;
    }

    if (!m_listDirtyNotesRequestId.isNull()) {
        QNTRACE("The last request for the list of new and dirty notes was not processed yet");
        return;
    }

    if (!m_listLinkedNotebooksRequestId.isNull()) {
        QNTRACE("The last request for the list of linked notebooks was not processed yet");
        return;
    }

    m_receivedDirtyLocalStorageObjectsFromUsersAccount = true;
    QNTRACE("Received all dirty objects from user's own account from local storage");
    emit receivedUserAccountDirtyObjects();

    if ((m_lastProcessedLinkedNotebookGuidIndex < 0) && (m_linkedNotebookGuidsAndShareKeys.size() > 0))
    {
        QNTRACE("There are " << m_linkedNotebookGuidsAndShareKeys.size() << " linked notebook guids, need to receive the dirty objects from them as well");

        const int numLinkedNotebookGuids = m_linkedNotebookGuidsAndShareKeys.size();
        for(int i = 0; i < numLinkedNotebookGuids; ++i)
        {
            const QPair<QString, QString> & guidAndShareKey = m_linkedNotebookGuidsAndShareKeys[i];
            requestStuffFromLocalStorage(guidAndShareKey.first);
        }

        return;
    }
    else if (m_linkedNotebookGuidsAndShareKeys.size() > 0)
    {
        if (!m_listDirtyTagsFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list tags from linked notebooks from local storage: "
                    << m_listDirtyTagsFromLinkedNotebooksRequestIds.size());
            return;
        }

        if (!m_listDirtyNotebooksFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list notebooks from linked notebooks from local storage: "
                    << m_listDirtyNotebooksFromLinkedNotebooksRequestIds.size());
            return;
        }

        if (!m_listDirtyNotesFromLinkedNotebooksRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list notes from linked notebooks from local storage: "
                    << m_listDirtyNotesFromLinkedNotebooksRequestIds.size());
            return;
        }
    }

    m_receivedAllDirtyLocalStorageObjects = true;
    QNTRACE("All relevant objects from local storage have been listed");
    emit receivedAllDirtyObjects();

    sendLocalChanges();
}

void SendLocalChangesManager::sendLocalChanges()
{
    QNDEBUG("SendLocalChangesManager::sendLocalChanges");

    if (!checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
        return;
    }

    // TODO: consider checking the expiration time of user's own authentication token here

    // IMPROVEME: it would be very nice to have a possibility to check whether the attempt to send tags/saved searches/notebooks
    // had to delay itself due to rate limit exceeding or problems with authentication token expiration; should such event occur,
    // there's no much sense in trying to send another stuff to Evernote service, it would respond with the same exception again
    sendTags();
    sendSavedSearches();
    sendNotebooks();

    findNotebooksForNotes();
}

void SendLocalChangesManager::sendTags()
{
    QNDEBUG("SendLocalChangesManager::sendTags");

    QString errorDescription;

    typedef QList<Tag>::iterator Iter;
    for(Iter it = m_tags.begin(); it != m_tags.end(); )
    {
        Tag & tag = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        QString linkedNotebookAuthToken;
        if (tag.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
            if (cit != m_authenticationTokensByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value();
            }
            else
            {
                errorDescription = QT_TR_NOOP("Couldn't find authentication token for linked notebook when attempting to create or update a tag from it");
                QNWARNING(errorDescription << ", tag: " << tag);

                auto sit = std::find_if(m_linkedNotebookGuidsAndShareKeys.begin(),
                                        m_linkedNotebookGuidsAndShareKeys.end(),
                                        CompareGuidAndShareKeyByGuid(tag.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndShareKeys.end()) {
                    QNWARNING("The linked notebook the tag refers to was not found within the list of linked notebooks "
                              "received from local storage!");
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingTag = !tag.hasUpdateSequenceNumber();
        if (creatingTag) {
            QNTRACE("Sending new tag: " << tag);
            errorCode = m_noteStore.createTag(tag, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE("Sending modified tag: " << tag);
            errorCode = m_noteStore.updateTag(tag, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Caught RATE_LIMIT_REACHED exception but "
                                              "the number of seconds to wait is negative: ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendTagsPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!tag.hasLinkedNotebookGuid())
            {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("Internal error: couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << ", linked notebook guid = " << tag.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("Unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << ", linked notebook guid = " << tag.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO("Encountered DATA_CONFLICT exception while trying to send new and/or modified tags, "
                   "it means the incremental sync should be repeated before sending the changes to the service");
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0) {
            QString errorPrefix = QT_TR_NOOP("Can't send new and/or modified tags to the service: ");
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE("Checking if we are still in sync with the remote service");

            if (!tag.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("Internal error: tag's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = nullptr;
            if (!tag.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE("Current tag does not belong to linked notebook");
            }
            else
            {
                QNTRACE("Current tag belongs to linked notebook with guid " << tag.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(tag.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("Internal error: can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (tag.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = tag.updateSequenceNumber();
                QNTRACE("The client is in sync with the service; updated corresponding last update count to " << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE("The client is not in sync with the service");
            }
        }

        it = m_tags.erase(it);
    }
}

void SendLocalChangesManager::sendSavedSearches()
{
    QNDEBUG("SendLocalChangesManager::sendSavedSearches");

    QString errorDescription;

    typedef QList<SavedSearch>::iterator Iter;
    for(Iter it = m_savedSearches.begin(); it != m_savedSearches.end(); )
    {
        SavedSearch & search = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        bool creatingSearch = !search.hasUpdateSequenceNumber();
        if (creatingSearch) {
            QNTRACE("Sending new saved search: " << search);
            errorCode = m_noteStore.createSavedSearch(search, errorDescription, rateLimitSeconds);
        }
        else {
            QNTRACE("Sending modified saved search: " << search);
            errorCode = m_noteStore.updateSavedSearch(search, errorDescription, rateLimitSeconds);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Caught RATE_LIMIT_REACHED exception but "
                                              "the number of seconds to wait is negative: ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
                                              "due to trta limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendSavedSearchesPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            handleAuthExpiration();
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO("Encountered DATA_CONFLICT exception while trying to send new and/or modified saved searches, "
                   "it means the incremental sync should be repeated before sending the changes to the service");
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0) {
            QString errorPrefix = QT_TR_NOOP("Can't send new and/or modified tags to the service: ");
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE("Checking if we are still in sync with the remote service");

            if (!search.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("Internal error: saved search's update sequence number is not set "
                                              "after being send to the service");
                emit failure(errorDescription);
                return;
            }

            if (search.updateSequenceNumber() == m_lastUpdateCount + 1) {
                m_lastUpdateCount = search.updateSequenceNumber();
                QNTRACE("The client is in sync with the service; updated last update count to " << m_lastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE("The client is not in sync with the service");
            }
        }

        it = m_savedSearches.erase(it);
    }
}

void SendLocalChangesManager::sendNotebooks()
{
    QNDEBUG("SendLocalChangesManager::sendNotebooks");

    QString errorDescription;

    typedef QList<Notebook>::iterator Iter;
    for(Iter it = m_notebooks.begin(); it != m_notebooks.end(); )
    {
        Notebook & notebook = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        QString linkedNotebookAuthToken;
        if (notebook.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
            if (cit != m_authenticationTokensByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value();
            }
            else
            {
                errorDescription = QT_TR_NOOP("Couldn't find authenticaton token for linked notebook when attempting to create or update a notebook");
                QNWARNING(errorDescription << ", notebook: " << notebook);

                auto sit = std::find_if(m_linkedNotebookGuidsAndShareKeys.begin(),
                                        m_linkedNotebookGuidsAndShareKeys.end(),
                                        CompareGuidAndShareKeyByGuid(notebook.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndShareKeys.end()) {
                    QNWARNING("The linked notebook the notebook refers to was not found within the list of linked notebooks "
                              "received from local storage");
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingNotebook = !notebook.hasUpdateSequenceNumber();
        if (creatingNotebook) {
            QNTRACE("Sending new notebook: " << notebook);
            errorCode = m_noteStore.createNotebook(notebook, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE("Sending modified notebook: " << notebook);
            errorCode = m_noteStore.updateNotebook(notebook, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Caught RATE_LIMIT_REACHED exception but "
                                              "the number of seconds to wait is negative");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendNotebooksPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!notebook.hasLinkedNotebookGuid()) {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("Internal error: couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << ", linked notebook guid = " << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("Unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << ", linked notebook guid = " << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO("Encountered DATA_CONFLICT exception while trying to send new and/or modified notebooks, "
                   "it means the incremental sync should be repeated before sending the changes to the service");
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0) {
            QString errorPrefix = QT_TR_NOOP("Can't send new and/or mofidied notebooks to the service: ");
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE("Checking if we are still in sync with the remote service");

            if (!notebook.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("Internal error: notebook's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = nullptr;
            if (!notebook.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE("Current notebook does not belong to linked notebook");
            }
            else
            {
                QNTRACE("Current notebook belongs to linked notebook with guid " << notebook.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("Internal error: can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (notebook.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = notebook.updateSequenceNumber();
                QNTRACE("The client is in sync with the service; updated last update count to " << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE("The client is not in sync with the service");
            }
        }

        it = m_notebooks.erase(it);
    }
}

void SendLocalChangesManager::checkAndSendNotes()
{
    QNDEBUG("SendLocalChangesManager::checkAndSendNotes");

    if (m_tags.isEmpty() && m_notebooks.isEmpty() && m_findNotebookRequestIds.isEmpty()) {
        sendNotes();
    }
}

void SendLocalChangesManager::sendNotes()
{
    QNDEBUG("SendLocalChangesManager::sendNotes");

    QString errorDescription;

    typedef QList<Note>::iterator Iter;
    for(Iter it = m_notes.begin(); it != m_notes.end(); )
    {
        Note & note = *it;

        errorDescription.clear();
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = qevercloud::EDAMErrorCode::UNKNOWN;

        if (!note.hasNotebookGuid()) {
            errorDescription = QT_TR_NOOP("Internal error: found note without notebook guid set");
            QNWARNING(errorDescription << ", note: " << note);
            emit failure(errorDescription);
            return;
        }

        auto nit = m_notebooksByGuidsCache.find(note.notebookGuid());
        if (nit == m_notebooksByGuidsCache.end()) {
            errorDescription = QT_TR_NOOP("Internal error: can't find notebook for one of notes to be sent to the remote service");
            QNWARNING(errorDescription << ", note: " << note);
            emit failure(errorDescription);
            return;
        }

        const Notebook & notebook = nit.value();

        QString linkedNotebookAuthToken;
        if (notebook.hasLinkedNotebookGuid())
        {
            auto cit = m_authenticationTokensByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
            if (cit != m_authenticationTokensByLinkedNotebookGuid.end())
            {
                linkedNotebookAuthToken = cit.value();
            }
            else
            {
                errorDescription = QT_TR_NOOP("Couldn't find authenticaton token for linked notebook when attempting to create or update a notebook");
                QNWARNING(errorDescription << ", notebook: " << notebook);

                auto sit = std::find_if(m_linkedNotebookGuidsAndShareKeys.begin(),
                                        m_linkedNotebookGuidsAndShareKeys.end(),
                                        CompareGuidAndShareKeyByGuid(notebook.linkedNotebookGuid()));
                if (sit == m_linkedNotebookGuidsAndShareKeys.end()) {
                    QNWARNING("The linked notebook the notebook refers to was not found within the list of linked notebooks "
                              "received from local storage");
                }

                emit failure(errorDescription);
                return;
            }
        }

        bool creatingNote = !note.hasUpdateSequenceNumber();
        if (creatingNote) {
            QNTRACE("Sending new note: " << note);
            errorCode = m_noteStore.createNote(note, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }
        else {
            QNTRACE("Sending modified note: " << note);
            errorCode = m_noteStore.updateNote(note, errorDescription, rateLimitSeconds, linkedNotebookAuthToken);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Caught RATE_LIMIT_REACHED exception but "
                                              "the number of seconds to wait is negative");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
                                              "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_sendNotesPostponeTimerId = timerId;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!notebook.hasLinkedNotebookGuid()) {
                handleAuthExpiration();
            }
            else
            {
                auto cit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (cit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end())
                {
                    errorDescription = QT_TR_NOOP("Internal error: couldn't find the expiration time of linked notebook auth token");
                    QNWARNING(errorDescription << ", linked notebook guid = " << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
                else if (checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
                    errorDescription = QT_TR_NOOP("Unexpected AUTH_EXPIRED error: authentication tokens for all linked notebooks "
                                                  "are still valid");
                    QNWARNING(errorDescription << ", linked notebook guid = " << notebook.linkedNotebookGuid());
                    emit failure(errorDescription);
                }
            }

            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
        {
            QNINFO("Encountered DATA_CONFLICT exception while trying to send new and/or modified notes, "
                   "it means the incremental sync should be repeated before sending the changes to the service");
            emit conflictDetected();
            pause();
            return;
        }
        else if (errorCode != 0) {
            QString errorPrefix = QT_TR_NOOP("Can't send new and/or mofidied notes to the service: ");
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
            return;
        }

        if (!m_shouldRepeatIncrementalSync)
        {
            QNTRACE("Checking if we are still in sync with the remote service");

            if (!note.hasUpdateSequenceNumber()) {
                errorDescription = QT_TR_NOOP("Internal error: note's update sequence number is not set after it being sent to the service");
                emit failure(errorDescription);
                return;
            }

            int * pLastUpdateCount = nullptr;
            if (!notebook.hasLinkedNotebookGuid())
            {
                pLastUpdateCount = &m_lastUpdateCount;
                QNTRACE("Current note does not belong to linked notebook");
            }
            else
            {
                QNTRACE("Current note belongs to linked notebook with guid " << notebook.linkedNotebookGuid());

                auto lit = m_lastUpdateCountByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
                if (lit == m_lastUpdateCountByLinkedNotebookGuid.end()) {
                    errorDescription = QT_TR_NOOP("Internal error: can't find update count per linked notebook guid on attempt to check "
                                                  "the update count of tag sent to the service");
                    emit failure(errorDescription);
                    return;
                }

                pLastUpdateCount = &lit.value();
            }

            if (note.updateSequenceNumber() == *pLastUpdateCount + 1) {
                *pLastUpdateCount = note.updateSequenceNumber();
                QNTRACE("The client is in sync with the service; updated last update count to " << *pLastUpdateCount);
            }
            else {
                m_shouldRepeatIncrementalSync = true;
                emit shouldRepeatIncrementalSync();
                QNTRACE("The client is not in sync with the service");
            }
        }

        it = m_notes.erase(it);
    }

    // If we got here, we are actually done
    QNINFO("Sent all locally added/updated notes back to the Evernote service");
    finalize();
}

void SendLocalChangesManager::findNotebooksForNotes()
{
    QNDEBUG("SendLocalChangesManager::findNotebooksForNotes");

    m_findNotebookRequestIds.clear();
    QSet<QString> notebookGuids;

    // NOTE: enforce limited scope for CIter typedef
    {
        auto notebooksByGuidsCacheEnd = m_notebooksByGuidsCache.end();

        typedef QList<Note>::const_iterator CIter;
        CIter notesEnd = m_notes.end();
        for(CIter it = m_notes.begin(); it != notesEnd; ++it)
        {
            const Note & note = *it;
            if (!note.hasNotebookGuid()) {
                continue;
            }

            auto nit = m_notebooksByGuidsCache.find(note.notebookGuid());
            if (nit == notebooksByGuidsCacheEnd) {
                Q_UNUSED(notebookGuids.insert(note.notebookGuid()));
            }
        }
    }

    if (!notebookGuids.isEmpty())
    {
        Notebook dummyNotebook;
        dummyNotebook.unsetLocalGuid();

        typedef QSet<QString>::const_iterator CIter;
        CIter notebookGuidsEnd = notebookGuids.end();
        for(CIter it = notebookGuids.begin(); it != notebookGuidsEnd; ++it)
        {
            const QString & notebookGuid = *it;
            dummyNotebook.setGuid(notebookGuid);

            QUuid requestId = QUuid::createUuid();
            emit findNotebook(dummyNotebook, requestId);
            Q_UNUSED(m_findNotebookRequestIds.insert(requestId));

            QNTRACE("Sent find notebook request for notebook guid " << notebookGuid << ", request id = " << requestId);
        }
    }
    else
    {
        checkAndSendNotes();
    }
}

bool SendLocalChangesManager::rateLimitIsActive() const
{
    return ( (m_sendTagsPostponeTimerId > 0) ||
             (m_sendSavedSearchesPostponeTimerId > 0) ||
             (m_sendNotebooksPostponeTimerId > 0) ||
             (m_sendNotesPostponeTimerId > 0) );
}

bool SendLocalChangesManager::hasPendingRequests() const
{
    QUuid emptyId;
    return ( (m_listDirtyTagsRequestId != emptyId) ||
             (m_listDirtySavedSearchesRequestId != emptyId) ||
             (m_listDirtyNotebooksRequestId != emptyId) ||
             (m_listDirtyNotesRequestId != emptyId) ||
             (m_listLinkedNotebooksRequestId != emptyId) ||
             !m_listDirtyTagsFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_listDirtyNotebooksFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_listDirtyNotesFromLinkedNotebooksRequestIds.isEmpty() ||
             !m_updateTagRequestIds.isEmpty() ||
             !m_updateSavedSearchRequestIds.isEmpty() ||
             !m_updateNotebookRequestIds.isEmpty() ||
             !m_updateNoteRequestIds.isEmpty() ||
             !m_findNotebookRequestIds.isEmpty() );
}

void SendLocalChangesManager::finalize()
{
    emit finished(m_lastUpdateCount, m_lastUpdateCountByLinkedNotebookGuid);
    clear();
    disconnectFromLocalStorage();
}

void SendLocalChangesManager::clear()
{
    QNDEBUG("SendLocalChangesManager::clear");

    m_lastUpdateCount = 0;
    m_lastUpdateCountByLinkedNotebookGuid.clear();

    m_shouldRepeatIncrementalSync = false;
    m_paused = false;
    m_requestedToStop = false;

    m_receivedDirtyLocalStorageObjectsFromUsersAccount = false;
    m_receivedAllDirtyLocalStorageObjects = false;

    QUuid emptyId;
    m_listDirtyTagsRequestId = emptyId;
    m_listDirtySavedSearchesRequestId = emptyId;
    m_listDirtyNotebooksRequestId = emptyId;
    m_listDirtyNotesRequestId = emptyId;
    m_listLinkedNotebooksRequestId = emptyId;

    m_listDirtyTagsFromLinkedNotebooksRequestIds.clear();
    m_listDirtyNotebooksFromLinkedNotebooksRequestIds.clear();
    m_listDirtyNotesFromLinkedNotebooksRequestIds.clear();

    m_tags.clear();
    m_savedSearches.clear();
    m_notebooks.clear();
    m_notes.clear();

    m_linkedNotebookGuidsAndShareKeys.clear();
    m_lastProcessedLinkedNotebookGuidIndex = -1;

    // NOTE: don't clear auth tokens by linked notebook guid as well as their expiration timestamps, these might be useful later on

    m_updateTagRequestIds.clear();
    m_updateSavedSearchRequestIds.clear();
    m_updateNotebookRequestIds.clear();
    m_updateNoteRequestIds.clear();

    m_findNotebookRequestIds.clear();
    m_notebooksByGuidsCache.clear();    // NOTE: don't get any ideas on preserving the cache, it can easily get stale
                                        // especially when disconnected from local storage

    killAllTimers();
}

void SendLocalChangesManager::killAllTimers()
{
    QNDEBUG("SendLocalChangesManager::killAllTimers");

    if (m_sendTagsPostponeTimerId > 0) {
        killTimer(m_sendTagsPostponeTimerId);
    }
    m_sendTagsPostponeTimerId = 0;

    if (m_sendSavedSearchesPostponeTimerId > 0) {
        killTimer(m_sendSavedSearchesPostponeTimerId);
    }
    m_sendSavedSearchesPostponeTimerId = 0;

    if (m_sendNotebooksPostponeTimerId > 0) {
        killTimer(m_sendNotebooksPostponeTimerId);
    }
    m_sendNotebooksPostponeTimerId = 0;

    if (m_sendNotesPostponeTimerId > 0) {
        killTimer(m_sendNotesPostponeTimerId);
    }
    m_sendNotesPostponeTimerId = 0;
}

bool SendLocalChangesManager::checkAndRequestAuthenticationTokensForLinkedNotebooks()
{
    QNDEBUG("SendLocalChangesManager::checkAndRequestAuthenticationTokensForLinkedNotebooks");

    if (m_linkedNotebookGuidsAndShareKeys.isEmpty()) {
        QNDEBUG("The list of linked notebook guids and share keys is empty");
        return true;
    }

    const int numLinkedNotebookGuids = m_linkedNotebookGuidsAndShareKeys.size();
    for(int i = 0; i < numLinkedNotebookGuids; ++i)
    {
        const QPair<QString, QString> & guidAndShareKey = m_linkedNotebookGuidsAndShareKeys[i];
        const QString & guid = guidAndShareKey.first;
        if (guid.isEmpty()) {
            QString error = QT_TR_NOOP("Internal error: found empty linked notebook guid within the list of linked notebook guids and share keys");
            QNWARNING(error);
            emit failure(error);
            return false;
        }

        auto it = m_authenticationTokensByLinkedNotebookGuid.find(guid);
        if (it == m_authenticationTokensByLinkedNotebookGuid.end()) {
            QNDEBUG("Authentication token for linked notebook with guid " << guid
                    << " was not found; will request authentication tokens for all linked notebooks at once");
            emit requestAuthenticationTokensForLinkedNotebooks(m_linkedNotebookGuidsAndShareKeys);
            return false;
        }

        auto eit = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(guid);
        if (eit == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end()) {
            QString error = QT_TR_NOOP("Internal inconsistency detected: can't find cached "
                                       "expiration time of linked notebook's authentication token");
            QNWARNING(error << ", linked notebook guid = " << guid);
            emit failure(error);
            return false;
        }

        const qevercloud::Timestamp & expirationTime = eit.value();
        const qevercloud::Timestamp currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - expirationTime < SIX_HOURS_IN_MSEC) {
            QNDEBUG("Authentication token for linked notebook with guid " << guid
                    << " is too close to expiration: its expiration time is " << PrintableDateTimeFromTimestamp(expirationTime)
                    << ", current time is " << PrintableDateTimeFromTimestamp(currentTime)
                    << "; will request new authentication tokens for all linked notebooks");
            emit requestAuthenticationTokensForLinkedNotebooks(m_linkedNotebookGuidsAndShareKeys);
            return false;
        }
    }

    QNDEBUG("Got authentication tokens for all linked notebooks, can proceed with "
            "their synchronization");

    return true;
}

void SendLocalChangesManager::handleAuthExpiration()
{
    QNINFO("Got AUTH_EXPIRED error, pausing and requesting new authentication token");
    m_paused = true;
    emit paused(/* pending authentication = */ true);
    emit requestAuthenticationToken();
}

} // namespace qute_note

