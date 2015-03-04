#include "SendLocalChangesManager.h"
#include <client/Utility.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {

SendLocalChangesManager::SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                 QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                 QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_noteStore(pNoteStore),
    m_lastUpdateCount(0),
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

void SendLocalChangesManager::start(const qint32 lastUpdateCount)
{
    QNDEBUG("SendLocalChangesManager::start: last update count = " << lastUpdateCount);

    m_lastUpdateCount = lastUpdateCount;

    if (m_paused) {
        resume();
        return;
    }

    QString dummyLinkedNotebookGuid;
    requestStuffFromLocalStorage(dummyLinkedNotebookGuid);
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

        // TODO: think of more fine-grained resuming from pause
        if (m_receivedAllDirtyLocalStorageObjects) {
            sendLocalChanges();
        }
        else {
            start(m_lastUpdateCount);
        }
    }
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

    bool ownTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!ownTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyTagsCompleted: flag = " << flag
                << ", limit = " << limit << ", offset = " << offset << ", order = " << order
                << ", orderDirection = " << orderDirection << ", linked notebook guid = " << linkedNotebookGuid
                << ", requestId = " << requestId);

        m_tags << tags;

        if (ownTagsListCompleted) {
            m_listDirtyTagsRequestId = QUuid();
        }
        else {
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

    bool ownTagsListCompleted = (requestId == m_listDirtyTagsRequestId);
    auto it = m_listDirtyTagsFromLinkedNotebooksRequestIds.end();
    if (!ownTagsListCompleted) {
        it = m_listDirtyTagsFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownTagsListCompleted || (it != m_listDirtyTagsFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyTagsFailed: flag = " << flag
                << ", limit = " << limit << ", offset = " << offset << ", order = "
                << order << ", orderDirection = " << orderDirection << ", linked notebook guid = "
                << linkedNotebookGuid << ", error description = " << errorDescription << ", requestId = " << requestId);

        if (ownTagsListCompleted) {
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

    bool ownNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!ownNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotebooksCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notebooks << notebooks;

        if (ownNotebooksListCompleted) {
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

    bool ownNotebooksListCompleted = (requestId == m_listDirtyNotebooksRequestId);
    auto it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end();
    if (!ownNotebooksListCompleted) {
        it = m_listDirtyNotebooksFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownNotebooksListCompleted || (it != m_listDirtyNotebooksFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyNotebooksFailed: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", errorDescription = " << errorDescription);

        if (ownNotebooksListCompleted) {
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

    bool ownNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!ownNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotesCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notes << notes;

        if (ownNotesListCompleted) {
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

    bool ownNotesListCompleted = (requestId == m_listDirtyNotesRequestId);
    auto it = m_listDirtyNotesFromLinkedNotebooksRequestIds.end();
    if (!ownNotesListCompleted) {
        it = m_listDirtyNotesFromLinkedNotebooksRequestIds.find(requestId);
    }

    if (ownNotesListCompleted || (it != m_listDirtyNotesFromLinkedNotebooksRequestIds.end()))
    {
        QNWARNING("SendLocalChangesManager::onListDirtyNotesFailed: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        if (ownNotesListCompleted) {
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

    LocalStorageManager::ListObjectsOptions listDirtyObjectsFlag =
            LocalStorageManager::ListDirty | LocalStorageManager::ListNonLocal;

    size_t limit = 0, offset = 0;
    LocalStorageManager::ListTagsOrder::type tagsOrder = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

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

    sendLocalChanges();
}

void SendLocalChangesManager::sendLocalChanges()
{
    QNDEBUG("SendLocalChangesManager::sendLocalChanges");

    // TODO: check that all required authentication tokens are present and that they are good to use (don't expire soon)

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

        bool creatingTag = !tag.hasUpdateSequenceNumber();
        if (creatingTag) {
            QNTRACE("Sending new tag: " << tag);
            errorCode = m_noteStore.createTag(tag, errorDescription, rateLimitSeconds);
        }
        else {
            QNTRACE("Sending modified tag: " << tag);
            errorCode = m_noteStore.updateTag(tag, errorDescription, rateLimitSeconds);
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
                QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
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
            QString errorPrefix = (creatingTag
                                   ? QT_TR_NOOP("Unexpected AUTH_EXPIRED error when trying to create tag in the service: ")
                                   : QT_TR_NOOP("Unexpected AUTH_EXPIRED error when trying to update tag in the service: "));
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
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

            if (tag.updateSequenceNumber() == m_lastUpdateCount + 1) {
                m_lastUpdateCount = tag.updateSequenceNumber();
                QNTRACE("The client is in sync with the service; updated last update count to " << m_lastUpdateCount);
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
    // TODO: implement
}

void SendLocalChangesManager::sendNotebooks()
{
    // TODO: implement
}

void SendLocalChangesManager::checkAndSendNotes()
{
    QNDEBUG("SendLocalChangesManager::checkAndSendNotes");

    // TODO: think thoroughly other possible conditions not letting one to start sending the modified notes
    if (m_tags.isEmpty() && m_notebooks.isEmpty()) {
        sendNotes();
    }
}

void SendLocalChangesManager::sendNotes()
{
    QNDEBUG("SendLocalChangesManager::sendNotes");

    // TODO: implement
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
    // TODO: implement
    return false;
}

void SendLocalChangesManager::finalize()
{
    // TODO: implement
}

} // namespace qute_note

