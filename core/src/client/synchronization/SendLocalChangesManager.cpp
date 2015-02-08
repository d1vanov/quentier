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
    m_listDirtyNotesRequestId()
{}

void SendLocalChangesManager::start()
{
    QNDEBUG("SendLocalChangesManager::start");

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    QNTRACE("Starting from listing the non-local dirty tags");
    LocalStorageManager::ListObjectsOptions listDirtyObjectsFlag =
            LocalStorageManager::ListDirty | LocalStorageManager::ListNonLocal;

    size_t limit = 0, offset = 0;
    LocalStorageManager::ListTagsOrder::type tagsOrder = LocalStorageManager::ListTagsOrder::NoOrder;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

    m_listDirtyTagsRequestId = QUuid::createUuid();
    emit requestLocalUnsynchronizedTags(listDirtyObjectsFlag, limit, offset, tagsOrder,
                                        orderDirection, m_listDirtyTagsRequestId);

    LocalStorageManager::ListSavedSearchesOrder::type savedSearchesOrder = LocalStorageManager::ListSavedSearchesOrder::NoOrder;
    m_listDirtySavedSearchesRequestId = QUuid::createUuid();
    emit requestLocalUnsynchronizedSavedSearches(listDirtyObjectsFlag, limit, offset, savedSearchesOrder,
                                                 orderDirection, m_listDirtySavedSearchesRequestId);

    LocalStorageManager::ListNotebooksOrder::type notebooksOrder = LocalStorageManager::ListNotebooksOrder::NoOrder;
    m_listDirtyNotebooksRequestId = QUuid::createUuid();
    emit requestLocalUnsynchronizedNotebooks(listDirtyObjectsFlag, limit, offset, notebooksOrder,
                                             orderDirection, m_listDirtyNotebooksRequestId);

    LocalStorageManager::ListNotesOrder::type notesOrder = LocalStorageManager::ListNotesOrder::NoOrder;
    m_listDirtyNotesRequestId = QUuid::createUuid();
    emit requestLocalUnsynchronizedNotes(listDirtyObjectsFlag, limit, offset, notesOrder,
                                         orderDirection, m_listDirtyNotesRequestId);

    QNTRACE("Emitted signals to local storage to request new and updates tags, saved searches, notebooks and notes");
}

void SendLocalChangesManager::onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                       size_t limit, size_t offset,
                                                       LocalStorageManager::ListTagsOrder::type order,
                                                       LocalStorageManager::OrderDirection::type orderDirection,
                                                       QList<Tag> tags, QUuid requestId)
{
    QNDEBUG("SendLocalChangesManager::onListDirtyTagsCompleted: flag = " << flag
            << ", limit = " << limit << ", offset = " << offset << ", order = " << order
            << ", orderDirection = " << orderDirection << ", requestId = " << requestId);

    const int numTags = tags.size();
    for(int i = 0; i < numTags; ++i)
    {
        Tag & tag = tags[i];

        if (!tag.hasUpdateSequenceNumber())
        {
            // The tag is new, need to create it in the remote service
            // TODO: call NoteStore::createTag
        }

        // TODO: continue from here
    }
}

void SendLocalChangesManager::onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                                                    size_t limit, size_t offset,
                                                    LocalStorageManager::ListTagsOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString errorDescription, QUuid requestId)
{
    QNWARNING("SendLocalChangesManager::onListDirtyTagsFailed: flag = " << flag
              << ", limit = " << limit << ", offset = " << offset << ", order = "
              << order << ", orderDirection = " << orderDirection << ", error description = "
              << errorDescription << ", requestId = " << requestId);

    emit failure("Error listing dirty tags from local storage: " + errorDescription);
}

