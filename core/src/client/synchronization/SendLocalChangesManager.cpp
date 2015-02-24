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
    m_linkedNotebookGuidByListDirtyTagsRequestIds(),
    m_linkedNotebookGuidByListDirtyNotebooksRequestIds(),
    m_linkedNotebookGuidByListDirtyNotesRequestIds(),
    m_tags(),
    m_savedSearches(),
    m_notebooks(),
    m_notes(),
    m_linkedNotebookGuids(),
    m_lastProcessedLinkedNotebookGuidIndex(-1),
    m_updateTagRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_updateNotebookRequestIds(),
    m_updateNoteRequestIds()
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
    auto it = m_linkedNotebookGuidByListDirtyTagsRequestIds.end();
    if (!ownTagsListCompleted) {
        it = m_linkedNotebookGuidByListDirtyTagsRequestIds.find(requestId);
    }

    if (ownTagsListCompleted || (it != m_linkedNotebookGuidByListDirtyTagsRequestIds.end()))
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
            Q_UNUSED(m_linkedNotebookGuidByListDirtyTagsRequestIds.erase(it));
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
    auto it = m_linkedNotebookGuidByListDirtyTagsRequestIds.end();
    if (!ownTagsListCompleted) {
        it = m_linkedNotebookGuidByListDirtyTagsRequestIds.find(requestId);
    }

    if (ownTagsListCompleted || (it != m_linkedNotebookGuidByListDirtyTagsRequestIds.end()))
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
    auto it = m_linkedNotebookGuidByListDirtyNotebooksRequestIds.end();
    if (!ownNotebooksListCompleted) {
        it = m_linkedNotebookGuidByListDirtyNotebooksRequestIds.find(requestId);
    }

    if (ownNotebooksListCompleted || (it != m_linkedNotebookGuidByListDirtyNotebooksRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotebooksCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notebooks << notebooks;

        if (ownNotebooksListCompleted) {
            m_listDirtyNotebooksRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_linkedNotebookGuidByListDirtyNotebooksRequestIds.erase(it));
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
    auto it = m_linkedNotebookGuidByListDirtyNotebooksRequestIds.end();
    if (!ownNotebooksListCompleted) {
        it = m_linkedNotebookGuidByListDirtyNotebooksRequestIds.find(requestId);
    }

    if (ownNotebooksListCompleted || (it != m_linkedNotebookGuidByListDirtyNotebooksRequestIds.end()))
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
    auto it = m_linkedNotebookGuidByListDirtyNotesRequestIds.end();
    if (!ownNotesListCompleted) {
        it = m_linkedNotebookGuidByListDirtyNotesRequestIds.find(requestId);
    }

    if (ownNotesListCompleted || (it != m_linkedNotebookGuidByListDirtyNotesRequestIds.end()))
    {
        QNDEBUG("SendLocalChangesManager::onListDirtyNotesCompleted: flag = " << flag << ", limit = " << limit
                << ", offset = " << offset << ", order = " << order << ", orderDirection = " << orderDirection
                << ", linkedNotebookGuid = " << linkedNotebookGuid << ", requestId = " << requestId);

        m_notes << notes;

        if (ownNotesListCompleted) {
            m_listDirtyNotesRequestId = QUuid();
        }
        else {
            Q_UNUSED(m_linkedNotebookGuidByListDirtyNotesRequestIds.erase(it));
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
    auto it = m_linkedNotebookGuidByListDirtyNotesRequestIds.end();
    if (!ownNotesListCompleted) {
        it = m_linkedNotebookGuidByListDirtyNotesRequestIds.find(requestId);
    }

    if (ownNotesListCompleted || (it != m_linkedNotebookGuidByListDirtyNotesRequestIds.end()))
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
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));
    // TODO: continue from here

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
        m_linkedNotebookGuidByListDirtyTagsRequestIds[listDirtyTagsRequestId] = linkedNotebookGuid;
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
        m_linkedNotebookGuidByListDirtyNotebooksRequestIds[listDirtyNotebooksRequestId] = linkedNotebookGuid;
    }
    emit requestLocalUnsynchronizedNotebooks(listDirtyObjectsFlag, limit, offset, notebooksOrder,
                                             orderDirection, linkedNotebookGuid, listDirtyNotebooksRequestId);

    LocalStorageManager::ListNotesOrder::type notesOrder = LocalStorageManager::ListNotesOrder::NoOrder;
    QUuid listDirtyNotesRequestId = QUuid::createUuid();
    if (emptyLinkedNotebookGuid) {
        m_listDirtyNotesRequestId = listDirtyNotesRequestId;
    }
    else {
        m_linkedNotebookGuidByListDirtyNotesRequestIds[listDirtyNotesRequestId] = linkedNotebookGuid;
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
        if (!m_linkedNotebookGuidByListDirtyTagsRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list tags per linked notebook from local storage: "
                    << m_linkedNotebookGuidByListDirtyTagsRequestIds.size());
            return;
        }

        if (!m_linkedNotebookGuidByListDirtyNotebooksRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list notebooks per linked notebook from local storage: "
                    << m_linkedNotebookGuidByListDirtyNotebooksRequestIds.size());
            return;
        }

        if (!m_linkedNotebookGuidByListDirtyNotesRequestIds.isEmpty()) {
            QNTRACE("There are pending requests to list notes per linked notebook from local storage: "
                    << m_linkedNotebookGuidByListDirtyNotesRequestIds.size());
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

    // NOTE: notes can only be sent after both tags and notebooks are done
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

} // namespace qute_note
