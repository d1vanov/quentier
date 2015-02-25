#include "SendLocalChangesManager.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {

SendLocalChangesManager::SendLocalChangesManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                 QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                 QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_noteStore(pNoteStore),
    m_connectedToLocalStorage(false),
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
    m_linkedNotebookGuids(),
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

void SendLocalChangesManager::start()
{
    QNDEBUG("SendLocalChangesManager::start");

    QString dummyLinkedNotebookGuid;
    requestStuffFromLocalStorage(dummyLinkedNotebookGuid);
}

void SendLocalChangesManager::onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                       size_t limit, size_t offset,
                                                       LocalStorageManager::ListTagsOrder::type order,
                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                       QList<Tag> tags, QString linkedNotebookGuid, QUuid requestId)
{
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

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                    size_t limit, size_t offset,
                                                    LocalStorageManager::ListTagsOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
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

        emit failure(QT_TR_NOOP("Error listing dirty tags from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                                size_t limit, size_t offset,
                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QList<SavedSearch> savedSearches, QString linkedNotebookGuid, QUuid requestId)
{
    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onListDirtySavedSearchesCompleted: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
            << ", linked notebook guid = " << linkedNotebookGuid << ", requestId = " << requestId);

    m_savedSearches << savedSearches;
    m_listDirtySavedSearchesRequestId = QUuid();
    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    if (requestId != m_listDirtySavedSearchesRequestId) {
        return;
    }

    QNWARNING("SendLocalChangesManager::onListDirtySavedSearchesFailed: flag = " << flag << ", limit = " << limit
              << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
              << ", linkedNotebookGuid = " << linkedNotebookGuid << ", errorDescription = " << errorDescription
              << ", requestId = " << requestId);

    emit failure(QT_TR_NOOP("Error listing dirty saved searches from local storage: ") + errorDescription);
}

void SendLocalChangesManager::onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                            size_t limit, size_t offset,
                                                            LocalStorageManager::ListNotebooksOrder::type order,
                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                            QList<Notebook> notebooks, QString linkedNotebookGuid, QUuid requestId)
{
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

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                         size_t limit, size_t offset,
                                                         LocalStorageManager::ListNotebooksOrder::type order,
                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                         QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
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

        emit failure(QT_TR_NOOP("Error listing dirty notebooks from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListNotesOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QList<Note> notes, QString linkedNotebookGuid, QUuid requestId)
{
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

        checkListLocalStorageObjectsCompletion();
    }
}

void SendLocalChangesManager::onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                                                     LocalStorageManager::ListNotesOrder::type order,
                                                     LocalStorageManager::OrderDirection::type orderDirection,
                                                     QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
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

        emit failure(QT_TR_NOOP("Error listing dirty notes from local storage: ") + errorDescription);
    }
}

void SendLocalChangesManager::onListLinkedNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QList<LinkedNotebook> linkedNotebooks, QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onListLinkedNotebooksCompleted: flag = " << flag << ", limit = " << limit
            << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
            << ", requestId = " << requestId);

    const int numLinkedNotebooks = linkedNotebooks.size();
    m_linkedNotebookGuids.reserve(std::max(numLinkedNotebooks, 0));

    for(int i = 0; i < numLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = linkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QString error = QT_TR_NOOP("Found linked notebook without guid set in local storage");
            emit failure(error);
            QNWARNING(error << ", linked notebook: " << linkedNotebook);
            return;
        }

        m_linkedNotebookGuids << linkedNotebook.guid();
    }

    m_listLinkedNotebooksRequestId = QUuid();
    checkListLocalStorageObjectsCompletion();
}

void SendLocalChangesManager::onListLinkedNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                          size_t limit, size_t offset,
                                                          LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                          LocalStorageManager::OrderDirection::type orderDirection,
                                                          QString errorDescription, QUuid requestId)
{
    if (requestId != m_listLinkedNotebooksRequestId) {
        return;
    }

    QNWARNING("SendLocalChangesManager::onListLinkedNotebooksFailed: flag = " << flag << ", limit = " << limit
              << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
              << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    emit failure(QT_TR_NOOP("Error listing linked notebooks from local storage: ") + errorDescription);
}

void SendLocalChangesManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateTagCompleted: tag = " << tag << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateTagRequestIds.erase(it));
}

void SendLocalChangesManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    auto it = m_updateTagRequestIds.find(requestId);
    if (it == m_updateTagRequestIds.end()) {
        return;
    }

    QString error = QT_TR_NOOP("Couldn't update tag in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateSavedSearchCompleted: search = " << savedSearch << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateSavedSearchRequestIds.erase(it));
}

void SendLocalChangesManager::onUpdateSavedSearchFailed(SavedSearch savedSearch, QString errorDescription, QUuid requestId)
{
    auto it = m_updateSavedSearchRequestIds.find(requestId);
    if (it == m_updateSavedSearchRequestIds.end()) {
        return;
    }

    QString error = QT_TR_NOOP("Couldn't update saved search in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateNotebookCompleted: notebook = " << notebook << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateNotebookRequestIds.erase(it));
}

void SendLocalChangesManager::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto it = m_updateNotebookRequestIds.find(requestId);
    if (it == m_updateNotebookRequestIds.end()) {
        return;
    }

    QString error = QT_TR_NOOP("Couldn't update notebook in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook);

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("SendLocalChangesManager::onUpdateNoteCompleted: note = " << note << "\nrequest id = " << requestId);
    Q_UNUSED(m_updateNoteRequestIds.erase(it));
}

void SendLocalChangesManager::onUpdateNoteFailed(Note note, Notebook notebook, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook);

    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QString error = QT_TR_NOOP("Couldn't update note in local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void SendLocalChangesManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
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

    // TODO: check if can send notes now
}

void SendLocalChangesManager::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    QNWARNING("SendLocalChangesManager::onFindNotebookFailed: notebook = " << notebook << "\nerrorDescription = "
              << errorDescription << ", request id = " << requestId);

    QString error = QT_TR_NOOP("Can't find notebook in local storage: ");
    error += errorDescription;
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

    QNTRACE("Received all dirty objects from user's own account from local storage");

    if ((m_lastProcessedLinkedNotebookGuidIndex < 0) && (m_linkedNotebookGuids.size() > 0))
    {
        QNTRACE("There are " << m_linkedNotebookGuids.size() << " linked notebook guids, need to receive the dirty objects from them as well");

        const int numLinkedNotebookGuids = m_linkedNotebookGuids.size();
        for(int i = 0; i < numLinkedNotebookGuids; ++i)
        {
            const QString & guid = m_linkedNotebookGuids[i];
            requestStuffFromLocalStorage(guid);
        }

        return;
    }
    else if (m_linkedNotebookGuids.size() > 0)
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

    QNTRACE("All relevant objects from local storage have been listed");

    sendLocalChanges();
}

void SendLocalChangesManager::sendLocalChanges()
{
    QNDEBUG("SendLocalChangesManager::sendLocalChanges");

    sendTags();
    sendSavedSearches();
    sendNotebooks();

    // NOTE: notes can only be sent after both tags and notebooks are sent + when notebooks for all notes are found
}

void SendLocalChangesManager::sendTags()
{
    // TODO: implement
}

void SendLocalChangesManager::sendSavedSearches()
{
    // TODO: implement
}

void SendLocalChangesManager::sendNotebooks()
{
    // TODO: implement
}

void SendLocalChangesManager::sendNotes()
{
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
        // TODO: check if can send notes now
    }
}

bool SendLocalChangesManager::rateLimitIsActive() const
{
    return ( (m_sendTagsPostponeTimerId > 0) ||
             (m_sendSavedSearchesPostponeTimerId > 0) ||
             (m_sendNotebooksPostponeTimerId > 0) ||
             (m_sendNotesPostponeTimerId > 0) );
}

} // namespace qute_note
