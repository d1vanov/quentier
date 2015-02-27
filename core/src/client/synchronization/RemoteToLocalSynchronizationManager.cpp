#include "RemoteToLocalSynchronizationManager.h"
#include <client/Utility.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <client/types/ResourceAdapter.h>
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QTimerEvent>
#include <algorithm>

namespace qute_note {

RemoteToLocalSynchronizationManager::RemoteToLocalSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                                         QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                                         QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_connectedToLocalStorage(false),
    m_noteStore(pNoteStore),
    m_maxSyncChunkEntries(50),
    m_lastSyncMode(SyncMode::FullSync),
    m_lastSyncTime(0),
    m_lastUpdateCount(0),
    m_onceSyncDone(false),
    m_lastUsnOnStart(-1),
    m_lastSyncChunksDownloadedUsn(-1),
    m_syncChunksDownloaded(false),
    m_fullNoteContentsDownloaded(false),
    m_expungedFromServerToClient(false),
    m_linkedNotebooksSyncChunksDownloaded(false),
    m_active(false),
    m_paused(false),
    m_requestedToStop(false),
    m_syncChunks(),
    m_linkedNotebookSyncChunks(),
    m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded(),
    m_tags(),
    m_expungedTags(),
    m_tagsToAddPerRequestId(),
    m_findTagByNameRequestIds(),
    m_findTagByGuidRequestIds(),
    m_addTagRequestIds(),
    m_updateTagRequestIds(),
    m_expungeTagRequestIds(),
    m_linkedNotebookGuidsByTagGuids(),
    m_expungeNotelessTagsRequestId(),
    m_savedSearches(),
    m_expungedSavedSearches(),
    m_savedSearchesToAddPerRequestId(),
    m_findSavedSearchByNameRequestIds(),
    m_findSavedSearchByGuidRequestIds(),
    m_addSavedSearchRequestIds(),
    m_updateSavedSearchRequestIds(),
    m_expungeSavedSearchRequestIds(),
    m_linkedNotebooks(),
    m_expungedLinkedNotebooks(),
    m_findLinkedNotebookRequestIds(),
    m_addLinkedNotebookRequestIds(),
    m_updateLinkedNotebookRequestIds(),
    m_expungeLinkedNotebookRequestIds(),
    m_authenticationTokensByLinkedNotebookGuid(),
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid(),
    m_syncStatesByLinkedNotebookGuid(),
    m_lastSynchronizedUsnByLinkedNotebookGuid(),
    m_lastSyncTimeByLinkedNotebookGuid(),
    m_lastUpdateCountByLinkedNotebookGuid(),
    m_notebooks(),
    m_expungedNotebooks(),
    m_notebooksToAddPerRequestId(),
    m_findNotebookByNameRequestIds(),
    m_findNotebookByGuidRequestIds(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds(),
    m_expungeNotebookRequestIds(),
    m_linkedNotebookGuidsByNotebookGuids(),
    m_notes(),
    m_expungedNotes(),
    m_findNoteByGuidRequestIds(),
    m_addNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_expungeNoteRequestIds(),
    m_notesWithFindRequestIdsPerFindNotebookRequestId(),
    m_notebooksPerNoteGuids(),
    m_resources(),
    m_findResourceByGuidRequestIds(),
    m_addResourceRequestIds(),
    m_updateResourceRequestIds(),
    m_resourcesWithFindRequestIdsPerFindNoteRequestId(),
    m_resourceFoundFlagPerFindResourceRequestId(),
    m_notesPerResourceGuids(),
    m_resourceConflictedAndRemoteNotesPerNotebookGuid(),
    m_findNotebookForNotesWithConflictedResourcesRequestIds(),
    m_localGuidsOfElementsAlreadyAttemptedToFindByName(),
    m_notesToAddPerAPICallPostponeTimerId(),
    m_notesToUpdatePerAPICallPostponeTimerId(),
    m_afterUsnForSyncChunkPerAPICallPostponeTimerId(),
    m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId(),
    m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId(),
    m_getSyncStateBeforeStartAPICallPostponeTimerId(0),
    m_gotLastSyncParameters(false)
{}

bool RemoteToLocalSynchronizationManager::active() const
{
    return m_active;
}

void RemoteToLocalSynchronizationManager::start(qint32 afterUsn)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::start: afterUsn = " << afterUsn);

    m_lastUsnOnStart = afterUsn;

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused || m_requestedToStop) {
        resume();   // NOTE: resume can call this method so it's necessary to return
        return;
    }

    if (!m_gotLastSyncParameters) {
        emit requestLastSyncParameters();
        return;
    }

    clear();

    m_lastSyncMode = ((afterUsn == 0)
                      ? SyncMode::FullSync
                      : SyncMode::IncrementalSync);

    if (m_onceSyncDone || (afterUsn != 0))
    {
        bool asyncWait = false;
        bool error = false;

        // check the sync state of user's own account, this may produce the asynchronous chain of events or some error
        bool res = checkUserAccountSyncState(asyncWait, error, afterUsn);
        if (error || asyncWait) {
            return;
        }

        if (!res)
        {
            QNTRACE("The service has no updates for user's own account, need to check for updates from linked notebooks");
            res = checkLinkedNotebooksSyncStates(asyncWait, error);
            if (asyncWait || error) {
                return;
            }

            if (!res) {
                QNTRACE("The service has no updates for any of linked notebooks");
                finalize();
            }
        }
        // Otherwise the sync of all linked notebooks from user's account would start after the sync of user's account
        // (Because the sync of user's account can bring in the new linked notebooks or remove any of them)
    }

    m_active = true;
    downloadSyncChunksAndLaunchSync(afterUsn);
}

void RemoteToLocalSynchronizationManager::stop()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::stop");
    m_requestedToStop = true;
    emit stopped();
}

void RemoteToLocalSynchronizationManager::pause()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::pause");
    m_paused = true;
    emit paused(/* pending authentication = */ false);
}