void SendLocalChangesManager::onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                                size_t limit, size_t offset,
                                                                LocalStorageManager::ListSavedSearchesOrder::type order,
                                                                LocalStorageManager::OrderDirection::type orderDirection,
                                                                QList<SavedSearch> savedSearches, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(savedSearches)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions flag,
                                                             size_t limit, size_t offset,
                                                             LocalStorageManager::ListSavedSearchesOrder::type order,
                                                             LocalStorageManager::OrderDirection::type orderDirection,
                                                             QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                            size_t limit, size_t offset,
                                                            LocalStorageManager::ListNotebooksOrder::type order,
                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                            QList<Notebook> notebooks, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(notebooks)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                                         size_t limit, size_t offset,
                                                         LocalStorageManager::ListNotebooksOrder::type order,
                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                         QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions flag,
                                                        size_t limit, size_t offset,
                                                        LocalStorageManager::ListNotesOrder::type order,
                                                        LocalStorageManager::OrderDirection::type orderDirection,
                                                        QList<Note> notes, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(notes)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order, LocalStorageManager::OrderDirection::type orderDirection, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void SendLocalChangesManager::createConnections()
{
    QNDEBUG("SendLocalChangesManager::createConnections");

    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedTags(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                           LocalStorageManager::ListTagsOrder::type,
                                                           LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListTagsRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                            LocalStorageManager::ListTagsOrder::type,
                                            LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedSavedSearches(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                                    LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotebooks(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListSavedSearchesRequest(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListSavedSearchesOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QUuid)));

    QObject::connect(this,
                     SIGNAL(requestLocalUnsynchronizedNotes(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListNotesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QUuid)),
                     &m_localStorageManagerThreadWorker,
                     SLOT(onListNotesRequest(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                             LocalStorageManager::ListNotesOrder::type,
                                             LocalStorageManager::OrderDirection::type,QUuid)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                             LocalStorageManager::ListTagsOrder::type,
                                             LocalStorageManager::OrderDirection::type,QList<Tag>,QUuid)),
                     this,
                     SLOT(onListDirtyTagsCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                   LocalStorageManager::ListTagsOrder::type,
                                                   LocalStorageManager::OrderDirection::type,QList<Tag>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                           LocalStorageManager::ListTagsOrder::type,
                                           LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyTagsFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListTagsOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                      LocalStorageManager::ListSavedSearchesOrder::type,
                                                      LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                            LocalStorageManager::ListSavedSearchesOrder::type,
                                                            LocalStorageManager::OrderDirection::type,QList<SavedSearch>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listSavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListSavedSearchesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtySavedSearchesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                         LocalStorageManager::ListSavedSearchesOrder::type,
                                                         LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksComplete(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                  LocalStorageManager::ListNotebooksOrder::type,
                                                  LocalStorageManager::OrderDirection::type,QList<Notebook>,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                        LocalStorageManager::ListNotebooksOrder::type,
                                                        LocalStorageManager::OrderDirection::type,QList<Notebook>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                LocalStorageManager::ListNotebooksOrder::type,
                                                LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotebooksFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                     LocalStorageManager::ListNotebooksOrder::type,
                                                     LocalStorageManager::OrderDirection::type,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesComplete(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                              LocalStorageManager::ListNotesOrder::type,
                                              LocalStorageManager::OrderDirection::type,QList<Note>,QUuid)),
                     this,
                     SLOT(onListDirtyNotesCompleted(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                    LocalStorageManager::ListNotesOrder::type,
                                                    LocalStorageManager::OrderDirection::type,QList<Note>,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     SIGNAL(listNotesFailed(LocalStorageManager::ListObjectsOptions,bool,size_t,size_t,
                                            LocalStorageManager::ListNotesOrder::type,
                                            LocalStorageManager::OrderDirection::type,QString,QUuid)),
                     this,
                     SLOT(onListDirtyNotesFailed(LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                 LocalStorageManager::ListNotesOrder::type,
                                                 LocalStorageManager::OrderDirection::type,QString,QUuid)));

    m_connectedToLocalStorage = true;
}

void SendLocalChangesManager::disconnectFromLocalStorage()
{
    // TODO: implement

    m_connectedToLocalStorage = false;
}

} // namespace qute_note