void RemoteToLocalSynchronizationManager::resume()
{
    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused || m_requestedToStop)
    {
        if (m_paused) {
            m_paused = false;
        }

        if (m_requestedToStop) {
            m_requestedToStop = false;
        }

        if (m_syncChunksDownloaded && (m_lastSyncChunksDownloadedUsn >= m_lastUsnOnStart))
        {
            QNDEBUG("last usn for which sync chunks were downloaded is " << m_lastSyncChunksDownloadedUsn
                    << ", there's no need to download the sync chunks again, launching the sync procedure");

            if (m_fullNoteContentsDownloaded)
            {
                QNDEBUG("Full note's contents have already been downloaded meaning that the sync for "
                        "tags, saved searches, notebooks and notes from user's account is over");

                if (m_linkedNotebooksSyncChunksDownloaded) {
                    QNDEBUG("sync chunks for linked notebooks were already downloaded, there's no need to "
                            "do it again, will launch the sync for linked notebooks");
                    launchLinkedNotebooksContentsSync();
                }
                else {
                    startLinkedNotebooksSync();
                }
            }
            else
            {
                launchSync();
            }
        }
        else
        {
            start(m_lastUsnOnStart);
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<Tag>(const Tag & tag)
{
    QUuid addTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_addTagRequestIds.insert(addTagRequestId));
    emit addTag(tag, addTagRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<SavedSearch>(const SavedSearch & search)
{
    QUuid addSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_addSavedSearchRequestIds.insert(addSavedSearchRequestId));
    emit addSavedSearch(search, addSavedSearchRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook)
{
    QUuid addLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addLinkedNotebookRequestIds.insert(addLinkedNotebookRequestId));
    emit addLinkedNotebook(linkedNotebook, addLinkedNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<Notebook>(const Notebook & notebook)
{
    QUuid addNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNotebookRequestIds.insert(addNotebookRequestId));
    emit addNotebook(notebook, addNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<Note>(const Note & note)
{
    QString noteGuid = (note.hasGuid() ? note.guid() : QString());
    QString noteLocalGuid = note.localGuid();
    QPair<QString,QString> key(noteGuid, noteLocalGuid);

    auto it = m_notebooksPerNoteGuids.find(key);
    if (it == m_notebooksPerNoteGuids.end()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to add note for which no notebook was found in local storage");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return;
    }

    const Notebook & notebook = it.value();

    QUuid addNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
    emit addNote(note, notebook, addNoteRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<ResourceWrapper>(const ResourceWrapper & resource)
{
    QString resourceGuid = (resource.hasGuid() ? resource.guid() : QString());
    QString resourceLocalGuid = resource.localGuid();
    QPair<QString,QString> key(resourceGuid, resourceLocalGuid);

    auto it = m_notesPerResourceGuids.find(key);
    if (it == m_notesPerResourceGuids.end()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to add resource for which no note was found in local storage");
        QNWARNING(errorDescription << ": " << resource);
        emit failure(errorDescription);
        return;
    }

    const Note & note = it.value();

    QUuid addResourceRequestId = QUuid::createUuid();
    Q_UNUSED(m_addResourceRequestIds.insert(addResourceRequestId));
    emit addResource(resource, note, addResourceRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Tag>(const Tag & tag,
                                                                 const Tag * tagToAddLater)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::emitUpdateRequest<Tag>: tag = " << tag
            << ", tagToAddLater = " << (tagToAddLater ? tagToAddLater->ToQString() : "<null>"));

    QUuid updateTagRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateTagRequestIds.insert(updateTagRequestId));

    if (tagToAddLater) {
        m_tagsToAddPerRequestId[updateTagRequestId] = *tagToAddLater;
    }

    emit updateTag(tag, updateTagRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<SavedSearch>(const SavedSearch & search,
                                                                         const SavedSearch * searchToAddLater)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::emitUpdateRequest<SavedSearch>: search = " << search
            << ", searchToAddLater = " << (searchToAddLater ? searchToAddLater->ToQString() : "<null>"));

    QUuid updateSavedSearchRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateSavedSearchRequestIds.insert(updateSavedSearchRequestId));

    if (searchToAddLater) {
        m_savedSearchesToAddPerRequestId[updateSavedSearchRequestId] = *searchToAddLater;
    }

    emit updateSavedSearch(search, updateSavedSearchRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<LinkedNotebook>(const LinkedNotebook & linkedNotebook,
                                                                            const LinkedNotebook *)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::emitUpdateRequest<LinkedNotebook>: linked notebook = "
            << linkedNotebook);

    QUuid updateLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateLinkedNotebookRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateLinkedNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Notebook>(const Notebook & notebook,
                                                                      const Notebook * notebookToAddLater)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::emitUpdateRequest<Notebook>: notebook = " << notebook
            << ", notebook to add later = " << (notebookToAddLater ? notebookToAddLater->ToQString() : "null"));

    QUuid updateNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(updateNotebookRequestId));

    if (notebookToAddLater) {
        m_notebooksToAddPerRequestId[updateNotebookRequestId] = *notebookToAddLater;
    }

    emit updateNotebook(notebook, updateNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Note>(const Note & note,
                                                                  const Note *)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::emitUpdateRequest<Note>: note = " << note);

    const Notebook * pNotebook = getNotebookPerNote(note);
    if (!pNotebook) {
        QString errorDescription = QT_TR_NOOP("Detected attempt to update note in local storage before its notebook is found");
        QNWARNING(errorDescription << ": " << note);
        emit failure(errorDescription);
        return;
    }

    // Dealing with separate Evernote API call for getting the content of note to be updated
    Note localNote = note;
    QString errorDescription;

    qint32 postponeAPICallSeconds = tryToGetFullNoteData(localNote, errorDescription);
    if (postponeAPICallSeconds < 0)
    {
        return;
    }
    else if (postponeAPICallSeconds > 0)
    {
        int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
        if (timerId == 0) {
            QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                  "the Evernote API call due to rate limit exceeding");
            emit failure(errorDescription);
            return;
        }

        m_notesToUpdatePerAPICallPostponeTimerId[timerId] = localNote;
        emit rateLimitExceeded(postponeAPICallSeconds);
        return;
    }

    QUuid updateNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));

    emit updateNote(localNote, *pNotebook, updateNoteRequestId);
}

#define CHECK_PAUSED() \
    if (m_paused && !m_requestedToStop) { \
        QNDEBUG("RemoteTolocalSynchronizationManager is being paused, returning without any actions"); \
        return; \
    }

#define CHECK_STOPPED() \
    if (m_requestedToStop && !hasPendingRequests()) { \
        QNDEBUG("RemoteToLocalSynchronizationManager is requested to stop and has no pending requests, " \
                "finishing the synchronization"); \
        finalize(); \
    }

void RemoteToLocalSynchronizationManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(notebook, requestId, "Notebook",
                                              m_notebooks, m_findNotebookByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(notebook, requestId, "Notebook",
                                              m_notebooks, m_findNotebookByNameRequestIds);
    if (foundByName) {
        CHECK_STOPPED();
        return;
    }

    NoteDataPerFindNotebookRequestId::iterator rit = m_notesWithFindRequestIdsPerFindNotebookRequestId.find(requestId);
    if (rit != m_notesWithFindRequestIdsPerFindNotebookRequestId.end())
    {
        const QPair<Note,QUuid> & noteWithFindRequestId = rit.value();
        const Note & note = noteWithFindRequestId.first;
        const QUuid & findNoteRequestId = noteWithFindRequestId.second;

        QString noteGuid = (note.hasGuid() ? note.guid() : QString());
        QString noteLocalGuid = note.localGuid();

        QPair<QString,QString> key(noteGuid, noteLocalGuid);

        // NOTE: notebook for notes is only required for its pair of guid + local guid,
        // it shouldn't prohibit the creation or update of the notes during the synchronization procedure
        notebook.setCanCreateNotes(true);
        notebook.setCanUpdateNotes(true);

        m_notebooksPerNoteGuids[key] = notebook;

        Q_UNUSED(onFoundDuplicateByGuid(note, findNoteRequestId, "Note", m_notes, m_findNoteByGuidRequestIds));
        CHECK_STOPPED();
        return;
    }

    QSet<QUuid>::iterator nit = m_findNotebookForNotesWithConflictedResourcesRequestIds.find(requestId);
    if (nit != m_findNotebookForNotesWithConflictedResourcesRequestIds.end())
    {
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.erase(nit));

        CHECK_STOPPED();

        if (!notebook.hasGuid()) {
            QString errorDescription = QT_TR_NOOP("Found notebook in local storage which doesn't have guid set");
            QNWARNING(errorDescription << ", notebook: " << notebook);
            emit failure(errorDescription);
            return;
        }

        QHash<QString,QPair<Note,Note> >::iterator notesPerNotebookGuidIt = m_resourceConflictedAndRemoteNotesPerNotebookGuid.find(notebook.guid());
        if (notesPerNotebookGuidIt == m_resourceConflictedAndRemoteNotesPerNotebookGuid.end()) {
            QString errorDescription = QT_TR_NOOP("Internal error: unable to figure out for which notes the notebook was requested to be found");
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        const QPair<Note,Note> & notesPair = notesPerNotebookGuidIt.value();
        const Note & conflictedNote = notesPair.first;
        const Note & updatedNote = notesPair.second;

        QString conflictedNoteLocalGuid = conflictedNote.localGuid();
        QString updatedNoteLocalGuid = updatedNote.localGuid();

        if (!conflictedNoteLocalGuid.isEmpty() || conflictedNote.hasGuid()) {
            QPair<QString,QString> key;
            key.first = (conflictedNote.hasGuid() ? conflictedNote.guid() : QString());
            key.second = conflictedNoteLocalGuid;

            m_notebooksPerNoteGuids[key] = notebook;
        }

        if (!updatedNoteLocalGuid.isEmpty() || updatedNote.hasGuid()) {
            QPair<QString,QString> key;
            key.first = (updatedNote.hasGuid() ? updatedNote.guid() : QString());
            key.second = updatedNoteLocalGuid;

            m_notebooksPerNoteGuids[key] = notebook;
        }

        QUuid updateNoteRequestId = QUuid::createUuid();
        Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));
        emit updateNote(updatedNote, notebook, updateNoteRequestId);

        QUuid addNoteRequestId = QUuid::createUuid();
        Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
        emit addNote(conflictedNote, notebook, addNoteRequestId);

        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindNotebookFailed(Notebook notebook, QString errorDescription,
                                                               QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(notebook, requestId, errorDescription, "Notebook",
                                                  m_notebooks, m_findNotebookByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(notebook, requestId, errorDescription, "Notebook",
                                                  m_notebooks, m_findNotebookByNameRequestIds);
    if (failedToFindByName) {
        CHECK_STOPPED();
        return;
    }

    NoteDataPerFindNotebookRequestId::iterator rit = m_notesWithFindRequestIdsPerFindNotebookRequestId.find(requestId);
    if (rit != m_notesWithFindRequestIdsPerFindNotebookRequestId.end())
    {
        QString errorDescription = QT_TR_NOOP("Failed to find notebook in local storage for one of processed notes");
        QNWARNING(errorDescription << ": " << notebook);
        emit failure(errorDescription);
        return;
    }

    QSet<QUuid>::iterator nit = m_findNotebookForNotesWithConflictedResourcesRequestIds.find(requestId);
    if (nit != m_findNotebookForNotesWithConflictedResourcesRequestIds.end())
    {
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.erase(nit));

        CHECK_STOPPED();

        QString errorDescription = QT_TR_NOOP("Could not find notebook for update of note which should resolve the conflict "
                                              "of individual resource");
        QNWARNING(errorDescription << ", notebook attempted to be found: " << notebook);
        emit failure(errorDescription);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindNoteCompleted(Note note, bool withResourceBinaryData, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator it = m_findNoteByGuidRequestIds.find(requestId);
    if (it != m_findNoteByGuidRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onFindNoteCompleted: note = " << note
                << ", requestId = " << requestId);

        // NOTE: erase is required for proper work of the macro; the request would be re-inserted below if macro doesn't return from the method
        Q_UNUSED(m_findNoteByGuidRequestIds.erase(it));

        CHECK_STOPPED();

        // Need to find Notebook corresponding to the note in order to proceed
        if (!note.hasNotebookGuid()) {
            QString errorDescription = QT_TR_NOOP("Found duplicate note in local storage which doesn't have notebook guid set");
            QNWARNING(errorDescription << ": " << note);
            emit failure(errorDescription);
            return;
        }

        QUuid findNotebookPerNoteRequestId = QUuid::createUuid();
        m_notesWithFindRequestIdsPerFindNotebookRequestId[findNotebookPerNoteRequestId] =
                QPair<Note,QUuid>(note, requestId);

        Notebook notebookToFind;
        notebookToFind.unsetLocalGuid();
        notebookToFind.setGuid(note.notebookGuid());

        Q_UNUSED(m_findNoteByGuidRequestIds.insert(requestId));

        emit findNotebook(notebookToFind, findNotebookPerNoteRequestId);
        return;
    }

    ResourceDataPerFindNoteRequestId::iterator rit = m_resourcesWithFindRequestIdsPerFindNoteRequestId.find(requestId);
    if (rit != m_resourcesWithFindRequestIdsPerFindNoteRequestId.end())
    {
        QPair<ResourceWrapper,QUuid> resourceWithFindRequestId = rit.value();

        Q_UNUSED(m_resourcesWithFindRequestIdsPerFindNoteRequestId.erase(rit));

        CHECK_STOPPED();

        if (!note.hasGuid()) {
            QString errorDescription = QT_TR_NOOP("Internal error: found note by guid in local storage but the returned note object doesn't have guid set");
            QNWARNING(errorDescription << ": " << note);
            emit failure(errorDescription);
            return;
        }

        const ResourceWrapper & resource = resourceWithFindRequestId.first;
        const QUuid & findResourceRequestId = resourceWithFindRequestId.second;

        QString resourceGuid = (resource.hasGuid() ? resource.guid() : QString());
        QString resourceLocalGuid = resource.localGuid();

        QPair<QString,QString> key(resourceGuid, resourceLocalGuid);

        m_notesPerResourceGuids[key] = note;

        auto resourceFoundIt = m_resourceFoundFlagPerFindResourceRequestId.find(findResourceRequestId);
        if (resourceFoundIt == m_resourceFoundFlagPerFindResourceRequestId.end())
        {
            QNWARNING("Duplicate of synchronized resource was not found in the local storage database! Attempting to add it to local storage");

            emitAddRequest(resource);
            auto notesPerResourceGuidsIt = m_notesPerResourceGuids.find(key);
            if (notesPerResourceGuidsIt != m_notesPerResourceGuids.end()) {
                Q_UNUSED(m_notesPerResourceGuids.erase(notesPerResourceGuidsIt));
            }
            return;
        }

        if (!resource.isDirty())
        {
            QNDEBUG("Found duplicate resource in local storage which is not marked dirty => using the version from synchronization manager");

            QUuid updateResourceRequestId = QUuid::createUuid();
            Q_UNUSED(m_updateResourceRequestIds.insert(updateResourceRequestId));

            emit updateResource(resource, note, updateResourceRequestId);
            return;
        }

        QNDEBUG("Found duplicate resource in local storage which is marked dirty => will copy the whole note owning it to the conflict one");

        Note conflictedNote(note);
        setConflicted("Note", conflictedNote);
        QString conflictedNoteTitle = (conflictedNote.hasTitle()
                                       ? QObject::tr("Note \"") + conflictedNote.title() + QObject::tr("\" containing conflicted resource")
                                       : QObject::tr("Note containing conflicted resource"));
        conflictedNoteTitle += " (" + QDateTime::currentDateTime().toString(Qt::ISODate) + ")";
        conflictedNote.setTitle(conflictedNoteTitle);

        Note updatedNote(note);
        updatedNote.unsetLocalGuid();
        bool hasResources = updatedNote.hasResources();
        QList<ResourceAdapter> resources;
        if (hasResources) {
            resources = updatedNote.resourceAdapters();
        }

        int numResources = resources.size();
        int resourceIndex = -1;
        for(int i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & existingResource = resources[i];
            if (existingResource.hasGuid() && (existingResource.guid() == resource.guid())) {
                resourceIndex = i;
                break;
            }
        }

        if (resourceIndex < 0)
        {
            updatedNote.addResource(resource);
        }
        else
        {
            QList<ResourceWrapper> noteResources = updatedNote.resources();
            noteResources[resourceIndex] = resource;
            updatedNote.setResources(noteResources);
        }

        const Notebook * pNotebook = getNotebookPerNote(note);
        if (pNotebook)
        {
            QUuid updateNoteRequestId = QUuid::createUuid();
            Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));
            emit updateNote(updatedNote, *pNotebook, updateNoteRequestId);

            QUuid addNoteRequestId = QUuid::createUuid();
            Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
            emit addNote(conflictedNote, *pNotebook, addNoteRequestId);

            return;
        }

        QNDEBUG("Notebook for note is not found yet, need to find it in order to resolve the notes with conflicted resources");

        if (!note.hasNotebookGuid()) {
            QString errorDescription = QT_TR_NOOP("Note containing the conflicted resource in the local storage does not have notebook guid set");
            QNWARNING(errorDescription << ": " << note);
            emit failure(errorDescription);
            return;
        }

        m_resourceConflictedAndRemoteNotesPerNotebookGuid[note.notebookGuid()] = QPair<Note,Note>(conflictedNote, updatedNote);

        Notebook notebookToFind;
        notebookToFind.unsetLocalGuid();
        notebookToFind.setGuid(note.notebookGuid());

        QUuid findNotebookForNotesWithConflictedResourcesRequestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.insert(findNotebookForNotesWithConflictedResourcesRequestId));
        emit findNotebook(notebookToFind, findNotebookForNotesWithConflictedResourcesRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindNoteFailed(Note note, bool withResourceBinaryData,
                                                           QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator it = m_findNoteByGuidRequestIds.find(requestId);
    if (it != m_findNoteByGuidRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onFindNoteFailed: note = " << note
                << ", requestId = " << requestId);

        Q_UNUSED(m_findNoteByGuidRequestIds.erase(it));

        CHECK_STOPPED();

        auto it = findItemByGuid(m_notes, note, "Note");
        if (it == m_notes.end()) {
            return;
        }

        // Removing the note from the list of notes waiting for processing
        Q_UNUSED(m_notes.erase(it));

        qint32 postponeAPICallSeconds = tryToGetFullNoteData(note, errorDescription);
        if (postponeAPICallSeconds < 0)
        {
            return;
        }
        else if (postponeAPICallSeconds > 0)
        {
            int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
            if (timerId == 0) {
                QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                      "the Evernote API call due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_notesToAddPerAPICallPostponeTimerId[timerId] = note;
            emit rateLimitExceeded(postponeAPICallSeconds);
            return;
        }

        // Note duplicates by title are completely allowed, so can add the note as is,
        // without any conflict by title resolution
        emitAddRequest(note);
    }

    ResourceDataPerFindNoteRequestId::iterator rit = m_resourcesWithFindRequestIdsPerFindNoteRequestId.find(requestId);
    if (rit != m_resourcesWithFindRequestIdsPerFindNoteRequestId.end())
    {
        Q_UNUSED(m_resourcesWithFindRequestIdsPerFindNoteRequestId.erase(rit));

        CHECK_STOPPED();

        QString errorDescription = QT_TR_NOOP("Internal error: wasn't able to find note in local storage for individual "
                                              "resource coming from the synchronization procedure");
        QNWARNING(errorDescription << ", note attempted to be found: " << note);
        emit failure(errorDescription);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindTagCompleted(Tag tag, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(tag, requestId, "Tag", m_tags, m_findTagByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(tag, requestId, "Tag", m_tags, m_findTagByNameRequestIds);
    if (foundByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(tag, requestId, errorDescription, "Tag",
                                                  m_tags, m_findTagByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(tag, requestId, errorDescription, "Tag",
                                                  m_tags, m_findTagByNameRequestIds);
    if (failedToFindByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindResourceCompleted(ResourceWrapper resource,
                                                                  bool withResourceBinaryData, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator rit = m_findResourceByGuidRequestIds.find(requestId);
    if (rit != m_findResourceByGuidRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onFindResourceCompleted: resource = " << resource
                << ", requestId = " << requestId);

        CHECK_STOPPED();

        auto it = findItemByGuid(m_resources, resource, "Resource");
        if (it == m_resources.end()) {
            return;
        }

        // Removing the resource from the list of resources waiting for processing
        Q_UNUSED(m_resources.erase(it));

        // need to find the note owning the resource to proceed
        if (!resource.hasNoteGuid()) {
            QString errorDescription = QT_TR_NOOP("Found duplicate resource in local storage which doesn't have note guid set");
            QNWARNING(errorDescription << ": " << resource);
            emit failure(errorDescription);
            return;
        }

        Q_UNUSED(m_resourceFoundFlagPerFindResourceRequestId.insert(requestId));

        QUuid findNotePerResourceRequestId = QUuid::createUuid();
        m_resourcesWithFindRequestIdsPerFindNoteRequestId[findNotePerResourceRequestId] =
            QPair<ResourceWrapper,QUuid>(resource, requestId);

        Note noteToFind;
        noteToFind.unsetLocalGuid();
        noteToFind.setGuid(resource.noteGuid());

        Q_UNUSED(m_findResourceByGuidRequestIds.insert(requestId));

        emit findNote(noteToFind, /* with resource binary data = */ true, findNotePerResourceRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindResourceFailed(ResourceWrapper resource,
                                                               bool withResourceBinaryData,
                                                               QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator rit = m_findResourceByGuidRequestIds.find(requestId);
    if (rit != m_findResourceByGuidRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onFindResourceFailed: resource = " << resource
                << ", requestId = " << requestId);

        // NOTE: erase is required for proper work of the macro; the request would be re-inserted below if macro doesn't return from the method
        Q_UNUSED(m_findResourceByGuidRequestIds.erase(rit));

        CHECK_STOPPED();

        auto it = findItemByGuid(m_resources, resource, "Resource");
        if (it == m_resources.end()) {
            return;
        }

        // Removing the resource from the list of resources waiting for processing
        Q_UNUSED(m_resources.erase(it));

        // need to find the note owning the resource to proceed
        if (!resource.hasNoteGuid()) {
            QString errorDescription = QT_TR_NOOP("Detected resource which doesn't have note guid set");
            QNWARNING(errorDescription << ": " << resource);
            emit failure(errorDescription);
            return;
        }

        QUuid findNotePerResourceRequestId = QUuid::createUuid();
        m_resourcesWithFindRequestIdsPerFindNoteRequestId[findNotePerResourceRequestId] =
            QPair<ResourceWrapper,QUuid>(resource, requestId);

        Note noteToFind;
        noteToFind.unsetLocalGuid();
        noteToFind.setGuid(resource.noteGuid());

        Q_UNUSED(m_findResourceByGuidRequestIds.insert(requestId));

        emit findNote(noteToFind, /* with resource binary data = */ false, findNotePerResourceRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook,
                                                                        QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(onFoundDuplicateByGuid(linkedNotebook, requestId, "LinkedNotebook",
                                    m_linkedNotebooks, m_findLinkedNotebookRequestIds));

    CHECK_STOPPED();
}

void RemoteToLocalSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                     QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    QSet<QUuid>::iterator rit = m_findLinkedNotebookRequestIds.find(requestId);
    if (rit == m_findLinkedNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG("RemoteToLocalSynchronizationManager::onFindLinkedNotebookFailed: " << linkedNotebook
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(m_findLinkedNotebookRequestIds.erase(rit));

    CHECK_STOPPED();

    LinkedNotebooksList::iterator it = findItemByGuid(m_linkedNotebooks, linkedNotebook, "LinkedNotebook");
    if (it == m_linkedNotebooks.end()) {
        return;
    }

    // This linked notebook was not found in the local storage by guid, adding it there
    emitAddRequest(linkedNotebook);
}

void RemoteToLocalSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(savedSearch, requestId, "SavedSearch",
                                              m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(savedSearch, requestId, "SavedSearch",
                                              m_savedSearches, m_findSavedSearchByNameRequestIds);
    if (foundByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch,
                                                                  QString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(savedSearch, requestId, errorDescription, "SavedSearch",
                                                  m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(savedSearch, requestId, errorDescription, "SavedSearch",
                                                  m_savedSearches, m_findSavedSearchByNameRequestIds);
    if (failedToFindByName) {
        CHECK_STOPPED();
        return;
    }
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onAddDataElementCompleted(const ElementType & element,
                                                                    const QUuid & requestId,
                                                                    const QString & typeName,
                                                                    QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onAddDataElementCompleted<" << typeName
                << ">: " << typeName << " = " << element << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        CHECK_PAUSED();
        CHECK_STOPPED();

        performPostAddOrUpdateChecks<ElementType>();

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onAddTagCompleted(Tag tag, QUuid requestId)
{
    onAddDataElementCompleted(tag, requestId, "Tag", m_addTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onAddDataElementCompleted(search, requestId, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                 const QString & errorDescription, const QString & typeName,
                                                                 QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNWARNING("FullSyncronizationManager::onAddDataElementFailed<" << typeName
                  << ">: " << typeName << " = " << element << ", error description = "
                  << errorDescription << ", requestId = " << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        QString error = QT_TR_NOOP("Can't add remote " + typeName + " to local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onAddTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(tag, requestId, errorDescription, "Tag", m_addTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(search, requestId, errorDescription, "SavedSearch", m_addSavedSearchRequestIds);
}

template <class ElementType, class ElementsToAddByUuid>
void RemoteToLocalSynchronizationManager::onUpdateDataElementCompleted(const ElementType & element,
                                                                       const QUuid & requestId,
                                                                       const QString & typeName,
                                                                       QSet<QUuid> & updateElementRequestIds,
                                                                       ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator rit = updateElementRequestIds.find(requestId);
    if (rit == updateElementRequestIds.end()) {
        return;
    }

    QNDEBUG("RemoteToLocalSynchronizartionManager::onUpdateDataElementCompleted<" << typeName
            << ">: " << typeName << " = " << element << ", requestId = " << requestId);

    Q_UNUSED(updateElementRequestIds.erase(rit));

    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end())
    {
        ElementType & elementToAdd = addIt.value();
        const QString localGuid = elementToAdd.localGuid();
        if (localGuid.isEmpty()) {
            QString errorDescription = QT_TR_NOOP("detected " + typeName + " from local storage "
                                                  "with empty local guid");
            QNWARNING(errorDescription << ": " << elementToAdd);
            emit failure(errorDescription);
            return;
        }

        QSet<QString>::iterator git = m_localGuidsOfElementsAlreadyAttemptedToFindByName.find(localGuid);
        if (git == m_localGuidsOfElementsAlreadyAttemptedToFindByName.end())
        {
            QNDEBUG("Checking whether " << typeName << " with the same name already exists in local storage");
            Q_UNUSED(m_localGuidsOfElementsAlreadyAttemptedToFindByName.insert(localGuid));

            elementToAdd.unsetLocalGuid();
            elementToAdd.setGuid("");
            emitFindByNameRequest(elementToAdd);
        }
        else
        {
            QNDEBUG("Adding " << typeName << " to local storage");
            emitAddRequest(elementToAdd);
        }

        Q_UNUSED(elementsToAddByRenameRequestId.erase(addIt));
    }
    else {
        CHECK_PAUSED();
        CHECK_STOPPED();
        performPostAddOrUpdateChecks<ElementType>();
    }

    checkServerDataMergeCompletion();
}

void RemoteToLocalSynchronizationManager::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    onUpdateDataElementCompleted(tag, requestId, "Tag", m_updateTagRequestIds, m_tagsToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onUpdateDataElementCompleted(search, requestId, "SavedSearch", m_updateSavedSearchRequestIds,
                                 m_savedSearchesToAddPerRequestId);
}

template <class ElementType, class ElementsToAddByUuid>
void RemoteToLocalSynchronizationManager::onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                    const QString & errorDescription, const QString & typeName,
                                                                    QSet<QUuid> & updateElementRequestIds,
                                                                    ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator it = updateElementRequestIds.find(requestId);
    if (it == updateElementRequestIds.end()) {
        return;
    }

    QNWARNING("RemoteToLocalSynchronizationManager::onUpdateDataElementFailed<" << typeName
              << ">: " << typeName << " = " << element << ", errorDescription = "
              << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(updateElementRequestIds.erase(it));

    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end())
    {
        Q_UNUSED(elementsToAddByRenameRequestId.erase(addIt));

        QString error = QT_TR_NOOP("Can't rename local dirty duplicate " + typeName +
                                   " in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
    else
    {
        QString error = QT_TR_NOOP("Can't update remote " + typeName + " in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onUpdateTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed(tag, requestId, errorDescription, "Tag", m_updateTagRequestIds,
                              m_tagsToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeTagCompleted(Tag tag, QUuid requestId)
{
    onExpungeDataElementCompleted(tag, requestId, "Tag", m_expungeTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeTagFailed(Tag tag, QString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(tag, requestId, errorDescription, "Tag", m_expungeTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId)
{
    if (requestId == m_expungeNotelessTagsRequestId) {
        QNDEBUG("RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksCompleted");
        m_expungeNotelessTagsRequestId = QUuid();
        finalize();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksFailed(QString errorDescription, QUuid requestId)
{
    if (requestId == m_expungeNotelessTagsRequestId)
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksFailed: " << errorDescription);
        m_expungeNotelessTagsRequestId = QUuid();

        QString error = QT_TR_NOOP("Can't expunge noteless tags belonging to linked notebooks from local storage: ");
        error += errorDescription;
        emit failure(error);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onUpdateSavedSearchFailed(SavedSearch search, QString errorDescription,
                                                                    QUuid requestId)
{
    onUpdateDataElementFailed(search, requestId, errorDescription, "SavedSearch", m_updateSavedSearchRequestIds,
                              m_savedSearchesToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onExpungeDataElementCompleted(search, requestId, "SavedSearch", m_expungeSavedSearchRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeSavedSearchFailed(SavedSearch search, QString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(search, requestId, errorDescription, "SavedSearch", m_expungeSavedSearchRequestIds);
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks()
{
    // do nothing
}

template <>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<Tag>()
{
    checkNotebooksAndTagsSyncAndLaunchNotesSync();

    if (m_addTagRequestIds.empty() && m_updateTagRequestIds.empty()) {
        expungeTags();
    }
}

template <>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<Notebook>()
{
    checkNotebooksAndTagsSyncAndLaunchNotesSync();
}

template <>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<Note>()
{
    checkNotesSyncAndLaunchResourcesSync();

    if (m_addNoteRequestIds.empty() && m_updateNoteRequestIds.empty() &&
        m_notesToAddPerAPICallPostponeTimerId.empty() && m_notesToUpdatePerAPICallPostponeTimerId.empty())
    {
        if (!m_resources.empty()) {
            return;
        }

        if (!m_expungedNotes.empty()) {
            expungeNotes();
        }
        else if (!m_expungedNotebooks.empty()) {
            expungeNotebooks();
        }
        else {
            expungeLinkedNotebooks();
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<ResourceWrapper>()
{
    if (m_addResourceRequestIds.empty() && m_updateResourceRequestIds.empty() &&
        m_resourcesWithFindRequestIdsPerFindNoteRequestId.empty() &&
        m_findNotebookForNotesWithConflictedResourcesRequestIds.empty())
    {
        if (!m_expungedNotes.empty()) {
            expungeNotes();
        }
        else if (!m_expungedNotebooks.empty()) {
            expungeNotebooks();
        }
        else {
            expungeLinkedNotebooks();
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<SavedSearch>()
{
    if (m_addSavedSearchRequestIds.empty() && m_updateSavedSearchRequestIds.empty()) {
        expungeSavedSearches();
    }
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::unsetLocalGuid(ElementType & element)
{
    element.unsetLocalGuid();
}

template <>
void RemoteToLocalSynchronizationManager::unsetLocalGuid<LinkedNotebook>(LinkedNotebook &)
{
    // do nothing, local guid doesn't make any sense to linked notebook
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onExpungeDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                                                        const QString & typeName, QSet<QUuid> & expungeElementRequestIds)
{
    auto it = expungeElementRequestIds.find(requestId);
    if (it == expungeElementRequestIds.end()) {
        return;
    }

    QNTRACE("Expunged " << typeName << " from local storage: " << element);
    Q_UNUSED(expungeElementRequestIds.erase(it));

    performPostExpungeChecks<ElementType>();

    checkExpungesCompletion();
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onExpungeDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                     const QString & errorDescription, const QString & typeName,
                                                                     QSet<QUuid> & expungeElementRequestIds)
{
    auto it = expungeElementRequestIds.find(requestId);
    if (it == expungeElementRequestIds.end()) {
        return;
    }

    Q_UNUSED(expungeElementRequestIds.erase(it));

    QString error = QT_TR_NOOP("Can't expunge ");
    error += typeName;
    error += " ";
    error += QT_TR_NOOP("from local storage: ");
    error += errorDescription;
    QNWARNING(error);
    emit failure(error);
}

void RemoteToLocalSynchronizationManager::expungeTags()
{
    if (m_expungedTags.empty()) {
        return;
    }

    Tag tagToExpunge;
    tagToExpunge.unsetLocalGuid();

    const int numExpungedTags = m_expungedTags.size();
    for(int i = 0; i < numExpungedTags; ++i)
    {
        const QString & expungedTagGuid = m_expungedTags[i];
        tagToExpunge.setGuid(expungedTagGuid);

        QUuid expungeTagRequestId = QUuid::createUuid();
        Q_UNUSED(m_expungeTagRequestIds.insert(expungeTagRequestId));
        emit expungeTag(tagToExpunge, expungeTagRequestId);
    }

    m_expungedTags.clear();
}

void RemoteToLocalSynchronizationManager::expungeSavedSearches()
{
    if (m_expungedSavedSearches.empty()) {
        return;
    }

    SavedSearch searchToExpunge;
    searchToExpunge.unsetLocalGuid();

    const int numExpungedSearches = m_expungedSavedSearches.size();
    for(int i = 0; i < numExpungedSearches; ++i)
    {
        const QString & expungedSavedSerchGuid = m_expungedSavedSearches[i];
        searchToExpunge.setGuid(expungedSavedSerchGuid);

        QUuid expungeSavedSearchRequestId = QUuid::createUuid();
        Q_UNUSED(m_expungeSavedSearchRequestIds.insert(expungeSavedSearchRequestId));
        emit expungeSavedSearch(searchToExpunge, expungeSavedSearchRequestId);
    }

    m_expungedSavedSearches.clear();
}

void RemoteToLocalSynchronizationManager::expungeLinkedNotebooks()
{
    if (m_expungedLinkedNotebooks.empty()) {
        return;
    }

    LinkedNotebook linkedNotebookToExpunge;

    const int numExpungedLinkedNotebooks = m_expungedLinkedNotebooks.size();
    for(int i = 0; i < numExpungedLinkedNotebooks; ++i)
    {
        const QString & expungedLinkedNotebookGuid = m_expungedLinkedNotebooks[i];
        linkedNotebookToExpunge.setGuid(expungedLinkedNotebookGuid);

        QUuid expungeLinkedNotebookRequestId = QUuid::createUuid();
        Q_UNUSED(m_expungeLinkedNotebookRequestIds.insert(expungeLinkedNotebookRequestId));
        emit expungeLinkedNotebook(linkedNotebookToExpunge, expungeLinkedNotebookRequestId);
    }

    m_expungedLinkedNotebooks.clear();
}

void RemoteToLocalSynchronizationManager::expungeNotebooks()
{
    if (m_expungedNotebooks.empty()) {
        return;
    }

    Notebook notebookToExpunge;
    notebookToExpunge.unsetLocalGuid();

    const int numExpungedNotebooks = m_expungedNotebooks.size();
    for(int i = 0; i < numExpungedNotebooks; ++i)
    {
        const QString & expungedNotebookGuid = m_expungedNotebooks[i];
        notebookToExpunge.setGuid(expungedNotebookGuid);

        QUuid expungeNotebookRequestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNotebookRequestIds.insert(expungeNotebookRequestId));
        emit expungeNotebook(notebookToExpunge, expungeNotebookRequestId);
    }

    m_expungedNotebooks.clear();
}

void RemoteToLocalSynchronizationManager::expungeNotes()
{
    if (m_expungedNotes.empty()) {
        return;
    }

    Note noteToExpunge;
    noteToExpunge.unsetLocalGuid();

    const int numExpungedNotes = m_expungedNotes.size();
    for(int i = 0; i < numExpungedNotes; ++i)
    {
        const QString & expungedNoteGuid = m_expungedNotes[i];
        noteToExpunge.setGuid(expungedNoteGuid);

        QUuid expungeNoteRequestId = QUuid::createUuid();
        Q_UNUSED(m_expungeNoteRequestIds.insert(expungeNoteRequestId));
        emit expungeNote(noteToExpunge, expungeNoteRequestId);
    }

    m_expungedNotes.clear();
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::performPostExpungeChecks()
{
    // do nothing by default
}

template <>
void RemoteToLocalSynchronizationManager::performPostExpungeChecks<Note>()
{
    if (m_expungeNoteRequestIds.empty())
    {
        if (!m_expungedNotebooks.isEmpty()) {
            expungeNotebooks();
            return;
        }

        expungeSavedSearches();
        expungeTags();
        expungeLinkedNotebooks();
    }
}

template <>
void RemoteToLocalSynchronizationManager::performPostExpungeChecks<Notebook>()
{
    if (m_expungeNotebookRequestIds.empty()) {
        expungeSavedSearches();
        expungeTags();
        expungeLinkedNotebooks();
    }
}

void RemoteToLocalSynchronizationManager::expungeFromServerToClient()
{
    if (!m_expungedNotes.isEmpty()) {
        expungeNotes();
        return;
    }

    if (!m_expungedNotebooks.isEmpty()) {
        expungeNotebooks();
        return;
    }

    expungeSavedSearches();
    expungeTags();
    expungeLinkedNotebooks();

    checkExpungesCompletion();
}

void RemoteToLocalSynchronizationManager::checkExpungesCompletion()
{
    if (m_expungedTags.isEmpty() && m_expungeTagRequestIds.isEmpty() &&
        m_expungedNotebooks.isEmpty() && m_expungeNotebookRequestIds.isEmpty() &&
        m_expungedSavedSearches.isEmpty() && m_expungeSavedSearchRequestIds.isEmpty() &&
        m_expungedLinkedNotebooks.isEmpty() && m_expungeLinkedNotebookRequestIds.isEmpty() &&
        m_expungedNotes.isEmpty() && m_expungeNoteRequestIds.isEmpty())
    {
        if (syncingLinkedNotebooksContent())
        {
            m_expungeNotelessTagsRequestId = QUuid::createUuid();
            emit expungeNotelessTagsFromLinkedNotebooks(m_expungeNotelessTagsRequestId);
        }
        else
        {
            m_expungedFromServerToClient = true;
            emit expungedFromServerToClient();

            startLinkedNotebooksSync();
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<Tag>(const QString & guid)
{
    Tag tag;
    tag.unsetLocalGuid();
    tag.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagByGuidRequestIds.insert(requestId));
    emit findTag(tag, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<SavedSearch>(const QString & guid)
{
    SavedSearch search;
    search.unsetLocalGuid();
    search.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchByGuidRequestIds.insert(requestId));
    emit findSavedSearch(search, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<Notebook>(const QString & guid)
{
    Notebook notebook;
    notebook.unsetLocalGuid();
    notebook.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookByGuidRequestIds.insert(requestId));
    emit findNotebook(notebook, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<LinkedNotebook>(const QString & guid)
{
    LinkedNotebook linkedNotebook;
    linkedNotebook.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findLinkedNotebookRequestIds.insert(requestId));
    emit findLinkedNotebook(linkedNotebook, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<Note>(const QString & guid)
{
    Note note;
    note.unsetLocalGuid();
    note.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteByGuidRequestIds.insert(requestId));
    bool withResourceDataOption = false;
    emit findNote(note, withResourceDataOption, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<ResourceWrapper>(const QString & guid)
{
    ResourceWrapper resource;
    resource.unsetLocalGuid();
    resource.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findResourceByGuidRequestIds.insert(requestId));
    bool withBinaryData = false;
    emit findResource(resource, withBinaryData, requestId);
}

void RemoteToLocalSynchronizationManager::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onAddDataElementCompleted(linkedNotebook, requestId, "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                    QString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(linkedNotebook, requestId, errorDescription,
                           "LinkedNotebook", m_addLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook,
                                                                          QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookCompleted: linkedNotebook = "
                << linkedNotebook << ", requestId = " << requestId);

        Q_UNUSED(m_updateLinkedNotebookRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                       QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookFailed: linkedNotebook = "
                << linkedNotebook << ", errorDescription = " << errorDescription
                << ", requestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update linked notebook in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onExpungeLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onExpungeDataElementCompleted(linkedNotebook, requestId, "Linked notebook", m_expungeLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeLinkedNotebookFailed(LinkedNotebook linkedNotebook, QString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(linkedNotebook, requestId, errorDescription, "Linked notebook", m_expungeLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onAddDataElementCompleted(notebook, requestId, "Notebook", m_addNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNotebookFailed(Notebook notebook, QString errorDescription,
                                                              QUuid requestId)
{
    onAddDataElementFailed(notebook, requestId, errorDescription, "Notebook", m_addNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onUpdateDataElementCompleted(notebook, requestId, "Notebook", m_updateNotebookRequestIds,
                                 m_notebooksToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onUpdateNotebookFailed(Notebook notebook, QString errorDescription,
                                                                 QUuid requestId)
{
    onUpdateDataElementFailed(notebook, requestId, errorDescription, "Notebook",
                              m_updateNotebookRequestIds, m_notebooksToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onExpungeDataElementCompleted(notebook, requestId, "Notebook", m_expungeNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(notebook, requestId, errorDescription, "Notebook", m_expungeNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    Q_UNUSED(notebook)
    onAddDataElementCompleted(note, requestId, "Note", m_addNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNoteFailed(Note note, Notebook notebook,
                                                          QString errorDescription, QUuid requestId)
{
    Q_UNUSED(notebook)
    onAddDataElementFailed(note, requestId, errorDescription, "Note", m_addNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateNoteCompleted(Note note, Notebook notebook, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateNoteCompleted: note = " << note
                << "\nnotebook = " << notebook << "\nrequestId = " << requestId);

        Q_UNUSED(m_updateNoteRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateNoteFailed(Note note, Notebook notebook,
                                                             QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateNoteFailed: note = " << note
                << "\nnotebook = " << notebook << "\nerrorDescription = " << errorDescription
                << "\nrequestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update note in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onExpungeNoteCompleted(Note note, QUuid requestId)
{
    onExpungeDataElementCompleted(note, requestId, "Note", m_expungeNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(note, requestId, errorDescription, "Note", m_expungeNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddResourceCompleted(ResourceWrapper resource, Note note, QUuid requestId)
{
    Q_UNUSED(note)
    onAddDataElementCompleted(resource, requestId, "Resource", m_addResourceRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddResourceFailed(ResourceWrapper resource, Note note, QString errorDescription, QUuid requestId)
{
    Q_UNUSED(note)
    onAddDataElementFailed(resource, requestId, errorDescription, "Resource", m_addResourceRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateResourceCompleted(ResourceWrapper resource, Note note, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateResourceRequestIds.find(requestId);
    if (it != m_updateResourceRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateResourceCompleted: resource = " << resource << "\nnote = "
                << note << "\nrequestId = " << requestId);

        Q_UNUSED(m_updateResourceRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateResourceFailed(ResourceWrapper resource, Note note, QString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateResourceRequestIds.find(requestId);
    if (it != m_updateResourceRequestIds.end())
    {
        QNDEBUG("RemoteToLocalSynchronizationManager::onUpdateResourceFailed: resource = " << resource << "\nnote = "
                << note << "\nrequestId = " << requestId);

        QString error = QT_TR_NOOP("Can't update resource in local storage: ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString, QString> authenticationTokensByLinkedNotebookGuid,
                                                                                           QHash<QString, qevercloud::Timestamp> authenticationTokenExpirationTimesByLinkedNotebookGuid)
{
    m_authenticationTokensByLinkedNotebookGuid = authenticationTokensByLinkedNotebookGuid;
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid = authenticationTokenExpirationTimesByLinkedNotebookGuid;

    startLinkedNotebooksSync();
}

void RemoteToLocalSynchronizationManager::onLastSyncParametersReceived(qint32 lastUpdateCount, qevercloud::Timestamp lastSyncTime,
                                                                       QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid,
                                                                       QHash<QString,qevercloud::Timestamp> lastSyncTimeByLinkedNotebookGuid)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::onLastSyncParametersReceived: last update count = " << lastUpdateCount
            << ", last sync time = " << lastSyncTime << ", last update counts per linked notebook = " << lastUpdateCountByLinkedNotebookGuid
            << "last sync time per linked notebook = " << lastSyncTimeByLinkedNotebookGuid);

    m_lastUpdateCount = lastUpdateCount;
    m_lastSyncTime = lastSyncTime;
    m_lastUpdateCountByLinkedNotebookGuid = lastUpdateCountByLinkedNotebookGuid;
    m_lastSyncTimeByLinkedNotebookGuid = lastSyncTimeByLinkedNotebookGuid;

    m_gotLastSyncParameters = true;

    if ((m_lastUpdateCount > 0) && (m_lastSyncTime > 0)) {
        m_onceSyncDone = true;
    }

    start(m_lastUsnOnStart);
}

void RemoteToLocalSynchronizationManager::createConnections()
{
    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook,QUuid)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));
    QObject::connect(this, SIGNAL(findNote(Note,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool,QUuid)));
    QObject::connect(this, SIGNAL(deleteNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note,QUuid)));
    QObject::connect(this, SIGNAL(expungeNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note,QUuid)));

    QObject::connect(this, SIGNAL(addTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(findTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(deleteTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag,QUuid)));
    QObject::connect(this, SIGNAL(expungeTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag,QUuid)));

    QObject::connect(this, SIGNAL(expungeNotelessTagsFromLinkedNotebooks(QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotelessTagsFromLinkedNotebooksRequest(QUuid)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool,QUuid)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook,QUuid)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch,QUuid)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch,QUuid)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onFindNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool,QUuid)), this, SLOT(onFindNoteCompleted(Note,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString,QUuid)), this, SLOT(onFindNoteFailed(Note,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag,QUuid)), this, SLOT(onFindTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString,QUuid)), this, SLOT(onFindTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onFindSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onFindSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool,QUuid)), this, SLOT(onFindResourceCompleted(ResourceWrapper,bool,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString,QUuid)), this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag,QUuid)), this, SLOT(onAddTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString,QUuid)), this, SLOT(onAddTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeTagComplete(Tag,QUuid)), this, SLOT(onExpungeTagCompleted(Tag,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeTagFailed(Tag,QString,QUuid)), this, SLOT(onExpungeTagFailed(Tag,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotelessTagsFromLinkedNotebooksComplete(QUuid)), this, SLOT(onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotelessTagsFromLinkedNotebooksFailed(QString,QUUid)), this, SLOT(onExpungeNotelessTagsFromLinkedNotebooksFailed(QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onExpungeSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onExpungeSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onExpungeLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onExpungeLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook,QUuid)), this, SLOT(onAddNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onAddNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotebookComplete(Notebook,QUuid)), this, SLOT(onExpungeNotebookCompleted(Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onExpungeNotebookFailed(Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteComplete(Note,Notebook,QUuid)), this, SLOT(onAddNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onAddNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)), this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNoteComplete(Note,QUuid)), this, SLOT(onExpungeNoteCompleted(Note,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNoteFailed(Note,QString,QUuid)), this, SLOT(onExpungeNoteFailed(Note,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addResourceComplete(ResourceWrapper,Note,QUuid)), this, SLOT(onAddResourceCompleted(ResourceWrapper,Note,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(addResourceFailed(ResourceWrapper,Note,QString,QUuid)), this, SLOT(onAddResourceFailed(ResourceWrapper,Note,QString,QUuid)));

    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateResourceComplete(ResourceWrapper,Note,QUuid)), this, SLOT(onUpdateResourceCompleted(ResourceWrapper,Note,QUuid)));
    QObject::connect(&m_localStorageManagerThreadWorker, SIGNAL(updateResourceFailed(ResourceWrapper,Note,QString,QUuid)), this, SLOT(onUpdateResourceFailed(ResourceWrapper,Note,QString,QUuid)));

    m_connectedToLocalStorage = true;
}

void RemoteToLocalSynchronizationManager::disconnectFromLocalStorage()
{
    // Disconnect local signals from localStorageManagerThread's slots
    QObject::disconnect(this, SIGNAL(addNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(updateNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(findNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(expungeNotebook(Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook,QUuid)));

    QObject::disconnect(this, SIGNAL(addNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(updateNote(Note,Notebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook,QUuid)));
    QObject::disconnect(this, SIGNAL(findNote(Note,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool,QUuid)));
    QObject::disconnect(this, SIGNAL(deleteNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note,QUuid)));
    QObject::disconnect(this, SIGNAL(expungeNote(Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note,QUuid)));

    QObject::disconnect(this, SIGNAL(addTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag,QUuid)));
    QObject::disconnect(this, SIGNAL(updateTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag,QUuid)));
    QObject::disconnect(this, SIGNAL(findTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag,QUuid)));
    QObject::disconnect(this, SIGNAL(deleteTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag,QUuid)));
    QObject::disconnect(this, SIGNAL(expungeTag(Tag,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag,QUuid)));

    QObject::disconnect(this, SIGNAL(addResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::disconnect(this, SIGNAL(updateResource(ResourceWrapper,Note,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note,QUuid)));
    QObject::disconnect(this, SIGNAL(findResource(ResourceWrapper,bool,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool,QUuid)));

    QObject::disconnect(this, SIGNAL(addLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::disconnect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::disconnect(this, SIGNAL(findLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook,QUuid)));
    QObject::disconnect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook,QUuid)));

    QObject::disconnect(this, SIGNAL(addSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch,QUuid)));
    QObject::disconnect(this, SIGNAL(updateSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch,QUuid)));
    QObject::disconnect(this, SIGNAL(findSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch,QUuid)));
    QObject::disconnect(this, SIGNAL(expungeSavedSearch(SavedSearch,QUuid)), &m_localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch,QUuid)));

    // Disconnect localStorageManagerThread's signals to local slots
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook,QUuid)), this, SLOT(onFindNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onFindNotebookFailed(Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool,QUuid)), this, SLOT(onFindNoteCompleted(Note,bool,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString,QUuid)), this, SLOT(onFindNoteFailed(Note,bool,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag,QUuid)), this, SLOT(onFindTagCompleted(Tag,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString,QUuid)), this, SLOT(onFindTagFailed(Tag,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onFindSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onFindSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool,QUuid)), this, SLOT(onFindResourceCompleted(ResourceWrapper,bool,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString,QUuid)), this, SLOT(onFindResourceFailed(ResourceWrapper,bool,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addTagComplete(Tag,QUuid)), this, SLOT(onAddTagCompleted(Tag,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addTagFailed(Tag,QString,QUuid)), this, SLOT(onAddTagFailed(Tag,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagComplete(Tag,QUuid)), this, SLOT(onUpdateTagCompleted(Tag,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateTagFailed(Tag,QString,QUuid)), this, SLOT(onUpdateTagFailed(Tag,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeTagComplete(Tag,QUuid)), this, SLOT(onExpungeTagCompleted(Tag,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeTagFailed(Tag,QString,QUuid)), this, SLOT(onExpungeTagFailed(Tag,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onAddSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onAddSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onUpdateSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onUpdateSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeSavedSearchComplete(SavedSearch,QUuid)), this, SLOT(onExpungeSavedSearchCompleted(SavedSearch,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeSavedSearchFailed(SavedSearch,QString,QUuid)), this, SLOT(onExpungeSavedSearchFailed(SavedSearch,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onAddLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onAddLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onUpdateLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onUpdateLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeLinkedNotebookComplete(LinkedNotebook,QUuid)), this, SLOT(onExpungeLinkedNotebookCompleted(LinkedNotebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeLinkedNotebookFailed(LinkedNotebook,QString,QUuid)), this, SLOT(onExpungeLinkedNotebookFailed(LinkedNotebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookComplete(Notebook,QUuid)), this, SLOT(onAddNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onAddNotebookFailed(Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookComplete(Notebook,QUuid)), this, SLOT(onUpdateNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onUpdateNotebookFailed(Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotebookComplete(Notebook,QUuid)), this, SLOT(onExpungeNotebookCompleted(Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNotebookFailed(Notebook,QString,QUuid)), this, SLOT(onExpungeNotebookFailed(Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteComplete(Note,Notebook,QUuid)), this, SLOT(onUpdateNoteCompleted(Note,Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(updateNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onUpdateNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteComplete(Note,Notebook,QUuid)), this, SLOT(onAddNoteCompleted(Note,Notebook,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(addNoteFailed(Note,Notebook,QString,QUuid)), this, SLOT(onAddNoteFailed(Note,Notebook,QString,QUuid)));

    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNoteComplete(Note,QUuid)), this, SLOT(onExpungeNoteCompleted(Note,QUuid)));
    QObject::disconnect(&m_localStorageManagerThreadWorker, SIGNAL(expungeNoteFailed(Note,QString,QUuid)), this, SLOT(onExpungeNoteFailed(Note,QString,QUuid)));

    m_connectedToLocalStorage = false;
}

void RemoteToLocalSynchronizationManager::launchSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchSync");

    launchSavedSearchSync();
    launchLinkedNotebookSync();

    launchTagsSync();
    launchNotebookSync();
    if (!m_tags.empty() || !m_notebooks.empty()) {
        // NOTE: the sync of notes and, if need be, individual resouces would be launched later
        return;
    }

    QNDEBUG("The local lists of tags and notebooks waiting for processing are empty, "
            "checking if there are notes to process");

    launchNotesSync();
    if (!m_notes.empty()) {
        QNDEBUG("Launching the sync of notes");
        // NOTE: the sync of individual resources would be launched later (if current sync is incremental)
        return;
    }

    QNDEBUG("The local list of notes waiting for processing is empty");

    if (m_lastSyncMode != SyncMode::IncrementalSync) {
        QNDEBUG("Running full sync => no sync for individual resources is needed");
        return;
    }

    launchResourcesSync();
}

void RemoteToLocalSynchronizationManager::launchTagsSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchTagsSync");
    launchDataElementSync<TagsList, Tag>(ContentSource::UserAccount, "Tag", m_tags, m_expungedTags);
}

void RemoteToLocalSynchronizationManager::launchSavedSearchSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchSavedSearchSync");
    launchDataElementSync<SavedSearchesList, SavedSearch>(ContentSource::UserAccount, "Saved search",
                                                          m_savedSearches, m_expungedSavedSearches);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebookSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchLinkedNotebookSync");
    launchDataElementSync<LinkedNotebooksList, LinkedNotebook>(ContentSource::UserAccount, "Linked notebook",
                                                               m_linkedNotebooks, m_expungedLinkedNotebooks);
}

void RemoteToLocalSynchronizationManager::launchNotebookSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchNotebookSync");
    launchDataElementSync<NotebooksList, Notebook>(ContentSource::UserAccount, "Notebook", m_notebooks, m_expungedNotebooks);
}

bool RemoteToLocalSynchronizationManager::syncingLinkedNotebooksContent() const
{
    if (m_lastSyncMode == SyncMode::FullSync) {
        return m_fullNoteContentsDownloaded;
    }

    return m_expungedFromServerToClient;
}

QTextStream & operator<<(QTextStream & strm, const RemoteToLocalSynchronizationManager::ContentSource::type & obj)
{
    switch(obj)
    {
    case RemoteToLocalSynchronizationManager::ContentSource::UserAccount:
        strm << "UserAccount";
        break;
    case RemoteToLocalSynchronizationManager::ContentSource::LinkedNotebook:
        strm << "LinkedNotebook";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

void RemoteToLocalSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync");

    if (m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty() &&
        m_updateTagRequestIds.empty() && m_addTagRequestIds.empty())
    {
        // All remote notebooks and tags were already either updated in the local storage or added there
        launchNotesSync();
    }
}

void RemoteToLocalSynchronizationManager::launchNotesSync()
{
    launchDataElementSync<NotesList, Note>(ContentSource::UserAccount, "Note", m_notes, m_expungedNotes);
}

void RemoteToLocalSynchronizationManager::checkNotesSyncAndLaunchResourcesSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkNotesSyncAndLaunchResourcesSync");

    if (m_lastSyncMode != SyncMode::IncrementalSync) {
        return;
    }

    if (m_updateNoteRequestIds.empty() && m_addNoteRequestIds.empty() && m_notesToAddPerAPICallPostponeTimerId.empty() &&
        m_notesToUpdatePerAPICallPostponeTimerId.empty())
    {
        // All remote notes were already either updated in the local storage or added there
        launchResourcesSync();
    }
}

void RemoteToLocalSynchronizationManager::launchResourcesSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchResourcesSync");
    QList<QString> dummyList;
    launchDataElementSync<ResourcesList, ResourceWrapper>(ContentSource::UserAccount, "Resource", m_resources, dummyList);
}

void RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync");

    if (m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty()) {
        // All remote linked notebooks were already updated in the local storage or added there
        startLinkedNotebooksSync();
    }
}

void RemoteToLocalSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaunchLinkedNotebookNotesSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaunchLinkedNotebookNotesSync");

    if (m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty() &&
        m_updateTagRequestIds.empty() && m_addTagRequestIds.empty())
    {
        // All remote notebooks and tags from linked notebooks were already either updated in the local storage or added there
        launchLinkedNotebooksNotesSync();
    }
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksContentsSync()
{
    launchLinkedNotebooksTagsSync();
    launchLinkedNotebooksNotebooksSync();
}

template <>
bool RemoteToLocalSynchronizationManager::mapContainerElementsWithLinkedNotebookGuid<RemoteToLocalSynchronizationManager::TagsList>(const QString & linkedNotebookGuid,
                                                                                                                                    const RemoteToLocalSynchronizationManager::TagsList & tags)
{
    const int numTags = tags.size();
    for(int i = 0; i < numTags; ++i)
    {
        const qevercloud::Tag & tag = tags[i];
        if (!tag.guid.isSet()) {
            QString error = QT_TR_NOOP("Internal error: detected attempt to map linked notebook guid to tag without guid set");
            QNWARNING(error << ", tag: " << tag);
            emit failure(error);
            return false;
        }

        m_linkedNotebookGuidsByTagGuids[tag.guid.ref()] = linkedNotebookGuid;
    }

    return true;
}

template <>
bool RemoteToLocalSynchronizationManager::mapContainerElementsWithLinkedNotebookGuid<RemoteToLocalSynchronizationManager::NotebooksList>(const QString & linkedNotebookGuid,
                                                                                                                                         const RemoteToLocalSynchronizationManager::NotebooksList & notebooks)
{
    const int numNotebooks = notebooks.size();
    for(int i = 0; i < numNotebooks; ++i)
    {
        const qevercloud::Notebook & notebook = notebooks[i];
        if (!notebook.guid.isSet()) {
            QString error = QT_TR_NOOP("Internal error: detected attempt to map linked notebook guid to notebook without guid set");
            QNWARNING(error << ", notebook: " << notebook);
            emit failure(error);
            return false;
        }

        m_linkedNotebookGuidsByNotebookGuids[notebook.guid.ref()] = linkedNotebookGuid;
    }

    return true;
}

template <>
void RemoteToLocalSynchronizationManager::unmapContainerElementsFromLinkedNotebookGuid<qevercloud::Tag>(const QList<QString> & tagGuids)
{
    typedef QList<QString>::const_iterator CIter;
    CIter tagGuidsEnd = tagGuids.end();
    for(CIter it = tagGuids.begin(); it != tagGuidsEnd; ++it)
    {
        const QString & guid = *it;

        auto mapIt = m_linkedNotebookGuidsByTagGuids.find(guid);
        if (mapIt == m_linkedNotebookGuidsByTagGuids.end()) {
            continue;
        }

        Q_UNUSED(m_linkedNotebookGuidsByTagGuids.erase(mapIt));
    }
}

template <>
void RemoteToLocalSynchronizationManager::unmapContainerElementsFromLinkedNotebookGuid<qevercloud::Notebook>(const QList<QString> & notebookGuids)
{
    typedef QList<QString>::const_iterator CIter;
    CIter notebookGuidsEnd = notebookGuids.end();
    for(CIter it = notebookGuids.begin(); it != notebookGuidsEnd; ++it)
    {
        const QString & guid = *it;

        auto mapIt = m_linkedNotebookGuidsByNotebookGuids.find(guid);
        if (mapIt == m_linkedNotebookGuidsByNotebookGuids.end()) {
            continue;
        }

        Q_UNUSED(m_linkedNotebookGuidsByNotebookGuids.erase(mapIt));
    }
}

void RemoteToLocalSynchronizationManager::startLinkedNotebooksSync()
{
    const int numLinkedNotebooks = m_linkedNotebooks.size();
    if (numLinkedNotebooks == 0) {
        QNDEBUG("No linked notebooks are present within the account, can finish the synchronization right away");
        m_linkedNotebooksSyncChunksDownloaded = true;
        emit linkedNotebooksSyncChunksDownloaded();
        emit linkedNotebooksFullNotesContentsDownloaded();
        finalize();
        return;
    }

    if (!checkAndRequestAuthenticationTokensForLinkedNotebooks()) {
        return;
    }

    if (!downloadLinkedNotebooksSyncChunks()) {
        return;
    }

    launchLinkedNotebooksContentsSync();
}

bool RemoteToLocalSynchronizationManager::checkAndRequestAuthenticationTokensForLinkedNotebooks()
{
    const int numLinkedNotebooks = m_linkedNotebooks.size();
    for(int i = 0; i < numLinkedNotebooks; ++i)
    {
        const qevercloud::LinkedNotebook & linkedNotebook = m_linkedNotebooks[i];
        if (!linkedNotebook.guid.isSet()) {
            QString error = QT_TR_NOOP("Internal error: found linked notebook without guid set");
            QNWARNING(error << ", linked notebook: " << linkedNotebook);
            emit failure(error);
            return false;
        }

        if (!m_authenticationTokensByLinkedNotebookGuid.contains(linkedNotebook.guid)) {
            QNDEBUG("Authentication token for linked notebook with guid " << linkedNotebook.guid
                    << " was not found; will request authentication tokens for all linked notebooks at once");

            requestAuthenticationTokensForAllLinkedNotebooks();
            return false;
        }

        auto it = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(linkedNotebook.guid);
        if (it == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end()) {
            QString error = QT_TR_NOOP("Internal inconsistency detected: can't find cached "
                                       "expiration time of linked notebook's authentication token");
            QNWARNING(error << ", linked notebook: " << linkedNotebook);
            emit failure(error);
            return false;
        }

        const qevercloud::Timestamp & expirationTime = it.value();
        const qevercloud::Timestamp currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - expirationTime < SIX_HOURS_IN_MSEC) {
            QNDEBUG("Authentication token for linked notebook with guid " << linkedNotebook.guid
                    << " is too close to expiration: its expiration time is " << PrintableDateTimeFromTimestamp(expirationTime)
                    << ", current time is " << PrintableDateTimeFromTimestamp(currentTime)
                    << "; will request new authentication tokens for all linked notebooks");

            requestAuthenticationTokensForAllLinkedNotebooks();
            return false;
        }
    }

    QNDEBUG("Got authentication tokens for all linked notebooks, can proceed with "
            "their synchronization");

    return true;
}

void RemoteToLocalSynchronizationManager::requestAuthenticationTokensForAllLinkedNotebooks()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::requestAuthenticationTokensForAllLinkedNotebooks");

    QList<QPair<QString,QString> > linkedNotebookGuidsAndShareKeys;
    const int numLinkedNotebooks = m_linkedNotebooks.size();
    for(int j = 0; j < numLinkedNotebooks; ++j)
    {
        const qevercloud::LinkedNotebook & currentLinkedNotebook = m_linkedNotebooks[j];

        if (!currentLinkedNotebook.guid.isSet()) {
            QString error = QT_TR_NOOP("Internal error: found linked notebook without guid set");
            QNWARNING(error << ", linked notebook: " << currentLinkedNotebook);
            emit failure(error);
            return;
        }

        if (!currentLinkedNotebook.shareKey.isSet()) {
            QString error = QT_TR_NOOP("Found linked notebook without a share key");
            QNWARNING(error << ", linked notebook: " << currentLinkedNotebook);
            emit failure(error);
            return;
        }

        linkedNotebookGuidsAndShareKeys << QPair<QString,QString>(currentLinkedNotebook.guid, currentLinkedNotebook.shareKey);
    }

    emit requestAuthenticationTokensForLinkedNotebooks(linkedNotebookGuidsAndShareKeys);
}

bool RemoteToLocalSynchronizationManager::downloadLinkedNotebooksSyncChunks()
{
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    QNDEBUG("Downloading linked notebook sync chunks:");

    const int numLinkedNotebooks = m_linkedNotebooks.size();
    for(int i = 0; i < numLinkedNotebooks; ++i)
    {
        const qevercloud::LinkedNotebook & linkedNotebook = m_linkedNotebooks[i];
        if (!linkedNotebook.guid.isSet()) {
            QString error = QT_TR_NOOP("Found linked notebook without guid set when "
                                       "attempting to synchronize the content it points to");
            QNWARNING(error << ": " << linkedNotebook);
            emit failure(error);
            return false;
        }

        const QString & linkedNotebookGuid = linkedNotebook.guid.ref();

        bool fullSyncOnly = false;
        auto lastSynchronizedUsnIt = m_lastSynchronizedUsnByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (lastSynchronizedUsnIt == m_lastSynchronizedUsnByLinkedNotebookGuid.end()) {
            lastSynchronizedUsnIt = m_lastSynchronizedUsnByLinkedNotebookGuid.insert(linkedNotebookGuid, 0);
            fullSyncOnly = true;
        }
        qint32 afterUsn = lastSynchronizedUsnIt.value();

        auto lastSyncTimeIt = m_lastSyncTimeByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (lastSyncTimeIt == m_lastSyncTimeByLinkedNotebookGuid.end()) {
            lastSyncTimeIt = m_lastSyncTimeByLinkedNotebookGuid.insert(linkedNotebookGuid, 0);
        }
        qevercloud::Timestamp lastSyncTime = lastSyncTimeIt.value();

        auto lastUpdateCountIt = m_lastUpdateCountByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (lastUpdateCountIt == m_lastUpdateCountByLinkedNotebookGuid.end()) {
            lastUpdateCountIt = m_lastUpdateCountByLinkedNotebookGuid.insert(linkedNotebookGuid, 0);
        }
        qint32 lastUpdateCount = lastUpdateCountIt.value();

        auto syncChunksDownloadedFlagIt = m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.find(linkedNotebookGuid);
        if (syncChunksDownloadedFlagIt != m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.end()) {
            QNDEBUG("Sync chunks were already downloaded for linked notebook with guid " << linkedNotebookGuid);
            continue;
        }

        auto it = m_authenticationTokensByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (it == m_authenticationTokensByLinkedNotebookGuid.end()) {
            QString error = QT_TR_NOOP("Can't find authentication token for one of linked notebooks "
                                       "when attempting to synchronize the content it points to");
            QNWARNING(error << ": " << linkedNotebook);
            emit failure(error);
            return false;
        }

        const QString & authToken = it.value();

        if (m_onceSyncDone || (afterUsn != 0))
        {
            auto syncStateIter = m_syncStatesByLinkedNotebookGuid.find(linkedNotebookGuid);
            if (syncStateIter == m_syncStatesByLinkedNotebookGuid.end())
            {
                qevercloud::SyncState syncState;
                QString errorDescription;
                qint32 rateLimitSeconds = 0;
                qint32 errorCode = m_noteStore.getLinkedNotebookSyncState(linkedNotebook, authToken, syncState,
                                                                          errorDescription, rateLimitSeconds);
                if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
                {
                    if (rateLimitSeconds <= 0) {
                        errorDescription += QString("\n") + QT_TR_NOOP("Internal error: rate limit seconds = ");
                        errorDescription += QString::number(rateLimitSeconds);
                        emit failure(errorDescription);
                        return false;
                    }

                    int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
                    if (timerId == 0) {
                        QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                              "the Evernote API call due to rate limit exceeding");
                        emit failure(errorDescription);
                        return false;
                    }

                    m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId = timerId;
                    emit rateLimitExceeded(rateLimitSeconds);
                    return false;
                }
                else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
                {
                    QString errorPrefix = QT_TR_NOOP("Unexpected AUTH_EXPIRED error when trying to get linked notebook sync state: ");
                    errorDescription.prepend(errorPrefix);
                    emit failure(errorDescription);
                    return false;
                }
                else if (errorCode != 0) {
                    QString errorPrefix = QT_TR_NOOP("Can't perform synchronization, can't get linked notebook sync state: ");
                    errorDescription.prepend(errorPrefix);
                    emit failure(errorDescription);
                    return false;
                }

                syncStateIter = m_syncStatesByLinkedNotebookGuid.insert(linkedNotebookGuid, syncState);
            }

            const qevercloud::SyncState & syncState = syncStateIter.value();

            if (syncState.fullSyncBefore > lastSyncTime)
            {
                QNDEBUG("Linked notebook sync state says the time has come to do the full sync");
                afterUsn = 0;
                if (!m_onceSyncDone) {
                    fullSyncOnly = true;
                }
                m_lastSyncMode = SyncMode::FullSync;
            }
            else if (syncState.updateCount == lastUpdateCount)
            {
                QNDEBUG("Server has no updates for data in this linked notebook, continuing with the next one");
                Q_UNUSED(m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.insert(linkedNotebookGuid));
                continue;
            }
        }

        while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
        {
            if (pSyncChunk) {
                afterUsn = pSyncChunk->chunkHighUSN;
            }

            m_linkedNotebookSyncChunks.push_back(qevercloud::SyncChunk());
            pSyncChunk = &(m_linkedNotebookSyncChunks.back());

            m_lastSyncTime = std::max(pSyncChunk->currentTime, m_lastSyncTime);
            m_lastUpdateCount = std::max(pSyncChunk->updateCount, m_lastUpdateCount);

            QString errorDescription;
            qint32 rateLimitSeconds = 0;
            qint32 errorCode = m_noteStore.getLinkedNotebookSyncChunk(linkedNotebook, afterUsn,
                                                                      m_maxSyncChunkEntries,
                                                                      authToken, fullSyncOnly, *pSyncChunk,
                                                                      errorDescription, rateLimitSeconds);
            if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
            {
                if (rateLimitSeconds <= 0) {
                    errorDescription += QString("\n") + QT_TR_NOOP("Internal error: rate limit seconds = ");
                    errorDescription += QString::number(rateLimitSeconds);
                    emit failure(errorDescription);
                    return false;
                }

                m_linkedNotebookSyncChunks.pop_back();

                int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
                if (timerId == 0) {
                    QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                          "the Evernote API call due to rate limit exceeding");
                    emit failure(errorDescription);
                    return false;
                }

                m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId = timerId;
                emit rateLimitExceeded(rateLimitSeconds);
                return false;
            }
            else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
            {
                QString errorPrefix = QT_TR_NOOP("Unexpected AUTH_EXPIRED error when trying to download "
                                                 "the linked notebook sync chunks: ");
                errorDescription.prepend(errorPrefix);
                emit failure(errorDescription);
                return false;
            }
            else if (errorCode != 0) {
                QString errorPrefix = QT_TR_NOOP("Can't perform synchronization, "
                                                 "can't download the sync chunks for "
                                                 "linked notebooks content: ");
                errorDescription.prepend(errorPrefix);
                emit failure(errorDescription);
                return false;
            }

            QNDEBUG("Received sync chunk: " << *pSyncChunk);

            if (pSyncChunk->tags.isSet())
            {
                bool res = mapContainerElementsWithLinkedNotebookGuid<TagsList>(linkedNotebookGuid, pSyncChunk->tags.ref());
                if (!res) {
                    return false;
                }
            }

            if (pSyncChunk->notebooks.isSet())
            {
                bool res = mapContainerElementsWithLinkedNotebookGuid<NotebooksList>(linkedNotebookGuid, pSyncChunk->notebooks.ref());
                if (!res) {
                    return false;
                }
            }

            if (pSyncChunk->expungedTags.isSet()) {
                unmapContainerElementsFromLinkedNotebookGuid<qevercloud::Tag>(pSyncChunk->expungedTags.ref());
            }

            if (pSyncChunk->expungedNotebooks.isSet()) {
                unmapContainerElementsFromLinkedNotebookGuid<qevercloud::Notebook>(pSyncChunk->expungedNotebooks.ref());
            }
        }

        Q_UNUSED(m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.insert(linkedNotebook.guid));
        m_lastSynchronizedUsnByLinkedNotebookGuid[linkedNotebookGuid] = afterUsn;
    }

    QNDEBUG("Done. Processing content pointed to by linked notebooks from buffered sync chunks");

    m_syncStatesByLinkedNotebookGuid.clear();   // don't need this anymore, it only served the purpose of preventing multiple get sync state calls for the same linked notebook

    m_linkedNotebooksSyncChunksDownloaded = true;
    emit linkedNotebooksSyncChunksDownloaded();

    return true;
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksTagsSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchLinkedNotebooksTagsSync");
    QList<QString> dummyList;
    launchDataElementSync<TagsList, Tag>(ContentSource::LinkedNotebook, "Tag", m_tags, dummyList);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotebooksSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotebooksSync");
    QList<QString> dummyList;
    launchDataElementSync<NotebooksList, Notebook>(ContentSource::LinkedNotebook, "Notebook", m_notebooks, dummyList);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotesSync()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotesSync");
    launchDataElementSync<NotesList, Note>(ContentSource::LinkedNotebook, "Note", m_notes, m_expungedNotes);
}

bool RemoteToLocalSynchronizationManager::hasPendingRequests() const
{
    return !(m_findTagByNameRequestIds.empty() &&
             m_findTagByGuidRequestIds.empty() &&
             m_addTagRequestIds.empty() &&
             m_updateTagRequestIds.empty() &&
             m_expungeTagRequestIds.empty() &&
             m_expungeNotelessTagsRequestId.isNull() &&
             m_findSavedSearchByNameRequestIds.empty() &&
             m_findSavedSearchByGuidRequestIds.empty() &&
             m_addSavedSearchRequestIds.empty() &&
             m_updateSavedSearchRequestIds.empty() &&
             m_expungeSavedSearchRequestIds.empty() &&
             m_findLinkedNotebookRequestIds.empty() &&
             m_addLinkedNotebookRequestIds.empty() &&
             m_updateLinkedNotebookRequestIds.empty() &&
             m_expungeLinkedNotebookRequestIds.empty() &&
             m_findNotebookByNameRequestIds.empty() &&
             m_findNotebookByGuidRequestIds.empty() &&
             m_addNotebookRequestIds.empty() &&
             m_updateNotebookRequestIds.empty() &&
             m_expungeNotebookRequestIds.empty() &&
             m_findNoteByGuidRequestIds.empty() &&
             m_addNoteRequestIds.empty() &&
             m_updateNoteRequestIds.empty() &&
             m_expungeNoteRequestIds.empty() &&
             m_findResourceByGuidRequestIds.empty() &&
             m_addResourceRequestIds.empty() &&
             m_updateResourceRequestIds.empty());
}

void RemoteToLocalSynchronizationManager::checkServerDataMergeCompletion()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkServerDataMergeCompletion");

    // Need to check whether we are still waiting for the response from some add or update request
    bool tagsReady = m_updateTagRequestIds.empty() && m_addTagRequestIds.empty();
    if (!tagsReady) {
        QNDEBUG("Tags are not ready, pending response for " << m_updateTagRequestIds.size()
                << " tag update requests and/or " << m_addTagRequestIds.size() << " tag add requests");
        return;
    }

    bool searchesReady = m_updateSavedSearchRequestIds.empty() && m_addSavedSearchRequestIds.empty();
    if (!searchesReady) {
        QNDEBUG("Saved searches are not ready, pending response for " << m_updateSavedSearchRequestIds.size()
                << " saved search update requests and/or " << m_addSavedSearchRequestIds.size() << " saved search add requests");
        return;
    }

    bool linkedNotebooksReady = m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty();
    if (!linkedNotebooksReady) {
        QNDEBUG("Linked notebooks are not ready, pending response for " << m_updateLinkedNotebookRequestIds.size()
                << " linked notebook update requests and/or " << m_addLinkedNotebookRequestIds.size() << " linked notebook add requests");
        return;
    }

    bool notebooksReady = m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty();
    if (!notebooksReady) {
        QNDEBUG("Notebooks are not ready, pending response for " << m_updateNotebookRequestIds.size()
                << " notebook update requests and/or " << m_addNotebookRequestIds.size() << " notebook add requests");
        return;
    }

    bool notesReady = m_updateNoteRequestIds.empty() && m_addNoteRequestIds.empty() &&
                      m_notesToAddPerAPICallPostponeTimerId.empty() && m_notesToUpdatePerAPICallPostponeTimerId.empty();
    if (!notesReady) {
        QNDEBUG("Notes are not ready, pending response for " << m_updateNoteRequestIds.size()
                << " note update requests and/or " << m_addNoteRequestIds.size()
                << " note add requests; also, there are " << m_notesToAddPerAPICallPostponeTimerId.size()
                << " postponed note add requests and/or " << m_notesToUpdatePerAPICallPostponeTimerId.size()
                << " note update requests");
        return;
    }

    if (m_lastSyncMode == SyncMode::IncrementalSync)
    {
        bool resourcesReady = m_updateResourceRequestIds.empty() && m_addResourceRequestIds.empty() &&
                              m_resourcesWithFindRequestIdsPerFindNoteRequestId.empty() &&
                              m_findNotebookForNotesWithConflictedResourcesRequestIds.empty();
        if (!resourcesReady) {
            QNDEBUG("Resources are not ready, pending response for " << m_updateResourceRequestIds.size()
                    << " resource update requests and/or " << m_addResourceRequestIds.size()
                    << " resource add requests and/or  " << m_resourcesWithFindRequestIdsPerFindNoteRequestId.size()
                    << " resource find note requests and/or " << m_findNotebookForNotesWithConflictedResourcesRequestIds.size()
                    << " resource find notebook requests");
            return;
        }
    }

    if (syncingLinkedNotebooksContent())
    {
        QNDEBUG("Synchronized the whole contents from linked notebooks");
        emit linkedNotebooksFullNotesContentsDownloaded();

        if (!m_expungedNotes.isEmpty()) {
            expungeNotes();
            return;
        }

        m_expungeNotelessTagsRequestId = QUuid::createUuid();
        emit expungeNotelessTagsFromLinkedNotebooks(m_expungeNotelessTagsRequestId);
    }
    else
    {
        QNDEBUG("Synchronized the whole contents from user's account");

        m_fullNoteContentsDownloaded = true;
        emit fullNotesContentsDownloaded();

        if (m_lastSyncMode == SyncMode::FullSync) {
            startLinkedNotebooksSync();
            return;
        }

        if (m_expungedFromServerToClient) {
            startLinkedNotebooksSync();
            return;
        }

        expungeFromServerToClient();
    }
}

void RemoteToLocalSynchronizationManager::finalize()
{
    m_onceSyncDone = true;
    emit finished(m_lastUpdateCount, m_lastSyncTime, m_lastUpdateCountByLinkedNotebookGuid, m_lastSyncTimeByLinkedNotebookGuid);
    clear();
    disconnectFromLocalStorage();
}

void RemoteToLocalSynchronizationManager::clear()
{
    QNDEBUG("RemoteToLocalSynchronizationManager::clear");

    // NOTE: not clearing authentication tokens by linked notebook guid hash; it is intentional,
    // this information can be reused in subsequent syncs

    m_lastSyncChunksDownloadedUsn = -1;
    m_syncChunksDownloaded = false;
    m_fullNoteContentsDownloaded = false;
    m_expungedFromServerToClient = false;
    m_linkedNotebooksSyncChunksDownloaded = false;

    m_active = false;
    m_paused = false;
    m_requestedToStop = false;

    m_syncChunks.clear();
    m_linkedNotebookSyncChunks.clear();
    m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.clear();

    m_tags.clear();
    m_expungedTags.clear();
    m_tagsToAddPerRequestId.clear();
    m_findTagByNameRequestIds.clear();
    m_findTagByGuidRequestIds.clear();
    m_addTagRequestIds.clear();
    m_updateTagRequestIds.clear();
    m_expungeTagRequestIds.clear();

    m_linkedNotebookGuidsByTagGuids.clear();
    m_expungeNotelessTagsRequestId = QUuid();

    m_savedSearches.clear();
    m_expungedSavedSearches.clear();
    m_savedSearchesToAddPerRequestId.clear();
    m_findSavedSearchByNameRequestIds.clear();
    m_findSavedSearchByGuidRequestIds.clear();
    m_addSavedSearchRequestIds.clear();
    m_updateSavedSearchRequestIds.clear();

    m_linkedNotebooks.clear();
    m_expungedLinkedNotebooks.clear();
    m_findLinkedNotebookRequestIds.clear();
    m_addLinkedNotebookRequestIds.clear();
    m_updateLinkedNotebookRequestIds.clear();
    m_expungeSavedSearchRequestIds.clear();

    m_syncStatesByLinkedNotebookGuid.clear();
    // NOTE: not clearing last synchronized usns by linked notebook guid; it is intentional,
    // this information can be reused in subsequent syncs

    m_notebooks.clear();
    m_expungedNotebooks.clear();
    m_notebooksToAddPerRequestId.clear();
    m_findNotebookByNameRequestIds.clear();
    m_findNotebookByGuidRequestIds.clear();
    m_addNotebookRequestIds.clear();
    m_updateNotebookRequestIds.clear();
    m_expungeNotebookRequestIds.clear();

    m_linkedNotebookGuidsByNotebookGuids.clear();

    m_notes.clear();
    m_expungedNotes.clear();
    m_findNoteByGuidRequestIds.clear();
    m_addNoteRequestIds.clear();
    m_updateNoteRequestIds.clear();
    m_expungeNoteRequestIds.clear();

    m_notesWithFindRequestIdsPerFindNotebookRequestId.clear();
    m_notebooksPerNoteGuids.clear();

    m_resources.clear();
    m_findResourceByGuidRequestIds.clear();
    m_addResourceRequestIds.clear();
    m_updateResourceRequestIds.clear();
    m_resourcesWithFindRequestIdsPerFindNoteRequestId.clear();
    m_resourceFoundFlagPerFindResourceRequestId.clear();
    m_notesPerResourceGuids.clear();
    m_resourceConflictedAndRemoteNotesPerNotebookGuid.clear();
    m_findNotebookForNotesWithConflictedResourcesRequestIds.clear();

    m_localGuidsOfElementsAlreadyAttemptedToFindByName.clear();

    auto notesToAddPerAPICallPostponeTimerIdEnd = m_notesToAddPerAPICallPostponeTimerId.end();
    for(auto it = m_notesToAddPerAPICallPostponeTimerId.begin(); it != notesToAddPerAPICallPostponeTimerIdEnd; ++it) {
        int key = it.key();
        killTimer(key);
    }
    m_notesToAddPerAPICallPostponeTimerId.clear();

    auto notesToUpdateAndToAddLaterPerAPICallPostponeTimerIdEnd = m_notesToUpdatePerAPICallPostponeTimerId.end();
    for(auto it = m_notesToUpdatePerAPICallPostponeTimerId.begin(); it != notesToUpdateAndToAddLaterPerAPICallPostponeTimerIdEnd; ++it) {
        int key = it.key();
        killTimer(key);
    }
    m_notesToUpdatePerAPICallPostponeTimerId.clear();

    auto afterUsnForSyncChunkPerAPICallPostponeTimerIdEnd = m_afterUsnForSyncChunkPerAPICallPostponeTimerId.end();
    for(auto it = m_afterUsnForSyncChunkPerAPICallPostponeTimerId.begin(); it != afterUsnForSyncChunkPerAPICallPostponeTimerIdEnd; ++it) {
        int key = it.key();
        killTimer(key);
    }
    m_afterUsnForSyncChunkPerAPICallPostponeTimerId.clear();

    if (m_getSyncStateBeforeStartAPICallPostponeTimerId != 0) {
        killTimer(m_getSyncStateBeforeStartAPICallPostponeTimerId);
        m_getSyncStateBeforeStartAPICallPostponeTimerId = 0;
    }

    if (m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId != 0) {
        killTimer(m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId);
        m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId = 0;
    }

    if (m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId != 0) {
        killTimer(m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId);
        m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId = 0;
    }
}

void RemoteToLocalSynchronizationManager::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::timerEvent");

    if (!pEvent) {
        QString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);
    QNDEBUG("Killed timer with id " << timerId);

    if (m_paused)
    {
        QNDEBUG("RemoteToLocalSynchronizationManager is being paused. Won't try to download full Note data "
                "but will return the notes waiting to be downloaded into the common list of notes");

        const int numNotesToAdd = m_notesToAddPerAPICallPostponeTimerId.size();
        const int numNotesToUpdate = m_notesToUpdatePerAPICallPostponeTimerId.size();
        const int numNotesInCommonList = m_notes.size();

        m_notes.reserve(std::max(numNotesToAdd + numNotesToUpdate + numNotesInCommonList, 0));

        typedef QHash<int,Note>::const_iterator CIter;

        CIter notesToAddEnd = m_notesToAddPerAPICallPostponeTimerId.constEnd();
        for(CIter it = m_notesToAddPerAPICallPostponeTimerId.constBegin(); it != notesToAddEnd; ++it) {
            m_notes.push_back(it.value());
        }
        m_notesToAddPerAPICallPostponeTimerId.clear();

        CIter notesToUpdateEnd = m_notesToUpdatePerAPICallPostponeTimerId.constEnd();
        for(CIter it = m_notesToUpdatePerAPICallPostponeTimerId.constBegin(); it != notesToUpdateEnd; ++it) {
            m_notes.push_back(it.value());
        }
        m_notesToUpdatePerAPICallPostponeTimerId.clear();

        m_afterUsnForSyncChunkPerAPICallPostponeTimerId.clear();
        m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId = 0;
        m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId = 0;

        return;
    }

    auto noteToAddIt = m_notesToAddPerAPICallPostponeTimerId.find(timerId);
    if (noteToAddIt != m_notesToAddPerAPICallPostponeTimerId.end())
    {
        Note note = noteToAddIt.value();

        Q_UNUSED(m_notesToAddPerAPICallPostponeTimerId.erase(noteToAddIt));

        QString errorDescription;
        qint32 postponeAPICallSeconds = tryToGetFullNoteData(note, errorDescription);
        if (postponeAPICallSeconds > 0)
        {
            int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
            if (timerId == 0) {
                QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                      "the Evernote API call due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_notesToAddPerAPICallPostponeTimerId[timerId] = note;
            emit rateLimitExceeded(postponeAPICallSeconds);
        }
        else if (postponeAPICallSeconds == 0)
        {
            emitAddRequest(note);
        }

        return;
    }

    auto noteToUpdateIt = m_notesToUpdatePerAPICallPostponeTimerId.find(timerId);
    if (noteToUpdateIt != m_notesToUpdatePerAPICallPostponeTimerId.end())
    {
        Note noteToUpdate = noteToUpdateIt.value();

        Q_UNUSED(m_notesToUpdatePerAPICallPostponeTimerId.erase(noteToUpdateIt));

        QString errorDescription;
        qint32 postponeAPICallSeconds = tryToGetFullNoteData(noteToUpdate, errorDescription);
        if (postponeAPICallSeconds > 0)
        {
            int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
            if (timerId == 0) {
                QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                      "the Evernote API call due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_notesToUpdatePerAPICallPostponeTimerId[timerId] = noteToUpdate;
            emit rateLimitExceeded(postponeAPICallSeconds);
        }
        else if (postponeAPICallSeconds == 0)
        {
            // NOTE: workarounding the stupidity of MSVC 2013
            emitUpdateRequest<Note>(noteToUpdate, static_cast<const Note*>(nullptr));
        }

        return;
    }

    auto afterUsnIt = m_afterUsnForSyncChunkPerAPICallPostponeTimerId.find(timerId);
    if (afterUsnIt != m_afterUsnForSyncChunkPerAPICallPostponeTimerId.end())
    {
        qint32 afterUsn = afterUsnIt.value();

        Q_UNUSED(m_afterUsnForSyncChunkPerAPICallPostponeTimerId.erase(afterUsnIt));

        downloadSyncChunksAndLaunchSync(afterUsn);
        return;
    }

    if (timerId == m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId) {
        m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId = 0;
        startLinkedNotebooksSync();
        return;
    }

    if (timerId == m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId) {
        m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId = 0;
        startLinkedNotebooksSync();
        return;
    }

    if (m_getSyncStateBeforeStartAPICallPostponeTimerId == timerId) {
        m_getSyncStateBeforeStartAPICallPostponeTimerId = 0;
        start(m_lastUsnOnStart);
        return;
    }
}

qint32 RemoteToLocalSynchronizationManager::tryToGetFullNoteData(Note & note, QString & errorDescription)
{
    if (!note.hasGuid()) {
        errorDescription = QT_TR_NOOP("detected attempt to get full note's data for note without guid");
        QNWARNING(errorDescription << ": " << note);
        return -1;
    }

    bool withContent = true;
    bool withResourceData = true;
    bool withResourceRecognition = true;
    bool withResourceAlternateData = true;
    qint32 rateLimitSeconds = 0;

    qint32 errorCode = m_noteStore.getNote(withContent, withResourceData,
                                           withResourceRecognition,
                                           withResourceAlternateData,
                                           note, errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds <= 0) {
            errorDescription = QT_TR_NOOP("QEverCloud or Evernote protocol error: caught RATE_LIMIT_REACHED "
                                          "exception but the number of seconds to wait is zero or negative: ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            return -1;
        }

        return rateLimitSeconds;
    }
    else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        handleAuthExpiration();
        return -1;
    }
    else if (errorCode != 0) {
        emit failure(errorDescription);
        return -1;
    }

    return 0;
}

void RemoteToLocalSynchronizationManager::downloadSyncChunksAndLaunchSync(qint32 afterUsn)
{
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    QNDEBUG("Downloading sync chunks:");

    while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
    {
        if (pSyncChunk) {
            afterUsn = pSyncChunk->chunkHighUSN;
        }

        m_syncChunks.push_back(qevercloud::SyncChunk());
        pSyncChunk = &(m_syncChunks.back());

        m_lastSyncTime = std::max(pSyncChunk->currentTime, m_lastSyncTime);
        m_lastUpdateCount = std::max(pSyncChunk->updateCount, m_lastUpdateCount);

        qevercloud::SyncChunkFilter filter;
        filter.includeNotebooks = true;
        filter.includeNotes = true;
        filter.includeTags = true;
        filter.includeSearches = true;
        filter.includeNoteResources = true;
        filter.includeNoteAttributes = true;
        filter.includeNoteApplicationDataFullMap = true;
        filter.includeNoteResourceApplicationDataFullMap = true;
        filter.includeLinkedNotebooks = true;

        if (m_lastSyncMode == SyncMode::IncrementalSync) {
            filter.includeExpunged = true;
            filter.includeResources = true;
        }

        QString errorDescription;
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = m_noteStore.getSyncChunk(afterUsn, m_maxSyncChunkEntries, filter,
                                                    *pSyncChunk, errorDescription, rateLimitSeconds);
        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QString("\n") + QT_TR_NOOP("Internal error: rate limit seconds = ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            m_syncChunks.pop_back();

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (timerId == 0) {
                QString errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone "
                                                      "the Evernote API call due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_afterUsnForSyncChunkPerAPICallPostponeTimerId[timerId] = afterUsn;
            emit rateLimitExceeded(rateLimitSeconds);
            return;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            handleAuthExpiration();
            return;
        }
        else if (errorCode != 0) {
            QString errorPrefix = QT_TR_NOOP("Can't perform synchronization, "
                                             "can't download the sync chunks: ");
            errorDescription.prepend(errorPrefix);
            emit failure(errorDescription);
            return;
        }

        QNDEBUG("Received sync chunk: " << *pSyncChunk);
    }

    QNDEBUG("Done. Processing tags, saved searches, linked notebooks and notebooks "
            "from buffered sync chunks");

    m_lastSyncChunksDownloadedUsn = afterUsn;
    m_syncChunksDownloaded = true;
    emit syncChunksDownloaded();

    launchSync();
}

const Notebook * RemoteToLocalSynchronizationManager::getNotebookPerNote(const Note & note) const
{
    QNDEBUG("RemoteToLocalSynchronizationManager::getNotebookPerNote: note = " << note);

    QString noteGuid = (note.hasGuid() ? note.guid() : QString());
    QString noteLocalGuid = note.localGuid();

    QPair<QString,QString> key(noteGuid, noteLocalGuid);
    QHash<QPair<QString,QString>,Notebook>::const_iterator cit = m_notebooksPerNoteGuids.find(key);
    if (cit == m_notebooksPerNoteGuids.end()) {
        return nullptr;
    }
    else {
        return &(cit.value());
    }
}

void RemoteToLocalSynchronizationManager::handleAuthExpiration()
{
    QNINFO("Got AUTH_EXPIRED error, pausing and requesting new authentication token");
    m_paused = true;
    emit paused(/* pending authentication = */ true);
    emit requestAuthenticationToken();
}

bool RemoteToLocalSynchronizationManager::checkUserAccountSyncState(bool & asyncWait, bool & error, qint32 & afterUsn)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkUsersAccountSyncState");

    asyncWait = false;
    error = false;

    QString errorDescription;
    qint32 rateLimitSeconds = 0;
    qevercloud::SyncState state;
    qint32 errorCode = m_noteStore.getSyncState(state, errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds < 0) {
            errorDescription = QT_TR_NOOP("Caught RATE_LIMIT_REACHED exception but "
                    "the number of seconds to wait is negative: ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            error = true;
            return false;
        }

        m_getSyncStateBeforeStartAPICallPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        if (m_getSyncStateBeforeStartAPICallPostponeTimerId == 0) {
            errorDescription = QT_TR_NOOP("Internal error: can't start timer to postpone the Evernote API call "
                    "due to rate limit exceeding");
            emit failure(errorDescription);
            error = true;
        }
        else {
            asyncWait = true;
        }

        return false;
    }
    else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        handleAuthExpiration();
        asyncWait = true;
        return false;
    }
    else if (errorCode != 0)
    {
        emit failure(errorDescription);
        error = true;
        return false;
    }

    if (state.fullSyncBefore > m_lastSyncTime)
    {
        QNDEBUG("Sync state says the time has come to do the full sync");
        afterUsn = 0;
        m_lastSyncMode = SyncMode::FullSync;
    }
    else if (state.updateCount == m_lastUpdateCount)
    {
        QNDEBUG("Server has no updates for user's data since the last sync");
        return false;
    }

    return true;
}

bool RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncStates(bool & asyncWait, bool & error)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncStates");

    // TODO: implement
    // 1) check that the list of all linked notebooks from local storage has been obtained, if not, obtain it
    // 2) get sync state for each linked notebook individually, keep the "afterUsn" for each of them (along with other info useful for the sync)
    // 3) return false if none of all linked notebooks has any updates from the service, return true otherwise

    return true;
}

QTextStream & operator<<(QTextStream & strm, const RemoteToLocalSynchronizationManager::SyncMode::type & obj)
{
    switch(obj)
    {
    case RemoteToLocalSynchronizationManager::SyncMode::FullSync:
        strm << "FullSync";
        break;
    case RemoteToLocalSynchronizationManager::SyncMode::IncrementalSync:
        strm << "IncrementalSync";
        break;
    default:
        strm << "<unknown>";
        break;
    }

    return strm;
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::TagsList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                    RemoteToLocalSynchronizationManager::TagsList & container)
{
    if (syncChunk.tags.isSet()) {
        const auto & tags = syncChunk.tags.ref();
        container.append(tags);
    }

    if (syncChunk.expungedTags.isSet())
    {
        const auto & expungedTags = syncChunk.expungedTags.ref();
        const auto expungedTagsEnd = expungedTags.end();
        for(auto eit = expungedTags.begin(); eit != expungedTagsEnd; ++eit)
        {
            const QString & tagGuid = *eit;
            if (tagGuid.isEmpty()) {
                QString error = QT_TR_NOOP("Error during sync: found empty expunged tag guid in the sync chunk");
                QNWARNING(error);
                emit failure(error);
                return;
            }

            TagsList::iterator it = std::find_if(container.begin(), container.end(),
                                                 CompareItemByGuid<qevercloud::Tag>(tagGuid));
            if (it == container.end()) {
                continue;
            }

            Q_UNUSED(container.erase(it));
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::SavedSearchesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                             RemoteToLocalSynchronizationManager::SavedSearchesList & container)
{
    if (syncChunk.searches.isSet()) {
        container.append(syncChunk.searches.ref());
    }

    if (syncChunk.expungedSearches.isSet())
    {
        const auto & expungedSearches = syncChunk.expungedSearches.ref();
        const auto expungedSearchesEnd = expungedSearches.end();
        for(auto eit = expungedSearches.begin(); eit != expungedSearchesEnd; ++eit)
        {
            SavedSearchesList::iterator it = std::find_if(container.begin(), container.end(),
                                                          CompareItemByGuid<qevercloud::SavedSearch>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::LinkedNotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                               RemoteToLocalSynchronizationManager::LinkedNotebooksList & container)
{
    if (syncChunk.linkedNotebooks.isSet()) {
        container.append(syncChunk.linkedNotebooks.ref());
    }

    if (syncChunk.expungedLinkedNotebooks.isSet())
    {
        const auto & expungedLinkedNotebooks = syncChunk.expungedLinkedNotebooks.ref();
        const auto expungedLinkedNotebooksEnd = expungedLinkedNotebooks.end();
        for(auto eit = expungedLinkedNotebooks.begin(); eit != expungedLinkedNotebooksEnd; ++eit)
        {
            LinkedNotebooksList::iterator it = std::find_if(container.begin(), container.end(),
                                                            CompareItemByGuid<qevercloud::LinkedNotebook>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::NotebooksList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                         RemoteToLocalSynchronizationManager::NotebooksList & container)
{
    if (syncChunk.notebooks.isSet()) {
        container.append(syncChunk.notebooks.ref());
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        const auto & expungedNotebooks = syncChunk.expungedNotebooks.ref();
        const auto expungedNotebooksEnd = expungedNotebooks.end();
        for(auto eit = expungedNotebooks.begin(); eit != expungedNotebooksEnd; ++eit)
        {
            NotebooksList::iterator it = std::find_if(container.begin(), container.end(),
                                                      CompareItemByGuid<qevercloud::Notebook>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::NotesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                     RemoteToLocalSynchronizationManager::NotesList & container)
{
    if (syncChunk.notes.isSet()) {
        container.append(syncChunk.notes.ref());
    }

    if (syncChunk.expungedNotes.isSet())
    {
        const auto & expungedNotes = syncChunk.expungedNotes.ref();
        const auto expungedNotesEnd = expungedNotes.end();
        for(auto eit = expungedNotes.begin(); eit != expungedNotesEnd; ++eit)
        {
            NotesList::iterator it = std::find_if(container.begin(), container.end(),
                                                  CompareItemByGuid<qevercloud::Note>(*eit));
            if (it != container.end()) {
                Q_UNUSED(container.erase(it));
            }
        }
    }

    if (syncChunk.expungedNotebooks.isSet())
    {
        const auto & expungedNotebooks = syncChunk.expungedNotebooks.ref();
        const auto expungedNotebooksEnd = expungedNotebooks.end();
        for(auto eit = expungedNotebooks.begin(); eit != expungedNotebooksEnd; ++eit)
        {
            const QString & expungedNotebookGuid = *eit;

            for(auto it = container.begin(); it != container.end();)
            {
                qevercloud::Note & note = *it;
                if (note.notebookGuid.isSet() && (note.notebookGuid.ref() == expungedNotebookGuid)) {
                    it = container.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }
}

template <>
void RemoteToLocalSynchronizationManager::appendDataElementsFromSyncChunkToContainer<RemoteToLocalSynchronizationManager::ResourcesList>(const qevercloud::SyncChunk & syncChunk,
                                                                                                                                         RemoteToLocalSynchronizationManager::ResourcesList & container)
{
    if (syncChunk.resources.isSet()) {
        const auto & resources = syncChunk.resources.ref();
        container.append(resources);
    }
}

template <class ContainerType, class ElementType>
typename ContainerType::iterator RemoteToLocalSynchronizationManager::findItemByName(ContainerType & container,
                                                                                     const ElementType & element,
                                                                                     const QString & typeName)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::findItemByName<" << typeName << ">");

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    typename ContainerType::iterator it = container.begin();
    if (!frontItem.name.isSet() || (frontItem.name.ref() != element.name()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByName<typename ContainerType::value_type>(element.name()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                                  "of remote elements waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template<>
RemoteToLocalSynchronizationManager::NotesList::iterator RemoteToLocalSynchronizationManager::findItemByName<RemoteToLocalSynchronizationManager::NotesList, Note>(NotesList & container,
                                                                                                                                                                   const Note & element,
                                                                                                                                                                   const QString & typeName)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::findItemByName<" << typeName << ">");

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasTitle()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty name in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    NotesList::iterator it = container.begin();
    if (!frontItem.title.isSet() || (frontItem.title.ref() != element.title()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByName<qevercloud::Note>(element.title()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("Can't find " + typeName + " by name within the list "
                                                  "of remote elements waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template <class ContainerType, class ElementType>
typename ContainerType::iterator RemoteToLocalSynchronizationManager::findItemByGuid(ContainerType & container,
                                                                                     const ElementType & element,
                                                                                     const QString & typeName)
{
    QNDEBUG("RemoteToLocalSynchronizationManager::findItemByGuid<" << typeName << ">");

    // Attempt to find this data element by guid within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasGuid()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " with empty guid in local storage");
        QNWARNING(errorDescription << ": " << typeName << " = " << element);
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        QString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list "
                                              "of remote elements waiting for processing but that list is empty");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return container.end();
    }

    // Try the front element first, in most cases it should be it
    const auto & frontItem = container.front();
    typename ContainerType::iterator it = container.begin();
    if (!frontItem.guid.isSet() || (frontItem.guid.ref() != element.guid()))
    {
        it = std::find_if(container.begin(), container.end(),
                          CompareItemByGuid<typename ContainerType::value_type>(element.guid()));
        if (it == container.end()) {
            QString errorDescription = QT_TR_NOOP("can't find " + typeName + " by guid within the list "
                                                  "of elements from the remote storage waiting for processing");
            QNWARNING(errorDescription << ": " << element);
            emit failure(errorDescription);
            return container.end();
        }
    }

    return it;
}

template <class T>
bool RemoteToLocalSynchronizationManager::CompareItemByName<T>::operator()(const T & item) const
{
    if (item.name.isSet()) {
        return (m_name.toUpper() == item.name.ref().toUpper());
    }
    else {
        return false;
    }
}

template <>
bool RemoteToLocalSynchronizationManager::CompareItemByName<qevercloud::Note>::operator()(const qevercloud::Note & item) const
{
    if (item.title.isSet()) {
        return (m_name.toUpper() == item.title->toUpper());
    }
    else {
        return false;
    }
}

template <class T>
bool RemoteToLocalSynchronizationManager::CompareItemByGuid<T>::operator()(const T & item) const
{
    if (item.guid.isSet()) {
        return (m_guid == item.guid.ref());
    }
    else {
        return false;
    }
}

template <class ContainerType, class ElementType>
void RemoteToLocalSynchronizationManager::launchDataElementSync(const ContentSource::type contentSource, const QString & typeName,
                                                                ContainerType & container, QList<QString> & expungedElements)
{
    bool syncingUserAccountData = (contentSource == ContentSource::UserAccount);
    const auto & syncChunks = (syncingUserAccountData ? m_syncChunks : m_linkedNotebookSyncChunks);

    container.clear();
    int numSyncChunks = syncChunks.size();
    for(int i = 0; i < numSyncChunks; ++i) {
        const qevercloud::SyncChunk & syncChunk = syncChunks[i];
        appendDataElementsFromSyncChunkToContainer<ContainerType>(syncChunk, container);
        extractExpungedElementsFromSyncChunk<ElementType>(syncChunk, expungedElements);
    }

    if (container.empty()) {
        return;
    }

    int numElements = container.size();
    for(int i = 0; i < numElements; ++i)
    {
        const typename ContainerType::value_type & remoteElement = container[i];
        if (!remoteElement.guid.isSet()) {
            QString errorDescription = QT_TR_NOOP("found " + typeName + " from the remote storage "
                                                  "without guid set");
            QNWARNING(errorDescription << ": " << remoteElement);
            emit failure(errorDescription);
            return;
        }

        emitFindByGuidRequest<ElementType>(remoteElement.guid.ref());
    }
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk(const qevercloud::SyncChunk & syncChunk, QList<QString> & expungedElementGuids)
{
    // do nothing by default
}

template <>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk<Tag>(const qevercloud::SyncChunk & syncChunk,
                                                                                    QList<QString> & expungedElementGuids)
{
    if (syncChunk.expungedTags.isSet()) {
        expungedElementGuids = syncChunk.expungedTags.ref();
    }
}

template <>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk<SavedSearch>(const qevercloud::SyncChunk & syncChunk,
                                                                                            QList<QString> & expungedElementGuids)
{
    if (syncChunk.expungedSearches.isSet()) {
        expungedElementGuids = syncChunk.expungedSearches.ref();
    }
}

template <>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk<Notebook>(const qevercloud::SyncChunk & syncChunk,
                                                                                         QList<QString> & expungedElementGuids)
{
    if (syncChunk.expungedNotebooks.isSet()) {
        expungedElementGuids = syncChunk.expungedNotebooks.ref();
    }
}

template <>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk<Note>(const qevercloud::SyncChunk & syncChunk,
                                                                                     QList<QString> & expungedElementGuids)
{
    if (syncChunk.expungedNotes.isSet()) {
        expungedElementGuids = syncChunk.expungedNotes.ref();
    }
}

template <>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk<LinkedNotebook>(const qevercloud::SyncChunk & syncChunk,
                                                                                               QList<QString> & expungedElementGuids)
{
    if (syncChunk.expungedLinkedNotebooks.isSet()) {
        expungedElementGuids = syncChunk.expungedLinkedNotebooks.ref();
    }
}

template <class ElementType>
void setConflictedBase(const QString & typeName, ElementType & element)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    element.setGuid(QString());
    element.setName(QObject::tr("Conflicted ") + typeName + element.name() + " (" + currentDateTime + ")");
    element.setDirty(true);
}

template <>
void setConflictedBase<Note>(const QString & typeName, Note & note)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    note.setGuid(QString());
    note.setTitle(QObject::tr("Conflicted ") + typeName + (note.hasTitle() ? note.title() : "note") + " (" + currentDateTime + ")");
    note.setDirty(true);
    note.setLocal(true);
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::setConflicted(const QString & typeName, ElementType & element)
{
    setConflictedBase(typeName, element);
}

template <>
void RemoteToLocalSynchronizationManager::setConflicted<Tag>(const QString & typeName, Tag & tag)
{
    setConflictedBase(typeName, tag);
    tag.setLocal(true);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByNameRequest<Tag>(const Tag & tag)
{
    if (!tag.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected tag from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << tag);
        emit failure(errorDescription);
        return;
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findTagByNameRequestIds.insert(findElementRequestId));
    emit findTag(tag, findElementRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByNameRequest<SavedSearch>(const SavedSearch & search)
{
    if (!search.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected saved search from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << search);
        emit failure(errorDescription);
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchByNameRequestIds.insert(findElementRequestId));
    emit findSavedSearch(search, findElementRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByNameRequest<Notebook>(const Notebook & notebook)
{
    if (!notebook.hasName()) {
        QString errorDescription = QT_TR_NOOP("detected notebook from remote storage which is "
                                              "to be searched by name in local storage but "
                                              "it has no name set");
        QNWARNING(errorDescription << ": " << notebook);
        emit failure(errorDescription);
    }

    QUuid findElementRequestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookByNameRequestIds.insert(findElementRequestId));
    emit findNotebook(notebook, findElementRequestId);
}

template <class ContainerType, class ElementType>
bool RemoteToLocalSynchronizationManager::onFoundDuplicateByName(ElementType element, const QUuid & requestId,
                                                                 const QString & typeName, ContainerType & container,
                                                                 QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("RemoteToLocalSynchronizationManager::onFoundDuplicateByName<" << typeName << ">: "
            << typeName << " = " << element << ", requestId  = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByName(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // The element exists both in the client and in the server
    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("Found " + typeName + " from sync chunk without the update sequence number");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return true;
    }

    ElementType remoteElementAdapter(remoteElement);
    checkAndAddLinkedNotebookBinding(element, remoteElementAdapter);

    checkUpdateSequenceNumbersAndProcessConflictedElements(remoteElementAdapter, typeName, element);

    Q_UNUSED(container.erase(it));

    return true;
}

template <class ElementType, class ContainerType>
bool RemoteToLocalSynchronizationManager::onFoundDuplicateByGuid(ElementType element, const QUuid & requestId,
                                                                 const QString & typeName, ContainerType & container,
                                                                 QSet<QUuid> & findByGuidRequestIds)
{
    typename QSet<QUuid>::iterator rit = findByGuidRequestIds.find(requestId);
    if (rit == findByGuidRequestIds.end()) {
        return false;
    }

    QNDEBUG("onFoundDuplicateByGuid<" << typeName << ">: "
            << typeName << " = " << element << ", requestId = " << requestId);

    typename ContainerType::iterator it = findItemByGuid(container, element, typeName);
    if (it == container.end()) {
        QString errorDescription = QT_TR_NOOP("could not find the remote " + typeName + " by guid "
                                              "when reported of duplicate by guid in the local storage");
        QNWARNING(errorDescription << ": " << element);
        emit failure(errorDescription);
        return true;
    }

    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QString errorDescription = QT_TR_NOOP("found " + typeName + " from remote storage without "
                                              "the update sequence number set");
        QNWARNING(errorDescription << ": " << remoteElement);
        emit failure(errorDescription);
        return true;
    }

    ElementType remoteElementAdapter(remoteElement);
    checkAndAddLinkedNotebookBinding(element, remoteElementAdapter);

    checkUpdateSequenceNumbersAndProcessConflictedElements(remoteElementAdapter, typeName, element);

    Q_UNUSED(container.erase(it));
    Q_UNUSED(findByGuidRequestIds.erase(rit));

    return true;
}

template <class ContainerType, class ElementType>
bool RemoteToLocalSynchronizationManager::onNoDuplicateByGuid(ElementType element, const QUuid & requestId,
                                                              const QString & errorDescription,
                                                              const QString & typeName, ContainerType & container,
                                                              QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("RemoteToLocalSynchronizationManager::onNoDuplicateByGuid<" << typeName << ">: " << element
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByGuid(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // This element wasn't found in the local storage by guid, need to check whether
    // the element with similar name exists
    ElementType elementToFindByName(*it);
    emitFindByNameRequest(elementToFindByName);

    return true;
}

template <class ContainerType, class ElementType>
bool RemoteToLocalSynchronizationManager::onNoDuplicateByName(ElementType element, const QUuid & requestId,
                                                              const QString & errorDescription,
                                                              const QString & typeName, ContainerType & container,
                                                              QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG("RemoteToLocalSynchronizationManager::onNoDuplicateByUniqueKey<" << typeName << ">: " << element
            << ", errorDescription = " << errorDescription << ", requestId = " << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByName(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // This element wasn't found in the local storage by guid or name ==> it's new from remote storage, adding it
    ElementType newElement(*it);
    checkAndAddLinkedNotebookBinding(element, newElement);

    emitAddRequest(newElement);

    // also removing the element from the list of ones waiting for processing
    Q_UNUSED(container.erase(it));

    return true;
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::processConflictedElement(const ElementType & remoteElement,
                                                                   const QString & typeName, ElementType & element)
{
    setConflicted(typeName, element);

    emitUpdateRequest(element, &remoteElement);
}

template <>
void RemoteToLocalSynchronizationManager::processConflictedElement(const LinkedNotebook & remoteLinkedNotebook,
                                                                   const QString &, LinkedNotebook & linkedNotebook)
{
    // Linked notebook itself is simply a pointer to another user's account;
    // The data it points to would have a separate synchronization procedure
    // including the synchronization for Notebook, Notes and Tags from another user's account
    // The linked notebook as a pointer to all this data would simply be overridden
    // by the server's version of it
    linkedNotebook = remoteLinkedNotebook;

    QUuid updateRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateRequestId);
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::checkAndAddLinkedNotebookBinding(const ElementType & sourceElement,
                                                                           ElementType & targetElement)
{
    // Do nothing in default instantiation, only tags and notebooks need to be processed specifically
}

template <>
void RemoteToLocalSynchronizationManager::checkAndAddLinkedNotebookBinding<Notebook>(const Notebook & sourceNotebook,
                                                                                     Notebook & targetNotebook)
{
    if (!sourceNotebook.hasGuid()) {
        return;
    }

    auto it = m_linkedNotebookGuidsByNotebookGuids.find(sourceNotebook.guid());
    if (it == m_linkedNotebookGuidsByNotebookGuids.end()) {
        return;
    }

    targetNotebook.setLinkedNotebookGuid(it.value());
}

template <>
void RemoteToLocalSynchronizationManager::checkAndAddLinkedNotebookBinding<Tag>(const Tag & sourceTag, Tag & targetTag)
{
    if (!sourceTag.hasGuid()) {
        return;
    }

    auto it = m_linkedNotebookGuidsByTagGuids.find(sourceTag.guid());
    if (it == m_linkedNotebookGuidsByTagGuids.end()) {
        return;
    }

    targetTag.setLinkedNotebookGuid(it.value());
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::checkUpdateSequenceNumbersAndProcessConflictedElements(const ElementType & remoteElement,
                                                                                                 const QString & typeName,
                                                                                                 ElementType & localElement)
{
    if ( !localElement.hasUpdateSequenceNumber() ||
         (remoteElement.updateSequenceNumber() > localElement.updateSequenceNumber()) )
    {
        if (!localElement.isDirty())
        {
            // Remote element is more recent, need to update the element existing in local storage
            ElementType elementToUpdate(remoteElement);
            unsetLocalGuid(elementToUpdate);

            // NOTE: workarounding the stupidity of MSVC 2013
            emitUpdateRequest<ElementType>(elementToUpdate, static_cast<const ElementType*>(nullptr));
        }
        else
        {
            // Remote element is more recent but the local one has been modified;
            // Evernote's synchronization protocol description suggests trying
            // to do field-by-field merge but it's overcomplicated and error-prone;
            // it's much easier to rename the existing local element to make it clear
            // it has a conflict with its remote counterpart and mark it dirty so that
            // it would be sent to the server along with other local changes
            processConflictedElement(remoteElement, typeName, localElement);
        }
    }
}

} // namespace qute_not e
