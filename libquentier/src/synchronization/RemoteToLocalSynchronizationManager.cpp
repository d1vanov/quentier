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

#include "RemoteToLocalSynchronizationManager.h"
#include "InkNoteImageDownloader.h"
#include "NoteThumbnailDownloader.h"
#include <quentier/utility/Utility.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/types/Resource.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QTimerEvent>
#include <QThreadPool>
#include <QSysInfo>
#include <algorithm>

#if defined(Q_OS_WIN32)
#include <windows.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD) || defined(Q_OS_OPENBSD)
#include <sys/utsname.h>
#endif

#define ACCOUNT_LIMITS_KEY_GROUP QStringLiteral("account_limits/")
#define ACCOUNT_LIMITS_LAST_SYNC_TIME_KEY QStringLiteral("last_sync_time")
#define ACCOUNT_LIMITS_SERVICE_LEVEL_KEY QStringLiteral("service_level")
#define ACCOUNT_LIMITS_USER_MAIL_LIMIT_DAILY_KEY QStringLiteral("user_mail_limit_daily")
#define ACCOUNT_LIMITS_NOTE_SIZE_MAX_KEY QStringLiteral("note_size_max")
#define ACCOUNT_LIMITS_RESOURCE_SIZE_MAX_KEY QStringLiteral("resource_size_max")
#define ACCOUNT_LIMITS_USER_LINKED_NOTEBOOK_MAX_KEY QStringLiteral("user_linked_notebook_max")
#define ACCOUNT_LIMITS_UPLOAD_LIMIT_KEY QStringLiteral("upload_limit")
#define ACCOUNT_LIMITS_USER_NOTE_COUNT_MAX_KEY QStringLiteral("user_note_count_max")
#define ACCOUNT_LIMITS_USER_NOTEBOOK_COUNT_MAX_KEY QStringLiteral("user_notebook_count_max")
#define ACCOUNT_LIMITS_USER_TAG_COUNT_MAX_KEY QStringLiteral("user_tag_count_max")
#define ACCOUNT_LIMITS_NOTE_TAG_COUNT_MAX_KEY QStringLiteral("note_tag_count_max")
#define ACCOUNT_LIMITS_USER_SAVED_SEARCH_COUNT_MAX_KEY QStringLiteral("user_saved_search_count_max")
#define ACCOUNT_LIMITS_NOTE_RESOURCE_COUNT_MAX_KEY QStringLiteral("note_resource_count_max")

#define THIRTY_DAYS_IN_MSEC (2592000000)

namespace quentier {

RemoteToLocalSynchronizationManager::RemoteToLocalSynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                                                                         const QString & host, QSharedPointer<qevercloud::NoteStore> pNoteStore,
                                                                         QSharedPointer<qevercloud::UserStore> pUserStore,
                                                                         QObject * parent) :
    QObject(parent),
    m_localStorageManagerThreadWorker(localStorageManagerThreadWorker),
    m_connectedToLocalStorage(false),
    m_host(host),
    m_noteStore(pNoteStore),
    m_userStore(pUserStore),
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
    m_edamProtocolVersionChecked(false),
    m_syncChunks(),
    m_linkedNotebookSyncChunks(),
    m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded(),
    m_accountLimits(),
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
    m_allLinkedNotebooks(),
    m_listAllLinkedNotebooksRequestId(),
    m_allLinkedNotebooksListed(false),
    m_authenticationToken(),
    m_userId(-1),
    m_shardId(),
    m_authenticationTokenExpirationTime(0),
    m_pendingAuthenticationTokenAndShardId(false),
    m_user(),
    m_findUserRequestId(),
    m_addOrUpdateUserRequestId(),
    m_authenticationTokensAndShardIdsByLinkedNotebookGuid(),
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid(),
    m_pendingAuthenticationTokensForLinkedNotebooks(false),
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
    m_inkNoteResourceDataPerFindNotebookRequestId(),
    m_numPendingInkNoteImageDownloads(0),
    m_noteGuidForThumbnailDownloadByFindNotebookRequestId(),
    m_noteGuidsPendingThumbnailDownload(),
    m_resourceFoundFlagPerFindResourceRequestId(),
    m_resourceConflictedAndRemoteNotesPerNotebookGuid(),
    m_findNotebookForNotesWithConflictedResourcesRequestIds(),
    m_localUidsOfElementsAlreadyAttemptedToFindByName(),
    m_notesToAddPerAPICallPostponeTimerId(),
    m_notesToUpdatePerAPICallPostponeTimerId(),
    m_afterUsnForSyncChunkPerAPICallPostponeTimerId(),
    m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId(),
    m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId(),
    m_getSyncStateBeforeStartAPICallPostponeTimerId(0),
    m_syncUserPostponeTimerId(0),
    m_syncAccountLimitsPostponeTimerId(0),
    m_gotLastSyncParameters(false)
{}

bool RemoteToLocalSynchronizationManager::active() const
{
    return m_active;
}

void RemoteToLocalSynchronizationManager::start(qint32 afterUsn)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::start: afterUsn = ") << afterUsn);

    m_lastUsnOnStart = afterUsn;

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused) {
        resume();   // NOTE: resume can call this method so it's necessary to return
        return;
    }

    if (!m_gotLastSyncParameters) {
        emit requestLastSyncParameters();
        return;
    }

    clear();
    m_active = true;

    // Checking the protocol version first
    if (!checkProtocolVersion()) {
        return;
    }

    // Retrieving the latest user info then, to figure out the service level and stuff like that
    if (!syncUser()) {
        return;
    }

    if (!checkAndSyncAccountLimits()) {
        return;
    }

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
            QNTRACE(QStringLiteral("The service has no updates for user's own account, need to check for updates from linked notebooks"));
            res = checkLinkedNotebooksSyncStates(asyncWait, error);
            if (asyncWait || error) {
                return;
            }

            if (!res) {
                QNTRACE(QStringLiteral("The service has no updates for any of linked notebooks"));
                finalize();
            }
        }
        // Otherwise the sync of all linked notebooks from user's account would start after the sync of user's account
        // (Because the sync of user's account can bring in the new linked notebooks or remove any of them)
    }

    downloadSyncChunksAndLaunchSync(afterUsn);
}

void RemoteToLocalSynchronizationManager::stop()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::stop"));
    m_requestedToStop = true;
    m_active = false;
    emit stopped();
}

void RemoteToLocalSynchronizationManager::pause()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::pause"));
    m_paused = true;
    m_active = false;
    emit paused(/* pending authentication = */ false);
}

void RemoteToLocalSynchronizationManager::resume()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::resume"));

    if (!m_connectedToLocalStorage) {
        createConnections();
    }

    if (m_paused)
    {
        m_active = true;
        m_paused = false;

        if (m_syncChunksDownloaded && (m_lastSyncChunksDownloadedUsn >= m_lastUsnOnStart))
        {
            QNDEBUG(QStringLiteral("last USN for which sync chunks were downloaded is ") << m_lastSyncChunksDownloadedUsn
                    << QStringLiteral(", there's no need to download the sync chunks again, launching the sync procedure"));

            if (m_fullNoteContentsDownloaded)
            {
                QNDEBUG(QStringLiteral("Full note's contents have already been downloaded meaning that the sync for "
                                       "tags, saved searches, notebooks and notes from user's account is over"));

                if (m_linkedNotebooksSyncChunksDownloaded) {
                    QNDEBUG(QStringLiteral("sync chunks for linked notebooks were already downloaded, there's no need to "
                                           "do it again, will launch the sync for linked notebooks"));
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
    QUuid addNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
    emit addNote(note, addNoteRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitAddRequest<Resource>(const Resource & resource)
{
    QString resourceGuid = (resource.hasGuid() ? resource.guid() : QString());
    QString resourceLocalUid = resource.localUid();
    QPair<QString,QString> key(resourceGuid, resourceLocalUid);

    QUuid addResourceRequestId = QUuid::createUuid();
    Q_UNUSED(m_addResourceRequestIds.insert(addResourceRequestId));
    emit addResource(resource, addResourceRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Tag>(const Tag & tag,
                                                                 const Tag * tagToAddLater)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::emitUpdateRequest<Tag>: tag = ")
            << tag << QStringLiteral(", tagToAddLater = ") << (tagToAddLater ? tagToAddLater->toString() : QStringLiteral("<null>")));

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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::emitUpdateRequest<SavedSearch>: search = ")
            << search << QStringLiteral(", searchToAddLater = ") << (searchToAddLater ? searchToAddLater->toString() : QStringLiteral("<null>")));

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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::emitUpdateRequest<LinkedNotebook>: linked notebook = ")
            << linkedNotebook);

    QUuid updateLinkedNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateLinkedNotebookRequestIds.insert(updateLinkedNotebookRequestId));
    emit updateLinkedNotebook(linkedNotebook, updateLinkedNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Notebook>(const Notebook & notebook,
                                                                      const Notebook * notebookToAddLater)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::emitUpdateRequest<Notebook>: notebook = ")
            << notebook << QStringLiteral(", notebook to add later = ") << (notebookToAddLater ? notebookToAddLater->toString() : QStringLiteral("<null>")));

    QUuid updateNotebookRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNotebookRequestIds.insert(updateNotebookRequestId));

    if (notebookToAddLater) {
        m_notebooksToAddPerRequestId[updateNotebookRequestId] = *notebookToAddLater;
    }

    emit updateNotebook(notebook, updateNotebookRequestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitUpdateRequest<Note>(const Note & note, const Note *)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::emitUpdateRequest<Note>: note = ") << note);

    const Notebook * pNotebook = getNotebookPerNote(note);
    if (!pNotebook) {
        QNLocalizedString errorDescription = QT_TR_NOOP("Detected attempt to update note in local storage before its notebook is found");
        QNWARNING(errorDescription << QStringLiteral(": ") << note);
        emit failure(errorDescription);
        return;
    }

    // Dealing with separate Evernote API call for getting the content of note to be updated
    Note localNote = note;
    QNLocalizedString errorDescription;

    qint32 postponeAPICallSeconds = tryToGetFullNoteData(localNote, errorDescription);
    if (postponeAPICallSeconds < 0)
    {
        return;
    }
    else if (postponeAPICallSeconds > 0)
    {
        int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
        if (Q_UNLIKELY(timerId == 0)) {
            QNLocalizedString errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call due to rate limit exceeding");
            emit failure(errorDescription);
            return;
        }

        m_notesToUpdatePerAPICallPostponeTimerId[timerId] = localNote;
        emit rateLimitExceeded(postponeAPICallSeconds);
        return;
    }

    QUuid updateNoteRequestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));

    localNote.setNotebookLocalUid(pNotebook->localUid());
    emit updateNote(localNote, /* update resources = */ true, /* update tags = */ true, updateNoteRequestId);
}

#define CHECK_PAUSED() \
    if (m_paused && !m_requestedToStop) { \
        QNDEBUG(QStringLiteral("RemoteTolocalSynchronizationManager is being paused, returning without any actions")); \
        return; \
    }

#define CHECK_STOPPED() \
    if (m_requestedToStop && !hasPendingRequests()) { \
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager is requested to stop and has no pending requests, " \
                               "finishing the synchronization")); \
        finalize(); \
        return; \
    }

void RemoteToLocalSynchronizationManager::onFindUserCompleted(User user, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_findUserRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindUserCompleted: user = ") << user
            << QStringLiteral("\nRequest id = ") << requestId);

    m_user = user;
    m_findUserRequestId = QUuid();

    CHECK_STOPPED();

    // Updating the user info as user was found in the local storage
    m_addOrUpdateUserRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to update user in the local storage database: request id = ")
            << m_addOrUpdateUserRequestId << QStringLiteral(", user = ") << m_user);
    emit updateUser(m_user, m_addOrUpdateUserRequestId);
}

void RemoteToLocalSynchronizationManager::onFindUserFailed(User user, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_findUserRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindUserFailed: user = ") << user
            << QStringLiteral("\nError description = ") << errorDescription << QStringLiteral(", request id = ")
            << requestId);

    m_findUserRequestId = QUuid();
    CHECK_STOPPED();

    // Adding the user info as user was not found in the local storage
    m_addOrUpdateUserRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting the request to add user to the local storage database: request id = ")
            << m_addOrUpdateUserRequestId << QStringLiteral(", user = ") << m_user);
    emit addUser(m_user, m_addOrUpdateUserRequestId);
}

void RemoteToLocalSynchronizationManager::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(notebook, requestId, QStringLiteral("Notebook"),
                                              m_notebooks, m_findNotebookByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(notebook, requestId, QStringLiteral("Notebook"),
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
        QString noteLocalUid = note.localUid();

        QPair<QString,QString> key(noteGuid, noteLocalUid);

        // NOTE: notebook for notes is only required for its pair of guid + local uid,
        // it shouldn't prohibit the creation or update of the notes during the synchronization procedure
        notebook.setCanCreateNotes(true);
        notebook.setCanUpdateNotes(true);

        m_notebooksPerNoteGuids[key] = notebook;

        Q_UNUSED(onFoundDuplicateByGuid(note, findNoteRequestId, QStringLiteral("Note"), m_notes, m_findNoteByGuidRequestIds));
        CHECK_STOPPED();
        return;
    }

    QSet<QUuid>::iterator nit = m_findNotebookForNotesWithConflictedResourcesRequestIds.find(requestId);
    if (nit != m_findNotebookForNotesWithConflictedResourcesRequestIds.end())
    {
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.erase(nit));

        CHECK_STOPPED();

        if (!notebook.hasGuid()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("found notebook in local storage which doesn't have guid set");
            QNWARNING(errorDescription << QStringLiteral(", notebook: ") << notebook);
            emit failure(errorDescription);
            return;
        }

        QHash<QString,QPair<Note,Note> >::iterator notesPerNotebookGuidIt = m_resourceConflictedAndRemoteNotesPerNotebookGuid.find(notebook.guid());
        if (notesPerNotebookGuidIt == m_resourceConflictedAndRemoteNotesPerNotebookGuid.end()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("unable to figure out for which notes the notebook was requested to be found");
            QNWARNING(errorDescription);
            emit failure(errorDescription);
            return;
        }

        const QPair<Note,Note> & notesPair = notesPerNotebookGuidIt.value();
        const Note & conflictedNote = notesPair.first;
        const Note & updatedNote = notesPair.second;

        QString conflictedNoteLocalUid = conflictedNote.localUid();
        QString updatedNoteLocalUid = updatedNote.localUid();

        if (!conflictedNoteLocalUid.isEmpty() || conflictedNote.hasGuid()) {
            QPair<QString,QString> key;
            key.first = (conflictedNote.hasGuid() ? conflictedNote.guid() : QString());
            key.second = conflictedNoteLocalUid;

            m_notebooksPerNoteGuids[key] = notebook;
        }

        if (!updatedNoteLocalUid.isEmpty() || updatedNote.hasGuid()) {
            QPair<QString,QString> key;
            key.first = (updatedNote.hasGuid() ? updatedNote.guid() : QString());
            key.second = updatedNoteLocalUid;

            m_notebooksPerNoteGuids[key] = notebook;
        }

        QUuid updateNoteRequestId = QUuid::createUuid();
        Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));
        emit updateNote(updatedNote, /* update resources = */ true, /* update tags = */ true, updateNoteRequestId);

        QUuid addNoteRequestId = QUuid::createUuid();
        Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
        emit addNote(conflictedNote, addNoteRequestId);

        return;
    }

    auto iit = m_inkNoteResourceDataPerFindNotebookRequestId.find(requestId);
    if (iit != m_inkNoteResourceDataPerFindNotebookRequestId.end())
    {
        InkNoteResourceData resourceData = iit.value();
        Q_UNUSED(m_inkNoteResourceDataPerFindNotebookRequestId.erase(iit))

        CHECK_STOPPED();

        setupInkNoteImageDownloading(resourceData.m_resourceGuid, resourceData.m_resourceHeight,
                                     resourceData.m_resourceWidth, notebook);
        return;
    }

    auto thumbnailIt = m_noteGuidForThumbnailDownloadByFindNotebookRequestId.find(requestId);
    if (thumbnailIt != m_noteGuidForThumbnailDownloadByFindNotebookRequestId.end())
    {
        QString noteGuid = thumbnailIt.value();
        Q_UNUSED(m_noteGuidForThumbnailDownloadByFindNotebookRequestId.erase(thumbnailIt))

        CHECK_STOPPED();

        setupNoteThumbnailDownloading(noteGuid, notebook);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription,
                                                               QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(notebook, requestId, errorDescription, QStringLiteral("Notebook"),
                                                  m_notebooks, m_findNotebookByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(notebook, requestId, errorDescription, QStringLiteral("Notebook"),
                                                  m_notebooks, m_findNotebookByNameRequestIds);
    if (failedToFindByName) {
        CHECK_STOPPED();
        return;
    }

    NoteDataPerFindNotebookRequestId::iterator rit = m_notesWithFindRequestIdsPerFindNotebookRequestId.find(requestId);
    if (rit != m_notesWithFindRequestIdsPerFindNotebookRequestId.end())
    {
        QNLocalizedString errorDescription = QT_TR_NOOP("failed to find notebook in the local storage for one of processed notes");
        QNWARNING(errorDescription << QStringLiteral(": ") << notebook);
        emit failure(errorDescription);
        return;
    }

    QSet<QUuid>::iterator nit = m_findNotebookForNotesWithConflictedResourcesRequestIds.find(requestId);
    if (nit != m_findNotebookForNotesWithConflictedResourcesRequestIds.end())
    {
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.erase(nit));

        CHECK_STOPPED();

        QNLocalizedString errorDescription = QT_TR_NOOP("Could not find notebook for update of note "
                                                        "which should resolve the conflict of individual resource");
        QNWARNING(errorDescription << QStringLiteral(", notebook attempted to be found: ") << notebook);
        emit failure(errorDescription);
        return;
    }

    auto iit = m_inkNoteResourceDataPerFindNotebookRequestId.find(requestId);
    if (iit != m_inkNoteResourceDataPerFindNotebookRequestId.end())
    {
        Q_UNUSED(m_inkNoteResourceDataPerFindNotebookRequestId.erase(iit))
        QNWARNING(QStringLiteral("Can't find notebook for the purpose of setting up the ink note image downloading"));
        CHECK_STOPPED();
        return;
    }

    auto thumbnailIt = m_noteGuidForThumbnailDownloadByFindNotebookRequestId.find(requestId);
    if (thumbnailIt != m_noteGuidForThumbnailDownloadByFindNotebookRequestId.end())
    {
        const QString & noteGuid = thumbnailIt.value();

        // We already have this note guid within those "pending the thumbnail download" for technical reasons, need to
        // remove it from there
        auto dit = m_noteGuidsPendingThumbnailDownload.find(noteGuid);
        if (dit != m_noteGuidsPendingThumbnailDownload.end()) {
            Q_UNUSED(m_noteGuidsPendingThumbnailDownload.erase(dit))
        }

        Q_UNUSED(m_noteGuidForThumbnailDownloadByFindNotebookRequestId.erase(thumbnailIt))

        QNWARNING(QStringLiteral("Can't find notebook for the purpose of setting up the note thumbnail downloading"));
        CHECK_STOPPED();
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
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindNoteCompleted: note = ")
                << note << QStringLiteral(", requestId = ") << requestId);

        // NOTE: erase is required for proper work of the macro; the request would be re-inserted below if macro doesn't return from the method
        Q_UNUSED(m_findNoteByGuidRequestIds.erase(it));

        CHECK_STOPPED();

        // Need to find Notebook corresponding to the note in order to proceed
        if (!note.hasNotebookGuid()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("found duplicate note in the local storage which doesn't have notebook guid set");
            QNWARNING(errorDescription << QStringLiteral(": ") << note);
            emit failure(errorDescription);
            return;
        }

        QUuid findNotebookPerNoteRequestId = QUuid::createUuid();
        m_notesWithFindRequestIdsPerFindNotebookRequestId[findNotebookPerNoteRequestId] =
                QPair<Note,QUuid>(note, requestId);

        Notebook notebookToFind;
        notebookToFind.unsetLocalUid();
        notebookToFind.setGuid(note.notebookGuid());

        Q_UNUSED(m_findNoteByGuidRequestIds.insert(requestId));

        emit findNotebook(notebookToFind, findNotebookPerNoteRequestId);
        return;
    }

    ResourceDataPerFindNoteRequestId::iterator rit = m_resourcesWithFindRequestIdsPerFindNoteRequestId.find(requestId);
    if (rit != m_resourcesWithFindRequestIdsPerFindNoteRequestId.end())
    {
        QPair<Resource,QUuid> resourceWithFindRequestId = rit.value();

        Q_UNUSED(m_resourcesWithFindRequestIdsPerFindNoteRequestId.erase(rit))

        CHECK_STOPPED();

        if (Q_UNLIKELY(!note.hasGuid())) {
            QNLocalizedString errorDescription = QT_TR_NOOP("found note by guid in the local storage but "
                                                            "the returned note object doesn't have guid set");
            QNWARNING(errorDescription << QStringLiteral(": ") << note);
            emit failure(errorDescription);
            return;
        }

        const Notebook * pNotebook = getNotebookPerNote(note);

        const Resource & resource = resourceWithFindRequestId.first;
        const QUuid & findResourceRequestId = resourceWithFindRequestId.second;

        auto noteThumbnailDownloadIt = m_noteGuidsPendingThumbnailDownload.find(note.guid());
        if (noteThumbnailDownloadIt == m_noteGuidsPendingThumbnailDownload.end())
        {
            QNDEBUG(QStringLiteral("Need to downloading the thumbnail for the note with added or updated resource"));
            if (pNotebook)
            {
                setupNoteThumbnailDownloading(note.guid(), *pNotebook);
            }
            else if (note.hasNotebookLocalUid() || note.hasNotebookGuid())
            {
                Notebook dummyNotebook;
                if (note.hasNotebookLocalUid()) {
                    dummyNotebook.setLocalUid(note.notebookLocalUid());
                }
                else {
                    dummyNotebook.setLocalUid(QString());
                    dummyNotebook.setGuid(note.notebookGuid());
                }

                QUuid findNotebookForNoteThumbnailDownloadRequestId = QUuid::createUuid();
                m_noteGuidForThumbnailDownloadByFindNotebookRequestId[findNotebookForNoteThumbnailDownloadRequestId] = note.guid();

                // NOTE: technically, here we don't start downloading the thumbnail yet; but it is necessary
                // to insert the guid into the set right here in order to prevent multiple thumbnail downloads
                // for the same note during the sync process
                Q_UNUSED(m_noteGuidsPendingThumbnailDownload.insert(note.guid()))

                QNTRACE(QStringLiteral("Emitting the request to find notebook for note thumbnail download setup: ")
                        << findNotebookForNoteThumbnailDownloadRequestId << QStringLiteral(", note guid = ") << note.guid()
                        << ", notebook: " << dummyNotebook);
                emit findNotebook(dummyNotebook, findNotebookForNoteThumbnailDownloadRequestId);
            }
            else
            {
                QNWARNING(QStringLiteral("Can't download the note thumbnail: note has neither notebook local uid "
                                         "nor notebook guid: ") << note);
            }
        }

        if (resource.hasGuid() && resource.hasMime() && resource.hasWidth() && resource.hasHeight() &&
            (resource.mime() == QStringLiteral("application/vnd.evernote.ink")))
        {
            QNDEBUG(QStringLiteral("The resource appears to be the one for the ink note, need to download the image for it; "
                                   "but first need to understand whether the note owning the resource is from the current "
                                   "user's account or from some linked notebook"));

            if (pNotebook)
            {
                setupInkNoteImageDownloading(resource.guid(), resource.height(), resource.width(), *pNotebook);
            }
            else if (note.hasNotebookLocalUid() || note.hasNotebookGuid())
            {
                InkNoteResourceData resourceData;
                resourceData.m_resourceGuid = resource.guid();
                resourceData.m_resourceHeight = resource.height();
                resourceData.m_resourceWidth = resource.width();

                Notebook dummyNotebook;
                if (note.hasNotebookLocalUid()) {
                    dummyNotebook.setLocalUid(note.notebookLocalUid());
                }
                else {
                    dummyNotebook.setLocalUid(QString());
                    dummyNotebook.setGuid(note.notebookGuid());
                }

                QUuid findNotebookForInkNoteSetupRequestId = QUuid::createUuid();
                m_inkNoteResourceDataPerFindNotebookRequestId[findNotebookForInkNoteSetupRequestId] = resourceData;
                QNTRACE(QStringLiteral("Emitting the request to find notebook for ink note image download resolution: ")
                        << findNotebookForInkNoteSetupRequestId << QStringLiteral(", resource guid = ") << resourceData.m_resourceGuid
                        << QStringLiteral(", resource height = ") << resourceData.m_resourceHeight << QStringLiteral(", resource width = ")
                        << resourceData.m_resourceWidth << QStringLiteral(", notebook: ") << dummyNotebook);
                emit findNotebook(dummyNotebook, findNotebookForInkNoteSetupRequestId);
            }
            else
            {
                QNWARNING(QStringLiteral("Can't download the ink note image: note has neither notebook local uid not notebook guid: ")
                          << note);
            }
        }

        auto resourceFoundIt = m_resourceFoundFlagPerFindResourceRequestId.find(findResourceRequestId);
        if (resourceFoundIt == m_resourceFoundFlagPerFindResourceRequestId.end()) {
            QNWARNING(QStringLiteral("Duplicate of synchronized resource was not found in the local storage database! "
                                     "Attempting to add it to local storage"));
            emitAddRequest(resource);
            return;
        }

        if (!resource.isDirty()) {
            QNDEBUG(QStringLiteral("Found duplicate resource in local storage which is not marked dirty => "
                                   "using the version from synchronization manager"));
            QUuid updateResourceRequestId = QUuid::createUuid();
            Q_UNUSED(m_updateResourceRequestIds.insert(updateResourceRequestId))
            emit updateResource(resource, updateResourceRequestId);
            return;
        }

        QNDEBUG(QStringLiteral("Found duplicate resource in local storage which is marked dirty => will copy the whole note "
                               "owning it to the conflict one"));

        Note conflictedNote(note);
        setConflicted(QStringLiteral("Note"), conflictedNote);
        QString conflictedNoteTitle = (conflictedNote.hasTitle()
                                       ? (tr("Note") + QStringLiteral(" \"") + conflictedNote.title() + QStringLiteral("\" ") +
                                          tr("containing conflicted resource"))
                                       : (tr("Note containing conflicted resource")));
        conflictedNoteTitle += QStringLiteral(" (") + QDateTime::currentDateTime().toString(Qt::ISODate) + QStringLiteral(")");
        conflictedNote.setTitle(conflictedNoteTitle);

        Note updatedNote(note);
        updatedNote.unsetLocalUid();
        bool hasResources = updatedNote.hasResources();
        QList<Resource> resources;
        if (hasResources) {
            resources = updatedNote.resources();
        }

        int numResources = resources.size();
        int resourceIndex = -1;
        for(int i = 0; i < numResources; ++i)
        {
            const Resource & existingResource = resources[i];
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
            QList<Resource> noteResources = updatedNote.resources();
            noteResources[resourceIndex] = resource;
            updatedNote.setResources(noteResources);
        }

        if (pNotebook)
        {
            updatedNote.setNotebookLocalUid(pNotebook->localUid());
            if (pNotebook->hasGuid()) {
                updatedNote.setNotebookGuid(pNotebook->guid());
            }

            QUuid updateNoteRequestId = QUuid::createUuid();
            Q_UNUSED(m_updateNoteRequestIds.insert(updateNoteRequestId));
            emit updateNote(updatedNote, /* update resources = */ true, /* update tags = */ true, updateNoteRequestId);

            conflictedNote.setNotebookLocalUid(pNotebook->localUid());
            if (pNotebook->hasGuid()) {
                conflictedNote.setNotebookGuid(pNotebook->guid());
            }

            QUuid addNoteRequestId = QUuid::createUuid();
            Q_UNUSED(m_addNoteRequestIds.insert(addNoteRequestId));
            emit addNote(conflictedNote, addNoteRequestId);

            return;
        }

        QNDEBUG(QStringLiteral("Notebook for note is not found yet, need to find it in order to resolve the notes "
                               "with conflicted resources"));

        if (!note.hasNotebookGuid()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("note containing the conflicted resource in the local storage "
                                                            "does not have notebook guid set");
            QNWARNING(errorDescription << QStringLiteral(": ") << note);
            emit failure(errorDescription);
            return;
        }

        m_resourceConflictedAndRemoteNotesPerNotebookGuid[note.notebookGuid()] = QPair<Note,Note>(conflictedNote, updatedNote);

        Notebook notebookToFind;
        notebookToFind.unsetLocalUid();
        notebookToFind.setGuid(note.notebookGuid());

        QUuid findNotebookForNotesWithConflictedResourcesRequestId = QUuid::createUuid();
        Q_UNUSED(m_findNotebookForNotesWithConflictedResourcesRequestIds.insert(findNotebookForNotesWithConflictedResourcesRequestId));
        emit findNotebook(notebookToFind, findNotebookForNotesWithConflictedResourcesRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindNoteFailed(Note note, bool withResourceBinaryData,
                                                           QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData);

    QSet<QUuid>::iterator it = m_findNoteByGuidRequestIds.find(requestId);
    if (it != m_findNoteByGuidRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindNoteFailed: note = ") << note
                << QStringLiteral(", requestId = ") << requestId);

        Q_UNUSED(m_findNoteByGuidRequestIds.erase(it));

        CHECK_STOPPED();

        auto it = findItemByGuid(m_notes, note, QStringLiteral("Note"));
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
            if (Q_UNLIKELY(timerId == 0)) {
                QNLocalizedString errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                                                "due to rate limit exceeding");
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

        QNLocalizedString errorDescription = QT_TR_NOOP("can't find note in local storage for individual "
                                                        "resource coming from the synchronization procedure");
        QNWARNING(errorDescription << QStringLiteral(", note attempted to be found: ") << note);
        emit failure(errorDescription);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindTagCompleted(Tag tag, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(tag, requestId, QStringLiteral("Tag"), m_tags, m_findTagByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(tag, requestId, QStringLiteral("Tag"), m_tags, m_findTagByNameRequestIds);
    if (foundByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(tag, requestId, errorDescription, QStringLiteral("Tag"),
                                                  m_tags, m_findTagByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(tag, requestId, errorDescription, QStringLiteral("Tag"),
                                                  m_tags, m_findTagByNameRequestIds);
    if (failedToFindByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindResourceCompleted(Resource resource,
                                                                  bool withResourceBinaryData, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData)

    QSet<QUuid>::iterator rit = m_findResourceByGuidRequestIds.find(requestId);
    if (rit != m_findResourceByGuidRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindResourceCompleted: resource = ")
                << resource << QStringLiteral(", requestId = ") << requestId);

        Q_UNUSED(m_findResourceByGuidRequestIds.erase(rit))

        CHECK_STOPPED();

        auto it = findItemByGuid(m_resources, resource, QStringLiteral("Resource"));
        if (it == m_resources.end()) {
            return;
        }

        // Removing the resource from the list of resources waiting for processing
        Q_UNUSED(m_resources.erase(it))

        // need to find the note owning the resource to proceed
        if (!resource.hasNoteGuid()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("found duplicate resource in local storage which doesn't have note guid set");
            QNWARNING(errorDescription << QStringLiteral(": ") << resource);
            emit failure(errorDescription);
            return;
        }

        Q_UNUSED(m_resourceFoundFlagPerFindResourceRequestId.insert(requestId))

        QUuid findNotePerResourceRequestId = QUuid::createUuid();
        m_resourcesWithFindRequestIdsPerFindNoteRequestId[findNotePerResourceRequestId] =
            QPair<Resource,QUuid>(resource, requestId);

        Note noteToFind;
        noteToFind.unsetLocalUid();
        noteToFind.setGuid(resource.noteGuid());

        emit findNote(noteToFind, /* with resource binary data = */ true, findNotePerResourceRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindResourceFailed(Resource resource,
                                                               bool withResourceBinaryData,
                                                               QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(withResourceBinaryData)

    QSet<QUuid>::iterator rit = m_findResourceByGuidRequestIds.find(requestId);
    if (rit != m_findResourceByGuidRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindResourceFailed: resource = ")
                << resource << QStringLiteral(", requestId = ") << requestId);

        Q_UNUSED(m_findResourceByGuidRequestIds.erase(rit))

        CHECK_STOPPED();

        auto it = findItemByGuid(m_resources, resource, QStringLiteral("Resource"));
        if (it == m_resources.end()) {
            return;
        }

        // Removing the resource from the list of resources waiting for processing
        Q_UNUSED(m_resources.erase(it))

        // need to find the note owning the resource to proceed
        if (!resource.hasNoteGuid()) {
            errorDescription = QT_TR_NOOP("detected resource which doesn't have note guid set");
            QNWARNING(errorDescription << QStringLiteral(": ") << resource);
            emit failure(errorDescription);
            return;
        }

        QUuid findNotePerResourceRequestId = QUuid::createUuid();
        m_resourcesWithFindRequestIdsPerFindNoteRequestId[findNotePerResourceRequestId] =
            QPair<Resource,QUuid>(resource, requestId);

        Note noteToFind;
        noteToFind.unsetLocalUid();
        noteToFind.setGuid(resource.noteGuid());

        emit findNote(noteToFind, /* with resource binary data = */ false, findNotePerResourceRequestId);
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook,
                                                                        QUuid requestId)
{
    CHECK_PAUSED();

    Q_UNUSED(onFoundDuplicateByGuid(linkedNotebook, requestId, QStringLiteral("LinkedNotebook"),
                                    m_linkedNotebooks, m_findLinkedNotebookRequestIds));

    CHECK_STOPPED();
}

void RemoteToLocalSynchronizationManager::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                     QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    QSet<QUuid>::iterator rit = m_findLinkedNotebookRequestIds.find(requestId);
    if (rit == m_findLinkedNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFindLinkedNotebookFailed: ")
            << linkedNotebook << QStringLiteral(", errorDescription = ") << errorDescription
            << QStringLiteral(", requestId = ") << requestId);

    Q_UNUSED(m_findLinkedNotebookRequestIds.erase(rit));

    CHECK_STOPPED();

    LinkedNotebooksList::iterator it = findItemByGuid(m_linkedNotebooks, linkedNotebook, QStringLiteral("LinkedNotebook"));
    if (it == m_linkedNotebooks.end()) {
        return;
    }

    // This linked notebook was not found in the local storage by guid, adding it there
    emitAddRequest(linkedNotebook);
}

void RemoteToLocalSynchronizationManager::onFindSavedSearchCompleted(SavedSearch savedSearch, QUuid requestId)
{
    CHECK_PAUSED();

    bool foundByGuid = onFoundDuplicateByGuid(savedSearch, requestId, QStringLiteral("SavedSearch"),
                                              m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (foundByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool foundByName = onFoundDuplicateByName(savedSearch, requestId, QStringLiteral("SavedSearch"),
                                              m_savedSearches, m_findSavedSearchByNameRequestIds);
    if (foundByName) {
        CHECK_STOPPED();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onFindSavedSearchFailed(SavedSearch savedSearch,
                                                                  QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    bool failedToFindByGuid = onNoDuplicateByGuid(savedSearch, requestId, errorDescription, QStringLiteral("SavedSearch"),
                                                  m_savedSearches, m_findSavedSearchByGuidRequestIds);
    if (failedToFindByGuid) {
        CHECK_STOPPED();
        return;
    }

    bool failedToFindByName = onNoDuplicateByName(savedSearch, requestId, errorDescription, QStringLiteral("SavedSearch"),
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
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onAddDataElementCompleted<") << typeName
                << QStringLiteral(">: ") << typeName << QStringLiteral(" = ") << element << QStringLiteral(", requestId = ")
                << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        CHECK_PAUSED();
        CHECK_STOPPED();

        performPostAddOrUpdateChecks<ElementType>();

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onAddUserCompleted(User user, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_addOrUpdateUserRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onAddUserCompleted: user = ") << user
            << QStringLiteral("\nRequest id = ") << requestId);

    m_addOrUpdateUserRequestId = QUuid();
    CHECK_STOPPED();
}

void RemoteToLocalSynchronizationManager::onAddUserFailed(User user, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_addOrUpdateUserRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onAddUserFailed: ") << user
            << QStringLiteral("\nRequest id = ") << requestId);

    QNLocalizedString error = QT_TR_NOOP("Can't add user data fetched from the remote database to the local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);

    m_addOrUpdateUserRequestId = QUuid();
    CHECK_STOPPED();
}

void RemoteToLocalSynchronizationManager::onAddTagCompleted(Tag tag, QUuid requestId)
{
    onAddDataElementCompleted(tag, requestId, QStringLiteral("Tag"), m_addTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onAddDataElementCompleted(search, requestId, QStringLiteral("SavedSearch"), m_addSavedSearchRequestIds);
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onAddDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                 const QNLocalizedString & errorDescription, const QString & typeName,
                                                                 QSet<QUuid> & addElementRequestIds)
{
    QSet<QUuid>::iterator it = addElementRequestIds.find(requestId);
    if (it != addElementRequestIds.end())
    {
        QNWARNING(QStringLiteral("FullSyncronizationManager::onAddDataElementFailed<") << typeName
                  << QStringLiteral(">: ") << typeName << QStringLiteral(" = ") << element << QStringLiteral(", error description = ")
                  << errorDescription << QStringLiteral(", requestId = ") << requestId);

        Q_UNUSED(addElementRequestIds.erase(it));

        QNLocalizedString error = QT_TR_NOOP("Can't add data item fetched from the remote database to the local storage");
        error += QStringLiteral(": ");
        error += QT_TR_NOOP("item type");
        error += QStringLiteral(": ");
        error += typeName;
        error += QStringLiteral("; ");
        error += QT_TR_NOOP("error");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onAddTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(tag, requestId, errorDescription, QStringLiteral("Tag"), m_addTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(search, requestId, errorDescription, QStringLiteral("SavedSearch"), m_addSavedSearchRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateUserCompleted(User user, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_addOrUpdateUserRequestId)
    {
        if (user.hasId() && (user.id() == m_userId)) {
            QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateUserCompleted: external update of current user, request id = ")
                    << requestId);
            m_user = user;
            CHECK_STOPPED();
        }

        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateUserCompleted: user = ") << user
            << QStringLiteral("\nRequest id = ") << requestId);

    m_addOrUpdateUserRequestId = QUuid();
    CHECK_STOPPED();
}

void RemoteToLocalSynchronizationManager::onUpdateUserFailed(User user, QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();

    if (requestId != m_addOrUpdateUserRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateUserFailed: user = ") << user
            << QStringLiteral("\nError description = ") << errorDescription << QStringLiteral(", request id = ")
            << requestId);

    QNLocalizedString error = QT_TR_NOOP("Can't update user data fetched from the remote database to the local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);

    m_addOrUpdateUserRequestId = QUuid();
    CHECK_STOPPED();
}

template <class ElementType, class ElementsToAddByUuid>
void RemoteToLocalSynchronizationManager::onUpdateDataElementCompleted(const ElementType & element,
                                                                       const QUuid & requestId, const QString & typeName,
                                                                       QSet<QUuid> & updateElementRequestIds,
                                                                       ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator rit = updateElementRequestIds.find(requestId);
    if (rit == updateElementRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizartionManager::onUpdateDataElementCompleted<") << typeName
            << QStringLiteral(">: ") << typeName << QStringLiteral(" = ") << element << QStringLiteral(", requestId = ") << requestId);

    Q_UNUSED(updateElementRequestIds.erase(rit));

    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end())
    {
        ElementType & elementToAdd = addIt.value();
        const QString localUid = elementToAdd.localUid();
        if (localUid.isEmpty()) {
            QNLocalizedString errorDescription = QT_TR_NOOP("detected item with empty local uid in the local storage");
            errorDescription += QStringLiteral(", ");
            errorDescription += QT_TR_NOOP("item type");
            errorDescription += QStringLiteral(": ");
            errorDescription += typeName;
            QNWARNING(errorDescription << QStringLiteral(": ") << elementToAdd);
            emit failure(errorDescription);
            return;
        }

        QSet<QString>::iterator git = m_localUidsOfElementsAlreadyAttemptedToFindByName.find(localUid);
        if (git == m_localUidsOfElementsAlreadyAttemptedToFindByName.end())
        {
            QNDEBUG(QStringLiteral("Checking whether ") << typeName << QStringLiteral(" with the same name already exists in local storage"));
            Q_UNUSED(m_localUidsOfElementsAlreadyAttemptedToFindByName.insert(localUid));

            elementToAdd.unsetLocalUid();
            elementToAdd.setGuid(QStringLiteral(""));
            emitFindByNameRequest(elementToAdd);
        }
        else
        {
            QNDEBUG(QStringLiteral("Adding ") << typeName << QStringLiteral(" to local storage"));
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
    onUpdateDataElementCompleted(tag, requestId, QStringLiteral("Tag"), m_updateTagRequestIds, m_tagsToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onUpdateDataElementCompleted(search, requestId, QStringLiteral("SavedSearch"), m_updateSavedSearchRequestIds,
                                 m_savedSearchesToAddPerRequestId);
}

template <class ElementType, class ElementsToAddByUuid>
void RemoteToLocalSynchronizationManager::onUpdateDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                    const QNLocalizedString & errorDescription, const QString & typeName,
                                                                    QSet<QUuid> & updateElementRequestIds,
                                                                    ElementsToAddByUuid & elementsToAddByRenameRequestId)
{
    QSet<QUuid>::iterator it = updateElementRequestIds.find(requestId);
    if (it == updateElementRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateDataElementFailed<") << typeName
              << QStringLiteral(">: ") << typeName << QStringLiteral(" = ") << element << QStringLiteral(", errorDescription = ")
              << errorDescription << QStringLiteral(", requestId = ") << requestId);

    Q_UNUSED(updateElementRequestIds.erase(it));

    QNLocalizedString error;
    typename ElementsToAddByUuid::iterator addIt = elementsToAddByRenameRequestId.find(requestId);
    if (addIt != elementsToAddByRenameRequestId.end()) {
        Q_UNUSED(elementsToAddByRenameRequestId.erase(addIt));
        error = QT_TR_NOOP("can't rename local dirty duplicate item in the local storage");
    }
    else {
        error = QT_TR_NOOP("can't update remote item in the local storage");
    }

    error += QStringLiteral(", ");
    error += QT_TR_NOOP("item type");
    error += QStringLiteral(": ");
    error += typeName;
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);
}

void RemoteToLocalSynchronizationManager::onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    onUpdateDataElementFailed(tag, requestId, errorDescription, QStringLiteral("Tag"), m_updateTagRequestIds,
                              m_tagsToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeTagCompleted(Tag tag, QUuid requestId)
{
    onExpungeDataElementCompleted(tag, requestId, QStringLiteral("Tag"), m_expungeTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(tag, requestId, errorDescription, QStringLiteral("Tag"), m_expungeTagRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId)
{
    if (requestId == m_expungeNotelessTagsRequestId) {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksCompleted"));
        m_expungeNotelessTagsRequestId = QUuid();
        finalize();
        return;
    }
}

void RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksFailed(QNLocalizedString errorDescription, QUuid requestId)
{
    if (requestId != m_expungeNotelessTagsRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onExpungeNotelessTagsFromLinkedNotebooksFailed: ")
            << errorDescription);
    m_expungeNotelessTagsRequestId = QUuid();

    QNLocalizedString error = QT_TR_NOOP("can't expunge noteless tags belonging to linked notebooks from local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);
}

void RemoteToLocalSynchronizationManager::onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription,
                                                                    QUuid requestId)
{
    onUpdateDataElementFailed(search, requestId, errorDescription, QStringLiteral("SavedSearch"), m_updateSavedSearchRequestIds,
                              m_savedSearchesToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeSavedSearchCompleted(SavedSearch search, QUuid requestId)
{
    onExpungeDataElementCompleted(search, requestId, QStringLiteral("SavedSearch"), m_expungeSavedSearchRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(search, requestId, errorDescription, QStringLiteral("SavedSearch"), m_expungeSavedSearchRequestIds);
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
void RemoteToLocalSynchronizationManager::performPostAddOrUpdateChecks<Resource>()
{
    if (m_addResourceRequestIds.empty() && m_updateResourceRequestIds.empty() &&
        m_resourcesWithFindRequestIdsPerFindNoteRequestId.empty() &&
        m_findNotebookForNotesWithConflictedResourcesRequestIds.empty() &&
        m_inkNoteResourceDataPerFindNotebookRequestId.empty() &&
        (m_numPendingInkNoteImageDownloads == 0) &&
        m_noteGuidForThumbnailDownloadByFindNotebookRequestId.empty() &&
        m_noteGuidsPendingThumbnailDownload.empty())
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
void RemoteToLocalSynchronizationManager::unsetLocalUid(ElementType & element)
{
    element.unsetLocalUid();
}

template <>
void RemoteToLocalSynchronizationManager::unsetLocalUid<LinkedNotebook>(LinkedNotebook &)
{
    // do nothing, local uid doesn't make any sense to linked notebook
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onExpungeDataElementCompleted(const ElementType & element, const QUuid & requestId,
                                                                        const QString & typeName, QSet<QUuid> & expungeElementRequestIds)
{
    auto it = expungeElementRequestIds.find(requestId);
    if (it == expungeElementRequestIds.end()) {
        return;
    }

    QNTRACE(QStringLiteral("Expunged ") << typeName << QStringLiteral(" from local storage: ") << element);
    Q_UNUSED(expungeElementRequestIds.erase(it));

    performPostExpungeChecks<ElementType>();

    checkExpungesCompletion();
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::onExpungeDataElementFailed(const ElementType & element, const QUuid & requestId,
                                                                     const QNLocalizedString & errorDescription, const QString & typeName,
                                                                     QSet<QUuid> & expungeElementRequestIds)
{
    Q_UNUSED(element);

    auto it = expungeElementRequestIds.find(requestId);
    if (it == expungeElementRequestIds.end()) {
        return;
    }

    Q_UNUSED(expungeElementRequestIds.erase(it));

    QNLocalizedString error = QT_TR_NOOP("can't expunge item from local storage");
    error += QStringLiteral(": ");
    error += QT_TR_NOOP("item type");
    error += QStringLiteral(": ");
    error += typeName;
    error += QStringLiteral(": ");
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
    tagToExpunge.unsetLocalUid();

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
    searchToExpunge.unsetLocalUid();

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
    notebookToExpunge.unsetLocalUid();

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
    noteToExpunge.unsetLocalUid();

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
    if (!m_expungeNoteRequestIds.isEmpty()) {
        return;
    }

    if (!m_expungedNotebooks.isEmpty()) {
        expungeNotebooks();
        return;
    }

    expungeSavedSearches();
    expungeTags();
    expungeLinkedNotebooks();
}

template <>
void RemoteToLocalSynchronizationManager::performPostExpungeChecks<Notebook>()
{
    if (!m_expungeNotebookRequestIds.isEmpty()) {
        return;
    }

    expungeSavedSearches();
    expungeTags();
    expungeLinkedNotebooks();
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
    tag.unsetLocalUid();
    tag.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagByGuidRequestIds.insert(requestId));
    emit findTag(tag, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<SavedSearch>(const QString & guid)
{
    SavedSearch search;
    search.unsetLocalUid();
    search.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findSavedSearchByGuidRequestIds.insert(requestId));
    emit findSavedSearch(search, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<Notebook>(const QString & guid)
{
    Notebook notebook;
    notebook.unsetLocalUid();
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
    note.unsetLocalUid();
    note.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteByGuidRequestIds.insert(requestId));
    bool withResourceDataOption = false;
    emit findNote(note, withResourceDataOption, requestId);
}

template <>
void RemoteToLocalSynchronizationManager::emitFindByGuidRequest<Resource>(const QString & guid)
{
    Resource resource;
    resource.unsetLocalUid();
    resource.setGuid(guid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findResourceByGuidRequestIds.insert(requestId));
    bool withBinaryData = false;
    emit findResource(resource, withBinaryData, requestId);
}

void RemoteToLocalSynchronizationManager::onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    handleLinkedNotebookAdded(linkedNotebook);
    onAddDataElementCompleted(linkedNotebook, requestId, QStringLiteral("LinkedNotebook"), m_addLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                    QNLocalizedString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(linkedNotebook, requestId, errorDescription,
                           QStringLiteral("LinkedNotebook"), m_addLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook,
                                                                          QUuid requestId)
{
    handleLinkedNotebookUpdated(linkedNotebook);

    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookCompleted: linkedNotebook = ")
                << linkedNotebook << QStringLiteral(", requestId = ") << requestId);

        Q_UNUSED(m_updateLinkedNotebookRequestIds.erase(it));

        CHECK_PAUSED();
        CHECK_STOPPED();

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook,
                                                                       QNLocalizedString errorDescription, QUuid requestId)
{
    QSet<QUuid>::iterator it = m_updateLinkedNotebookRequestIds.find(requestId);
    if (it != m_updateLinkedNotebookRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateLinkedNotebookFailed: linkedNotebook = ")
                << linkedNotebook << QStringLiteral(", errorDescription = ") << errorDescription
                << QStringLiteral(", requestId = ") << requestId);

        QNLocalizedString error = QT_TR_NOOP("can't update linked notebook in the local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onExpungeLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId)
{
    onExpungeDataElementCompleted(linkedNotebook, requestId, QStringLiteral("Linked notebook"), m_expungeLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeLinkedNotebookFailed(LinkedNotebook linkedNotebook, QNLocalizedString errorDescription,
                                                                        QUuid requestId)
{
    onExpungeDataElementFailed(linkedNotebook, requestId, errorDescription, QStringLiteral("Linked notebook"),
                               m_expungeLinkedNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onListAllLinkedNotebooksCompleted(size_t limit, size_t offset,
                                                                            LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                            LocalStorageManager::OrderDirection::type orderDirection,
                                                                            QList<LinkedNotebook> linkedNotebooks, QUuid requestId)
{
    CHECK_PAUSED();
    CHECK_STOPPED();

    if (requestId != m_listAllLinkedNotebooksRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onListAllLinkedNotebooksCompleted: limit = ") << limit
            << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order
            << QStringLiteral(", order direction = ") << orderDirection << QStringLiteral(", requestId = ") << requestId);

    m_allLinkedNotebooks = linkedNotebooks;
    m_allLinkedNotebooksListed = true;
    startLinkedNotebooksSync();
}

void RemoteToLocalSynchronizationManager::onListAllLinkedNotebooksFailed(size_t limit, size_t offset,
                                                                         LocalStorageManager::ListLinkedNotebooksOrder::type order,
                                                                         LocalStorageManager::OrderDirection::type orderDirection,
                                                                         QNLocalizedString errorDescription, QUuid requestId)
{
    CHECK_PAUSED();
    CHECK_STOPPED();

    if (requestId != m_listAllLinkedNotebooksRequestId) {
        return;
    }

    QNWARNING(QStringLiteral("RemoteToLocalSynchronizationManager::onListAllLinkedNotebooksFailed: limit = ") << limit
              << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", order direction = ")
              << orderDirection << QStringLiteral(", error description = ") << errorDescription << QStringLiteral("; request id = ")
              << requestId);

    m_allLinkedNotebooksListed = false;

    QNLocalizedString error = QT_TR_NOOP("can't list all linked notebooks from local storage");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit failure(error);
}

void RemoteToLocalSynchronizationManager::onAddNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onAddDataElementCompleted(notebook, requestId, QStringLiteral("Notebook"), m_addNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNotebookFailed(Notebook notebook, QNLocalizedString errorDescription,
                                                              QUuid requestId)
{
    onAddDataElementFailed(notebook, requestId, errorDescription, QStringLiteral("Notebook"), m_addNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onUpdateDataElementCompleted(notebook, requestId, QStringLiteral("Notebook"), m_updateNotebookRequestIds,
                                 m_notebooksToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription,
                                                                 QUuid requestId)
{
    onUpdateDataElementFailed(notebook, requestId, errorDescription, QStringLiteral("Notebook"),
                              m_updateNotebookRequestIds, m_notebooksToAddPerRequestId);
}

void RemoteToLocalSynchronizationManager::onExpungeNotebookCompleted(Notebook notebook, QUuid requestId)
{
    onExpungeDataElementCompleted(notebook, requestId, QStringLiteral("Notebook"), m_expungeNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(notebook, requestId, errorDescription, QStringLiteral("Notebook"), m_expungeNotebookRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNoteCompleted(Note note, QUuid requestId)
{
    onAddDataElementCompleted(note, requestId, QStringLiteral("Note"), m_addNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddNoteFailed(Note note, QNLocalizedString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(note, requestId, errorDescription, QStringLiteral("Note"), m_addNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateNoteCompleted(Note note, bool updateResources,
                                                                bool updateTags, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateNoteCompleted: note = ") << note
                << QStringLiteral("\nrequestId = ") << requestId);

        Q_UNUSED(m_updateNoteRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                                             QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    QSet<QUuid>::iterator it = m_updateNoteRequestIds.find(requestId);
    if (it != m_updateNoteRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateNoteFailed: note = ") << note
                << QStringLiteral("\nerrorDescription = ") << errorDescription << QStringLiteral("\nrequestId = ")
                << requestId);

        QNLocalizedString error = QT_TR_NOOP("can't update note in local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onExpungeNoteCompleted(Note note, QUuid requestId)
{
    onExpungeDataElementCompleted(note, requestId, QStringLiteral("Note"), m_expungeNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onExpungeNoteFailed(Note note, QNLocalizedString errorDescription, QUuid requestId)
{
    onExpungeDataElementFailed(note, requestId, errorDescription, QStringLiteral("Note"), m_expungeNoteRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddResourceCompleted(Resource resource, QUuid requestId)
{
    onAddDataElementCompleted(resource, requestId, QStringLiteral("Resource"), m_addResourceRequestIds);
}

void RemoteToLocalSynchronizationManager::onAddResourceFailed(Resource resource, QNLocalizedString errorDescription, QUuid requestId)
{
    onAddDataElementFailed(resource, requestId, errorDescription, QStringLiteral("Resource"), m_addResourceRequestIds);
}

void RemoteToLocalSynchronizationManager::onUpdateResourceCompleted(Resource resource, QUuid requestId)
{
    auto it = m_updateResourceRequestIds.find(requestId);
    if (it != m_updateResourceRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateResourceCompleted: resource = ")
                << resource << QStringLiteral("\nrequestId = ") << requestId);

        Q_UNUSED(m_updateResourceRequestIds.erase(it));

        checkServerDataMergeCompletion();
    }
}

void RemoteToLocalSynchronizationManager::onUpdateResourceFailed(Resource resource, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_updateResourceRequestIds.find(requestId);
    if (it != m_updateResourceRequestIds.end())
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onUpdateResourceFailed: resource = ")
                << resource << QStringLiteral("\nrequestId = ") << requestId);

        QNLocalizedString error = QT_TR_NOOP("can't update resource in the local storage");
        error += QStringLiteral(": ");
        error += errorDescription;
        emit failure(error);
    }
}

void RemoteToLocalSynchronizationManager::onInkNoteImageDownloadFinished(bool status, QString inkNoteImageFilePath,
                                                                         QNLocalizedString errorDescription)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onInkNoteImageDownloadFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", ink note image file path = ") << inkNoteImageFilePath
            << QStringLiteral(", error description = ") << errorDescription);

    --m_numPendingInkNoteImageDownloads;
    m_numPendingInkNoteImageDownloads = std::max(m_numPendingInkNoteImageDownloads, qint64(0));

    checkServerDataMergeCompletion();

    if (status) {
        return;
    }

    QNWARNING(errorDescription);
}

void RemoteToLocalSynchronizationManager::onNoteThumbnailDownloadingFinished(bool status, QString noteGuid,
                                                                             QString downloadedThumbnailImageFilePath,
                                                                             QNLocalizedString errorDescription)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onNoteThumbnailDownloadingFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", note guid = ") << noteGuid << QStringLiteral(", downloaded thumbnail image file path = ")
            << downloadedThumbnailImageFilePath << QStringLiteral(", error description = ") << errorDescription);

    auto it = m_noteGuidsPendingThumbnailDownload.find(noteGuid);
    if (Q_LIKELY(it != m_noteGuidsPendingThumbnailDownload.end())) {
        Q_UNUSED(m_noteGuidsPendingThumbnailDownload.erase(it))
    }
    else {
        QNWARNING(QStringLiteral("Received note thumbnail downloaded event for note which seemingly was not pending it: note guid = ")
                  << noteGuid);
    }

    checkServerDataMergeCompletion();

    if (status) {
        return;
    }

    QNWARNING(errorDescription);
}

void RemoteToLocalSynchronizationManager::onAuthenticationInfoReceived(QString authToken, qint32 userId, QString shardId,
                                                                       qevercloud::Timestamp expirationTime)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onAuthenticationInfoReceived: expiration time = ")
            << printableDateTimeFromTimestamp(expirationTime) << QStringLiteral(", user id = ") << userId);

    bool wasPending = m_pendingAuthenticationTokenAndShardId;

    // NOTE: we only need this authentication information to download the thumbnails and ink note images
    m_authenticationToken = authToken;
    m_userId = userId;
    m_shardId = shardId;
    m_authenticationTokenExpirationTime = expirationTime;
    m_pendingAuthenticationTokenAndShardId = false;

    if (!wasPending) {
        return;
    }

    if (m_paused) {
        resume();
    }
    else {
        launchSync();
    }
}

void RemoteToLocalSynchronizationManager::onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString, QPair<QString,QString> > authenticationTokensAndShardIdsByLinkedNotebookGuid,
                                                                                           QHash<QString, qevercloud::Timestamp> authenticationTokenExpirationTimesByLinkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onAuthenticationTokensForLinkedNotebooksReceived"));

    bool wasPending = m_pendingAuthenticationTokensForLinkedNotebooks;

    m_authenticationTokensAndShardIdsByLinkedNotebookGuid = authenticationTokensAndShardIdsByLinkedNotebookGuid;
    m_authenticationTokenExpirationTimesByLinkedNotebookGuid = authenticationTokenExpirationTimesByLinkedNotebookGuid;
    m_pendingAuthenticationTokensForLinkedNotebooks = false;

    if (!wasPending) {
        QNDEBUG(QStringLiteral("Authentication tokens for linked notebooks were not requested"));
        return;
    }

    startLinkedNotebooksSync();
}

void RemoteToLocalSynchronizationManager::onLastSyncParametersReceived(qint32 lastUpdateCount, qevercloud::Timestamp lastSyncTime,
                                                                       QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid,
                                                                       QHash<QString,qevercloud::Timestamp> lastSyncTimeByLinkedNotebookGuid)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onLastSyncParametersReceived: last update count = ")
            << lastUpdateCount << QStringLiteral(", last sync time = ") << lastSyncTime
            << QStringLiteral(", last update counts per linked notebook = ") << lastUpdateCountByLinkedNotebookGuid
            << QStringLiteral("last sync time per linked notebook = ") << lastSyncTimeByLinkedNotebookGuid);

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
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addUser,User,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateUser,User,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateUserRequest,User,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findUser,User,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindUserRequest,User,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addNotebook,Notebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateNotebook,Notebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findNotebook,Notebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeNotebook,Notebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotebookRequest,Notebook,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addNote,Note,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateNote,Note,bool,bool,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findNote,Note,bool,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeNote,Note,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNoteRequest,Note,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addTag,Tag,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateTag,Tag,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findTag,Tag,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeTag,Tag,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeTagRequest,Tag,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeNotelessTagsFromLinkedNotebooks,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotelessTagsFromLinkedNotebooksRequest,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addResource,Resource,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddResourceRequest,Resource,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateResource,Resource,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateResourceRequest,Resource,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findResource,Resource,bool,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindResourceRequest,Resource,bool,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addLinkedNotebook,LinkedNotebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateLinkedNotebook,LinkedNotebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findLinkedNotebook,LinkedNotebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeLinkedNotebook,LinkedNotebook,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeLinkedNotebookRequest,LinkedNotebook,QUuid));

    QObject::connect(this,
                     QNSIGNAL(RemoteToLocalSynchronizationManager,listAllLinkedNotebooks,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                     &m_localStorageManagerThreadWorker,
                     QNSLOT(LocalStorageManagerThreadWorker,onListAllLinkedNotebooksRequest,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QUuid));

    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addSavedSearch,SavedSearch,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateSavedSearch,SavedSearch,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findSavedSearch,SavedSearch,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindSavedSearchRequest,SavedSearch,QUuid));
    QObject::connect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeSavedSearch,SavedSearch,QUuid),
                     &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeSavedSearch,SavedSearch,QUuid));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserComplete,User,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindUserCompleted,User,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserFailed,User,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindUserFailed,User,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNoteCompleted,Note,bool,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNoteFailed,Note,bool,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findResourceComplete,Resource,bool,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindResourceCompleted,Resource,bool,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findResourceFailed,Resource,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onFindResourceFailed,Resource,bool,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotelessTagsFromLinkedNotebooksCompleted,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksFailed,QNLocalizedString,QUUid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotelessTagsFromLinkedNotebooksFailed,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserComplete,User,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddUserCompleted,User,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserFailed,User,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddUserFailed,User,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserComplete,User,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateUserCompleted,User,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserFailed,User,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateUserFailed,User,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeSavedSearchCompleted,SavedSearch,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeLinkedNotebookComplete,LinkedNotebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllLinkedNotebooksComplete,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QUuid),
                     this,
                     QNSLOT(RemoteToLocalSynchronizationManager,onListAllLinkedNotebooksCompleted,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker,
                     QNSIGNAL(LocalStorageManagerThreadWorker,listAllLinkedNotebooksFailed,size_t,size_t,
                              LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                     this,
                     QNSLOT(RemoteToLocalSynchronizationManager,onListAllLinkedNotebooksFailed,size_t,size_t,
                            LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNoteCompleted,Note,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNoteFailed,Note,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNoteCompleted,Note,bool,bool,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNoteCompleted,Note,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNoteFailed,Note,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addResourceComplete,Resource,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddResourceCompleted,Resource,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addResourceFailed,Resource,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onAddResourceFailed,Resource,QNLocalizedString,QUuid));

    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateResourceComplete,Resource,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateResourceCompleted,Resource,QUuid));
    QObject::connect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateResourceFailed,Resource,QNLocalizedString,QUuid),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateResourceFailed,Resource,QNLocalizedString,QUuid));

    m_connectedToLocalStorage = true;
}

void RemoteToLocalSynchronizationManager::disconnectFromLocalStorage()
{
    // Disconnect local signals from localStorageManagerThread's slots
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addUser,User,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddUserRequest,User,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateUser,User,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateUserRequest,User,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findUser,User,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindUserRequest,User,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addNotebook,Notebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateNotebook,Notebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findNotebook,Notebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeNotebook,Notebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotebookRequest,Notebook,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addNote,Note,Notebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNoteRequest,Note,Notebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateNote,Note,Notebook,bool,bool,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,Notebook,bool,bool,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findNote,Note,bool,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeNote,Note,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNoteRequest,Note,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addTag,Tag,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateTag,Tag,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateTagRequest,Tag,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findTag,Tag,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeTag,Tag,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeTagRequest,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotelessTagsFromLinkedNotebooksCompleted,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksFailed,QNLocalizedString,QUUid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotelessTagsFromLinkedNotebooksFailed,QNLocalizedString,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addResource,Resource,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddResourceRequest,Resource,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateResource,Resource,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateResourceRequest,Resource,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findResource,Resource,bool,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindResourceRequest,Resource,bool,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addLinkedNotebook,LinkedNotebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateLinkedNotebook,LinkedNotebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findLinkedNotebook,LinkedNotebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindLinkedNotebookRequest,LinkedNotebook,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeLinkedNotebook,LinkedNotebook,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeLinkedNotebookRequest,LinkedNotebook,QUuid));

    QObject::disconnect(this,
                        QNSIGNAL(RemoteToLocalSynchronizationManager,listAllLinkedNotebooks,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QUuid),
                        &m_localStorageManagerThreadWorker,
                        QNSLOT(LocalStorageManagerThreadWorker,onListAllLinkedNotebooksRequest,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QUuid));

    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,addSavedSearch,SavedSearch,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddSavedSearchRequest,SavedSearch,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,updateSavedSearch,SavedSearch,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateSavedSearchRequest,SavedSearch,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,findSavedSearch,SavedSearch,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindSavedSearchRequest,SavedSearch,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoteToLocalSynchronizationManager,expungeSavedSearch,SavedSearch,QUuid),
                        &m_localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeSavedSearch,SavedSearch,QUuid));

    // Disconnect localStorageManagerThread's signals to local slots
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserComplete,User,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindUserCompleted,User,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findUserFailed,User,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindUserFailed,User,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNoteCompleted,Note,bool,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindNoteFailed,Note,bool,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindTagCompleted,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLinkedNotebookComplete,LinkedNotebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchComplete,SavedSearch,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindSavedSearchCompleted,SavedSearch,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findResourceComplete,Resource,bool,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindResourceCompleted,Resource,bool,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findResourceFailed,Resource,bool,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onFindResourceFailed,Resource,bool,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddTagCompleted,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateTagCompleted,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagFailed,Tag,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeTagCompleted,Tag,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagFailed,Tag,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeTagFailed,Tag,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchComplete,SavedSearch,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddSavedSearchCompleted,SavedSearch,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchComplete,SavedSearch,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateSavedSearchCompleted,SavedSearch,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchComplete,SavedSearch,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeSavedSearchCompleted,SavedSearch,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeSavedSearchFailed,SavedSearch,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserComplete,User,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddUserCompleted,User,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addUserFailed,User,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddUserFailed,User,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserComplete,User,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateUserCompleted,User,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateUserFailed,User,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateUserFailed,User,QNLocalizedString,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookComplete,LinkedNotebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookComplete,LinkedNotebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeLinkedNotebookComplete,LinkedNotebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeLinkedNotebookCompleted,LinkedNotebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeLinkedNotebookFailed,LinkedNotebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listAllLinkedNotebooksComplete,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QUuid),
                        this,
                        QNSLOT(RemoteToLocalSynchronizationManager,onListAllLinkedNotebooksCompleted,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QList<LinkedNotebook>,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker,
                        QNSIGNAL(LocalStorageManagerThreadWorker,listAllLinkedNotebooksFailed,size_t,size_t,
                                 LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid),
                        this,
                        QNSLOT(RemoteToLocalSynchronizationManager,onListAllLinkedNotebooksFailed,size_t,size_t,
                               LocalStorageManager::ListLinkedNotebooksOrder::type,LocalStorageManager::OrderDirection::type,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotebookCompleted,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNotebookFailed,Notebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,Notebook,bool,bool,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNoteCompleted,Note,Notebook,bool,bool,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,Notebook,bool,bool,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateNoteFailed,Note,Notebook,bool,bool,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteComplete,Note,Notebook,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNoteCompleted,Note,Notebook,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNoteFailed,Note,Notebook,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddNoteFailed,Note,Notebook,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNoteCompleted,Note,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteFailed,Note,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onExpungeNoteFailed,Note,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addResourceComplete,Resource,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddResourceCompleted,Resource,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addResourceFailed,Resource,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onAddResourceFailed,Resource,QNLocalizedString,QUuid));

    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateResourceComplete,Resource,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateResourceCompleted,Resource,QUuid));
    QObject::disconnect(&m_localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateResourceFailed,Resource,QNLocalizedString,QUuid),
                        this, QNSLOT(RemoteToLocalSynchronizationManager,onUpdateResourceFailed,Resource,QNLocalizedString,QUuid));
    m_connectedToLocalStorage = false;

    // With the disconnect from local storage the list of previously received linked notebooks (if any) + new additions/updates becomes invalidated
    m_allLinkedNotebooksListed = false;
}

void RemoteToLocalSynchronizationManager::launchSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchSync"));

    if (m_authenticationToken.isEmpty()) {
        m_pendingAuthenticationTokenAndShardId = true;
        emit requestAuthenticationToken();
        return;
    }

    launchSavedSearchSync();
    launchLinkedNotebookSync();

    launchTagsSync();
    launchNotebookSync();

    if (!m_tags.empty() || !m_notebooks.empty()) {
        // NOTE: the sync of notes and, if need be, individual resouces would be launched asynchronously when the
        // notebooks and tags are synced
        return;
    }

    QNDEBUG(QStringLiteral("The local lists of tags and notebooks waiting for processing are empty, "
                           "checking if there are notes to process"));

    launchNotesSync();
    if (!m_notes.empty())
    {
        QNDEBUG(QStringLiteral("Synchronizing notes"));
        // NOTE: the sync of individual resources would be launched asynchronously (if current sync is incremental)
        // when the notes are synced
        return;
    }

    QNDEBUG(QStringLiteral("The local list of notes waiting for processing is empty"));

    if (m_lastSyncMode != SyncMode::IncrementalSync) {
        QNDEBUG(QStringLiteral("Running full sync => no sync for individual resources is needed"));
        return;
    }

    launchResourcesSync();
}

bool RemoteToLocalSynchronizationManager::checkProtocolVersion()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkProtocolVersion"));

    if (m_edamProtocolVersionChecked) {
        QNDEBUG(QStringLiteral("Already checked the protocol version, skipping it"));
        return true;
    }

    QNLocalizedString errorDescription;
    QString clientName = clientNameForProtocolVersionCheck();
    qint16 edamProtocolVersionMajor = qevercloud::EDAM_VERSION_MAJOR;
    qint16 edamProtocolVersionMinor = qevercloud::EDAM_VERSION_MINOR;
    bool protocolVersionChecked = m_userStore.checkVersion(clientName, edamProtocolVersionMajor, edamProtocolVersionMinor,
                                                           errorDescription);
    if (Q_UNLIKELY(!protocolVersionChecked))
    {
        if (!errorDescription.isEmpty()) {
            QNLocalizedString fullErrorDescription = QT_TR_NOOP("EDAM protocol version check failed");
            fullErrorDescription += QStringLiteral(": ");
            fullErrorDescription += errorDescription;
            errorDescription = fullErrorDescription;
        }
        else {
            errorDescription = QT_TR_NOOP("Evernote service reports that the protocol version of");
            errorDescription += QStringLiteral(" ");
            errorDescription += QString::number(edamProtocolVersionMajor);
            errorDescription += QStringLiteral(".");
            errorDescription += QString::number(edamProtocolVersionMinor);
            errorDescription += QStringLiteral(" ");
            errorDescription += QT_TR_NOOP("can no longer be used for the communication with it");
        }

        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return false;
    }

    m_edamProtocolVersionChecked = true;
    return true;
}

bool RemoteToLocalSynchronizationManager::syncUser()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::syncUser"));

    if (m_user.hasId() && m_user.hasServiceLevel()) {
        QNDEBUG(QStringLiteral("User id and service level are set, that means the user info has already been synchronized once "
                               "during the current session, won't do it again"));
        return true;
    }

    QNLocalizedString errorDescription;
    qint32 rateLimitSeconds = 0;
    qint32 errorCode = m_userStore.getUser(m_user, errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds <= 0) {
            errorDescription += QStringLiteral("\n");
            errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            return false;
        }

        int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        if (Q_UNLIKELY(timerId == 0)) {
            QNLocalizedString errorMessage = QT_TR_NOOP("can't start timer to postpone the Evernote API call due to rate limit exceeding");
            errorMessage += QStringLiteral(": ");
            errorMessage += errorDescription;
            emit failure(errorMessage);
            return false;
        }

        m_syncUserPostponeTimerId = timerId;

        QNDEBUG(QStringLiteral("Rate limit exceeded, need to wait for ") << rateLimitSeconds << QStringLiteral(" seconds"));
        emit rateLimitExceeded(rateLimitSeconds);
        return false;
    }
    else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("unexpected AUTH_EXPIRED error when trying to download "
                                                    "the latest information about the current user: ");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        QNDEBUG(errorMessage);
        emit failure(errorMessage);
        return false;
    }
    else if (errorCode != 0)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("can't download the latest user info");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        emit failure(errorMessage);
        return false;
    }

    if (m_user.hasAccountLimits()) {
        m_accountLimits = m_user.accountLimits();
        writeAccountLimitsToAppSettings();
    }

    // See if this user's entry already exists in the local storage or not
    m_findUserRequestId = QUuid::createUuid();
    QNTRACE(QStringLiteral("Emitting request to find user in the local storage database: request id = ")
            << m_findUserRequestId << QStringLiteral(", user = ") << m_user);
    emit findUser(m_user, m_findUserRequestId);

    return true;
}

bool RemoteToLocalSynchronizationManager::checkAndSyncAccountLimits()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkAndSyncAccountLimits"));

    ApplicationSettings appSettings;
    const QString keyGroup = ACCOUNT_LIMITS_KEY_GROUP + QString::number(m_userId) + QStringLiteral("/");

    QVariant accountLimitsLastSyncTime = appSettings.value(keyGroup + ACCOUNT_LIMITS_LAST_SYNC_TIME_KEY);
    if (!accountLimitsLastSyncTime.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null last sync time for account limits: ") << accountLimitsLastSyncTime);

        bool conversionResult = false;
        qint64 timestamp = accountLimitsLastSyncTime.toLongLong(&conversionResult);
        if (conversionResult)
        {
            QNTRACE(QStringLiteral("Successfully read last sync time for account limits: ")
                    << printableDateTimeFromTimestamp(timestamp));
            qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
            qint64 diff = currentTimestamp - timestamp;
            if ((diff > 0) && (diff < THIRTY_DAYS_IN_MSEC)) {
                QNTRACE("The cached account limits appear to be still valid");
                readSavedAccountLimits();
                return true;
            }
        }
    }

    return syncAccountLimits();
}

bool RemoteToLocalSynchronizationManager::syncAccountLimits()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::syncAccountLimits"));

    QNLocalizedString errorDescription;

    if (Q_UNLIKELY(!m_user.hasServiceLevel())) {
        errorDescription = QT_TR_NOOP("No Evernote service level was found for the current user");
        QNDEBUG(errorDescription);
        emit failure(errorDescription);
        return false;
    }

    qint32 rateLimitSeconds = 0;
    qint32 errorCode = m_userStore.getAccountLimits(m_user.serviceLevel(), m_accountLimits, errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds <= 0) {
            errorDescription += QStringLiteral("\n");
            errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            return false;
        }

        int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        if (Q_UNLIKELY(timerId == 0)) {
            QNLocalizedString errorMessage = QT_TR_NOOP("can't start timer to postpone the Evernote API call due to rate limit exceeding");
            errorMessage += QStringLiteral(": ");
            errorMessage += errorDescription;
            emit failure(errorMessage);
            return false;
        }

        m_syncAccountLimitsPostponeTimerId = timerId;

        QNDEBUG(QStringLiteral("Rate limit exceeded, need to wait for ") << rateLimitSeconds << QStringLiteral(" seconds"));
        emit rateLimitExceeded(rateLimitSeconds);
        return false;
    }
    else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("unexpected AUTH_EXPIRED error when trying to sync the current user's account limits");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        emit failure(errorMessage);
        return false;
    }
    else if (errorCode != 0)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("can't get account limits for the current user");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        emit failure(errorMessage);
        return false;
    }

    writeAccountLimitsToAppSettings();
    return true;
}

void RemoteToLocalSynchronizationManager::readSavedAccountLimits()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::readSavedAccountLimits"));

    m_accountLimits = qevercloud::AccountLimits();

    ApplicationSettings appSettings;
    const QString keyGroup = ACCOUNT_LIMITS_KEY_GROUP + QString::number(m_userId) + QStringLiteral("/");

    QVariant userMailLimitDaily = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_MAIL_LIMIT_DAILY_KEY);
    if (!userMailLimitDaily.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user mail limit daily account limit: ") << userMailLimitDaily);
        bool conversionResult = false;
        qint32 value = userMailLimitDaily.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userMailLimitDaily = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user mail limit daily account limit to qint32: ") << userMailLimitDaily);
        }
    }

    QVariant noteSizeMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_NOTE_SIZE_MAX_KEY);
    if (!noteSizeMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null note size max: ") << noteSizeMax);
        bool conversionResult = false;
        qint64 value = noteSizeMax.toLongLong(&conversionResult);
        if (conversionResult) {
            m_accountLimits.noteSizeMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert note size max account limit to qint64: ") << noteSizeMax);
        }
    }

    QVariant resourceSizeMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_RESOURCE_SIZE_MAX_KEY);
    if (!resourceSizeMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null resource size max: ") << resourceSizeMax);
        bool conversionResult = false;
        qint64 value = resourceSizeMax.toLongLong(&conversionResult);
        if (conversionResult) {
            m_accountLimits.resourceSizeMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert resource size max account limit to qint64: ") << resourceSizeMax);
        }
    }

    QVariant userLinkedNotebookMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_LINKED_NOTEBOOK_MAX_KEY);
    if (!userLinkedNotebookMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user linked notebook max: ") << userLinkedNotebookMax);
        bool conversionResult = false;
        qint32 value = userLinkedNotebookMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userLinkedNotebookMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user linked notebook max account limit to qint32: ") << userLinkedNotebookMax);
        }
    }

    QVariant uploadLimit = appSettings.value(keyGroup + ACCOUNT_LIMITS_UPLOAD_LIMIT_KEY);
    if (!uploadLimit.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null upload limit: ") << uploadLimit);
        bool conversionResult = false;
        qint64 value = uploadLimit.toLongLong(&conversionResult);
        if (conversionResult) {
            m_accountLimits.uploadLimit = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert upload limit to qint64: ") << uploadLimit);
        }
    }

    QVariant userNoteCountMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_NOTE_COUNT_MAX_KEY);
    if (!userNoteCountMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user note count max: ") << userNoteCountMax);
        bool conversionResult = false;
        qint32 value = userNoteCountMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userNoteCountMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user note count max to qint32: ") << userNoteCountMax);
        }
    }

    QVariant userNotebookCountMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_NOTEBOOK_COUNT_MAX_KEY);
    if (!userNotebookCountMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user notebook count max: ") << userNotebookCountMax);
        bool conversionResult = false;
        qint32 value = userNotebookCountMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userNotebookCountMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user notebook count max to qint32: ") << userNotebookCountMax);
        }
    }

    QVariant userTagCountMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_TAG_COUNT_MAX_KEY);
    if (!userTagCountMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user tag count max: ") << userTagCountMax);
        bool conversionResult = false;
        qint32 value = userTagCountMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userTagCountMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user tag count max to qint32: ") << userTagCountMax);
        }
    }

    QVariant noteTagCountMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_NOTE_TAG_COUNT_MAX_KEY);
    if (!noteTagCountMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null note tag cont max: ") << noteTagCountMax);
        bool conversionResult = false;
        qint32 value = noteTagCountMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.noteTagCountMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert note tag count max to qint32: ") << noteTagCountMax);
        }
    }

    QVariant userSavedSearchesMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_USER_SAVED_SEARCH_COUNT_MAX_KEY);
    if (!userSavedSearchesMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null user saved search max: ") << userSavedSearchesMax);
        bool conversionResult = false;
        qint32 value = userSavedSearchesMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.userSavedSearchesMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert user saved search max to qint32: ") << userSavedSearchesMax);
        }
    }

    QVariant noteResourceCountMax = appSettings.value(keyGroup + ACCOUNT_LIMITS_NOTE_RESOURCE_COUNT_MAX_KEY);
    if (!noteResourceCountMax.isNull())
    {
        QNTRACE(QStringLiteral("Found non-null note resource count max: ") << noteResourceCountMax);
        bool conversionResult = false;
        qint32 value = noteResourceCountMax.toInt(&conversionResult);
        if (conversionResult) {
            m_accountLimits.noteResourceCountMax = value;
        }
        else {
            QNWARNING(QStringLiteral("Failed to convert note resource count max to qint32: ") << noteResourceCountMax);
        }
    }

    QNTRACE("Read account limits from application settings: " << m_accountLimits);
}

void RemoteToLocalSynchronizationManager::writeAccountLimitsToAppSettings()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::writeAccountLimitsToAppSettings"));

    ApplicationSettings appSettings;
    const QString keyGroup = ACCOUNT_LIMITS_KEY_GROUP + QString::number(m_userId) + QStringLiteral("/");

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_MAIL_LIMIT_DAILY_KEY,
                         (m_accountLimits.userMailLimitDaily.isSet()
                          ? QVariant(m_accountLimits.userMailLimitDaily.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_NOTE_SIZE_MAX_KEY,
                         (m_accountLimits.noteSizeMax.isSet()
                          ? QVariant(m_accountLimits.noteSizeMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_RESOURCE_SIZE_MAX_KEY,
                         (m_accountLimits.resourceSizeMax.isSet()
                          ? QVariant(m_accountLimits.resourceSizeMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_LINKED_NOTEBOOK_MAX_KEY,
                         (m_accountLimits.userLinkedNotebookMax.isSet()
                          ? QVariant(m_accountLimits.userLinkedNotebookMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_UPLOAD_LIMIT_KEY,
                         (m_accountLimits.uploadLimit.isSet()
                          ? QVariant(m_accountLimits.uploadLimit.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_NOTE_COUNT_MAX_KEY,
                         (m_accountLimits.userNoteCountMax.isSet()
                          ? QVariant(m_accountLimits.userNoteCountMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_NOTEBOOK_COUNT_MAX_KEY,
                         (m_accountLimits.userNotebookCountMax.isSet()
                          ? QVariant(m_accountLimits.userNotebookCountMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_TAG_COUNT_MAX_KEY,
                         (m_accountLimits.userTagCountMax.isSet()
                          ? QVariant(m_accountLimits.userTagCountMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_NOTE_TAG_COUNT_MAX_KEY,
                         (m_accountLimits.noteTagCountMax.isSet()
                          ? QVariant(m_accountLimits.noteTagCountMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_USER_SAVED_SEARCH_COUNT_MAX_KEY,
                         (m_accountLimits.userSavedSearchesMax.isSet()
                          ? QVariant(m_accountLimits.userSavedSearchesMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_NOTE_RESOURCE_COUNT_MAX_KEY,
                         (m_accountLimits.noteResourceCountMax.isSet()
                          ? QVariant(m_accountLimits.noteResourceCountMax.ref())
                          : QVariant()));

    appSettings.setValue(keyGroup + ACCOUNT_LIMITS_LAST_SYNC_TIME_KEY, QVariant(QDateTime::currentMSecsSinceEpoch()));
}

void RemoteToLocalSynchronizationManager::launchTagsSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchTagsSync"));
    launchDataElementSync<TagsList, Tag>(ContentSource::UserAccount, QStringLiteral("Tag"), m_tags, m_expungedTags);
}

void RemoteToLocalSynchronizationManager::launchSavedSearchSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchSavedSearchSync"));
    launchDataElementSync<SavedSearchesList, SavedSearch>(ContentSource::UserAccount, QStringLiteral("Saved search"),
                                                          m_savedSearches, m_expungedSavedSearches);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebookSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchLinkedNotebookSync"));
    launchDataElementSync<LinkedNotebooksList, LinkedNotebook>(ContentSource::UserAccount, QStringLiteral("Linked notebook"),
                                                               m_linkedNotebooks, m_expungedLinkedNotebooks);
}

void RemoteToLocalSynchronizationManager::launchNotebookSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchNotebookSync"));
    launchDataElementSync<NotebooksList, Notebook>(ContentSource::UserAccount, QStringLiteral("Notebook"), m_notebooks, m_expungedNotebooks);
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
        strm << QStringLiteral("UserAccount");
        break;
    case RemoteToLocalSynchronizationManager::ContentSource::LinkedNotebook:
        strm << QStringLiteral("LinkedNotebook");
        break;
    default:
        strm << QStringLiteral("Unknown");
        break;
    }

    return strm;
}

void RemoteToLocalSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkNotebooksAndTagsSyncAndLaunchNotesSync"));

    if (m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty() &&
        m_updateTagRequestIds.empty() && m_addTagRequestIds.empty())
    {
        // All remote notebooks and tags were already either updated in the local storage or added there
        launchNotesSync();
    }
}

void RemoteToLocalSynchronizationManager::launchNotesSync()
{
    launchDataElementSync<NotesList, Note>(ContentSource::UserAccount, QStringLiteral("Note"), m_notes, m_expungedNotes);
}

void RemoteToLocalSynchronizationManager::checkNotesSyncAndLaunchResourcesSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkNotesSyncAndLaunchResourcesSync"));

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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchResourcesSync"));
    QList<QString> dummyList;
    launchDataElementSync<ResourcesList, Resource>(ContentSource::UserAccount, QStringLiteral("Resource"), m_resources, dummyList);
}

void RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncAndLaunchLinkedNotebookContentSync"));

    if (m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty()) {
        // All remote linked notebooks were already updated in the local storage or added there
        startLinkedNotebooksSync();
    }
}

void RemoteToLocalSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaunchLinkedNotebookNotesSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkLinkedNotebooksNotebooksAndTagsSyncAndLaunchLinkedNotebookNotesSync"));

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
            QNLocalizedString error = QT_TR_NOOP("detected attempt to map linked notebook guid to tag without guid set");
            QNWARNING(error << QStringLiteral(", tag: ") << tag);
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
            QNLocalizedString error = QT_TR_NOOP("detected attempt to map linked notebook guid to notebook without guid set");
            QNWARNING(error << QStringLiteral(", notebook: ") << notebook);
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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::startLinkedNotebooksSync"));

    if (!m_allLinkedNotebooksListed) {
        requestAllLinkedNotebooks();
        return;
    }

    const int numAllLinkedNotebooks = m_allLinkedNotebooks.size();
    if (numAllLinkedNotebooks == 0) {
        QNDEBUG(QStringLiteral("No linked notebooks are present within the account, can finish the synchronization right away"));
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
    const int numAllLinkedNotebooks = m_allLinkedNotebooks.size();
    for(int i = 0; i < numAllLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = m_allLinkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QNLocalizedString error = QT_TR_NOOP("found linked notebook without guid set");
            QNWARNING(error << QStringLiteral(", linked notebook: ") << linkedNotebook);
            emit failure(error);
            return false;
        }

        if (!m_authenticationTokensAndShardIdsByLinkedNotebookGuid.contains(linkedNotebook.guid())) {
            QNDEBUG(QStringLiteral("Authentication token for linked notebook with guid ") << linkedNotebook.guid()
                    << QStringLiteral(" was not found; will request authentication tokens for all linked notebooks at once"));
            requestAuthenticationTokensForAllLinkedNotebooks();
            return false;
        }

        auto it = m_authenticationTokenExpirationTimesByLinkedNotebookGuid.find(linkedNotebook.guid());
        if (it == m_authenticationTokenExpirationTimesByLinkedNotebookGuid.end()) {
            QNLocalizedString error = QT_TR_NOOP("can't find cached expiration time of linked notebook's authentication token");
            QNWARNING(error << QStringLiteral(", linked notebook: ") << linkedNotebook);
            emit failure(error);
            return false;
        }

        const qevercloud::Timestamp & expirationTime = it.value();
        const qevercloud::Timestamp currentTime = QDateTime::currentMSecsSinceEpoch();
        if (currentTime - expirationTime < SIX_HOURS_IN_MSEC) {
            QNDEBUG(QStringLiteral("Authentication token for linked notebook with guid ") << linkedNotebook.guid()
                    << QStringLiteral(" is too close to expiration: its expiration time is ")
                    << printableDateTimeFromTimestamp(expirationTime) << QStringLiteral(", current time is ")
                    << printableDateTimeFromTimestamp(currentTime)
                    << QStringLiteral("; will request new authentication tokens for all linked notebooks"));

            requestAuthenticationTokensForAllLinkedNotebooks();
            return false;
        }
    }

    QNDEBUG(QStringLiteral("Got authentication tokens for all linked notebooks, can proceed with their synchronization"));

    return true;
}

void RemoteToLocalSynchronizationManager::requestAuthenticationTokensForAllLinkedNotebooks()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::requestAuthenticationTokensForAllLinkedNotebooks"));

    QList<QPair<QString,QString> > linkedNotebookGuidsAndSharedNotebookGlobalIds;
    const int numAllLinkedNotebooks = m_allLinkedNotebooks.size();
    for(int j = 0; j < numAllLinkedNotebooks; ++j)
    {
        const LinkedNotebook & currentLinkedNotebook = m_allLinkedNotebooks[j];

        if (!currentLinkedNotebook.hasGuid()) {
            QNLocalizedString error = QT_TR_NOOP("found linked notebook without guid set");
            QNWARNING(error << QStringLiteral(", linked notebook: ") << currentLinkedNotebook);
            emit failure(error);
            return;
        }

        if (!currentLinkedNotebook.hasSharedNotebookGlobalId()) {
            QNLocalizedString error = QT_TR_NOOP("found linked notebook without shared notebook global id");
            QNWARNING(error << QStringLiteral(", linked notebook: ") << currentLinkedNotebook);
            emit failure(error);
            return;
        }

        linkedNotebookGuidsAndSharedNotebookGlobalIds << QPair<QString,QString>(currentLinkedNotebook.guid(),
                                                                                currentLinkedNotebook.sharedNotebookGlobalId());
    }

    emit requestAuthenticationTokensForLinkedNotebooks(linkedNotebookGuidsAndSharedNotebookGlobalIds);
    m_pendingAuthenticationTokensForLinkedNotebooks = true;
}

void RemoteToLocalSynchronizationManager::requestAllLinkedNotebooks()
{
    size_t limit = 0, offset = 0;
    LocalStorageManager::ListLinkedNotebooksOrder::type order = LocalStorageManager::ListLinkedNotebooksOrder::NoOrder;
    LocalStorageManager::OrderDirection::type orderDirection = LocalStorageManager::OrderDirection::Ascending;

    m_listAllLinkedNotebooksRequestId = QUuid::createUuid();
    emit listAllLinkedNotebooks(limit, offset, order, orderDirection, m_listAllLinkedNotebooksRequestId);

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::requestAllLinkedNotebooks: request id = ")
            << m_listAllLinkedNotebooksRequestId);
}

void RemoteToLocalSynchronizationManager::getLinkedNotebookSyncState(const LinkedNotebook & linkedNotebook,
                                                                     const QString & authToken, qevercloud::SyncState & syncState,
                                                                     bool & asyncWait, bool & error)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::getLinkedNotebookSyncState"));

    asyncWait = false;
    error = false;

    QNLocalizedString errorDescription;
    qint32 rateLimitSeconds = 0;
    qint32 errorCode = m_noteStore.getLinkedNotebookSyncState(linkedNotebook, authToken, syncState,
                                                              errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds <= 0) {
            errorDescription += QStringLiteral("\n");
            errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            error = true;
            return;
        }

        int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        if (Q_UNLIKELY(timerId == 0)) {
            QNLocalizedString errorMessage = QT_TR_NOOP("can't start timer to postpone the Evernote API call due to rate limit exceeding");
            errorMessage += QStringLiteral(": ");
            errorMessage += errorDescription;
            emit failure(errorMessage);
            error = true;
            return;
        }

        m_getLinkedNotebookSyncStateBeforeStartAPICallPostponeTimerId = timerId;

        QNDEBUG(QStringLiteral("Rate limit exceeded, need to wait for ") << rateLimitSeconds << QStringLiteral(" seconds"));
        emit rateLimitExceeded(rateLimitSeconds);
        asyncWait = true;
        return;
    }
    else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("unexpected AUTH_EXPIRED error when trying to get linked notebook sync state");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        emit failure(errorMessage);
        error = true;
        return;
    }
    else if (errorCode != 0)
    {
        QNLocalizedString errorMessage = QT_TR_NOOP("can't get linked notebook sync state");
        errorMessage += QStringLiteral(": ");
        errorMessage += errorDescription;
        emit failure(errorMessage);
        error = true;
        return;
    }
}

bool RemoteToLocalSynchronizationManager::downloadLinkedNotebooksSyncChunks()
{
    qevercloud::SyncChunk * pSyncChunk = Q_NULLPTR;

    QNDEBUG(QStringLiteral("Downloading linked notebook sync chunks:"));

    const int numAllLinkedNotebooks = m_allLinkedNotebooks.size();
    for(int i = 0; i < numAllLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = m_allLinkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QNLocalizedString error = QT_TR_NOOP("found linked notebook without guid set when "
                                                 "attempting to synchronize the content it points to");
            QNWARNING(error << QStringLiteral(": ") << linkedNotebook);
            emit failure(error);
            return false;
        }

        const QString & linkedNotebookGuid = linkedNotebook.guid();

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
            QNDEBUG(QStringLiteral("Sync chunks were already downloaded for linked notebook with guid ") << linkedNotebookGuid);
            continue;
        }

        auto it = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (it == m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end()) {
            QNLocalizedString error = QT_TR_NOOP("can't find authentication token for one of linked notebooks "
                                                 "when attempting to synchronize the content it points to");
            QNWARNING(error << QStringLiteral(": ") << linkedNotebook);
            emit failure(error);
            return false;
        }

        const QString & authToken = it.value().first;

        if (m_onceSyncDone || (afterUsn != 0))
        {
            auto syncStateIter = m_syncStatesByLinkedNotebookGuid.find(linkedNotebookGuid);
            if (syncStateIter == m_syncStatesByLinkedNotebookGuid.end())
            {
                qevercloud::SyncState syncState;
                bool error = false;
                bool asyncWait = false;
                getLinkedNotebookSyncState(linkedNotebook, authToken, syncState, asyncWait, error);
                if (asyncWait || error) {
                    return false;
                }

                syncStateIter = m_syncStatesByLinkedNotebookGuid.insert(linkedNotebookGuid, syncState);
            }

            const qevercloud::SyncState & syncState = syncStateIter.value();

            if (syncState.fullSyncBefore > lastSyncTime)
            {
                QNDEBUG(QStringLiteral("Linked notebook sync state says the time has come to do the full sync"));
                afterUsn = 0;
                if (!m_onceSyncDone) {
                    fullSyncOnly = true;
                }
                m_lastSyncMode = SyncMode::FullSync;
            }
            else if (syncState.updateCount == lastUpdateCount)
            {
                QNDEBUG(QStringLiteral("Server has no updates for data in this linked notebook, continuing with the next one"));
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

            QNLocalizedString errorDescription;
            qint32 rateLimitSeconds = 0;
            qint32 errorCode = m_noteStore.getLinkedNotebookSyncChunk(linkedNotebook, afterUsn,
                                                                      m_maxSyncChunkEntries,
                                                                      authToken, fullSyncOnly, *pSyncChunk,
                                                                      errorDescription, rateLimitSeconds);
            if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
            {
                if (rateLimitSeconds <= 0) {
                    errorDescription += QStringLiteral("\n");
                    errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                    errorDescription += QStringLiteral(": ");
                    errorDescription += QString::number(rateLimitSeconds);
                    emit failure(errorDescription);
                    return false;
                }

                m_linkedNotebookSyncChunks.pop_back();

                int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
                if (Q_UNLIKELY(timerId == 0)) {
                    QNLocalizedString errorMessage = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                                                "due to rate limit exceeding");
                    errorMessage += QStringLiteral(": ");
                    errorMessage += errorDescription;
                    emit failure(errorMessage);
                    return false;
                }

                m_downloadLinkedNotebookSyncChunkAPICallPostponeTimerId = timerId;

                QNDEBUG(QStringLiteral("Rate limit exceeded, need to wait for ") << rateLimitSeconds << QStringLiteral(" seconds"));
                emit rateLimitExceeded(rateLimitSeconds);
                return false;
            }
            else if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
            {
                QNLocalizedString errorMessage = QT_TR_NOOP("unexpected AUTH_EXPIRED error when trying to download "
                                                            "the linked notebook sync chunks: ");
                errorMessage += QStringLiteral(": ");
                errorMessage += errorDescription;
                QNDEBUG(errorMessage);
                emit failure(errorMessage);
                return false;
            }
            else if (errorCode != 0)
            {
                QNLocalizedString errorMessage = QT_TR_NOOP("can't download the sync chunks for linked notebooks content");
                errorMessage += QStringLiteral(": ");
                errorMessage += errorDescription;
                QNDEBUG(errorMessage);
                emit failure(errorMessage);
                return false;
            }

            QNDEBUG(QStringLiteral("Received sync chunk: ") << *pSyncChunk);

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

        Q_UNUSED(m_linkedNotebookGuidsForWhichSyncChunksWereDownloaded.insert(linkedNotebook.guid()));
        m_lastSynchronizedUsnByLinkedNotebookGuid[linkedNotebookGuid] = afterUsn;
    }

    QNDEBUG(QStringLiteral("Done. Processing content pointed to by linked notebooks from buffered sync chunks"));

    m_syncStatesByLinkedNotebookGuid.clear();   // don't need this anymore, it only served the purpose of preventing multiple get sync state calls for the same linked notebook

    m_linkedNotebooksSyncChunksDownloaded = true;
    emit linkedNotebooksSyncChunksDownloaded();

    return true;
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksTagsSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchLinkedNotebooksTagsSync"));
    QList<QString> dummyList;
    launchDataElementSync<TagsList, Tag>(ContentSource::LinkedNotebook, QStringLiteral("Tag"), m_tags, dummyList);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotebooksSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotebooksSync"));
    QList<QString> dummyList;
    launchDataElementSync<NotebooksList, Notebook>(ContentSource::LinkedNotebook, QStringLiteral("Notebook"), m_notebooks, dummyList);
}

void RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotesSync()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::launchLinkedNotebooksNotesSync"));
    launchDataElementSync<NotesList, Note>(ContentSource::LinkedNotebook, QStringLiteral("Note"), m_notes, m_expungedNotes);
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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkServerDataMergeCompletion"));

    // Need to check whether we are still waiting for the response from some add or update request
    bool tagsReady = m_updateTagRequestIds.empty() && m_addTagRequestIds.empty();
    if (!tagsReady) {
        QNDEBUG(QStringLiteral("Tags are not ready, pending response for ") << m_updateTagRequestIds.size()
                << QStringLiteral(" tag update requests and/or ") << m_addTagRequestIds.size() << QStringLiteral(" tag add requests"));
        return;
    }

    bool searchesReady = m_updateSavedSearchRequestIds.empty() && m_addSavedSearchRequestIds.empty();
    if (!searchesReady) {
        QNDEBUG(QStringLiteral("Saved searches are not ready, pending response for ") << m_updateSavedSearchRequestIds.size()
                << QStringLiteral(" saved search update requests and/or ") << m_addSavedSearchRequestIds.size()
                << QStringLiteral(" saved search add requests"));
        return;
    }

    bool linkedNotebooksReady = m_updateLinkedNotebookRequestIds.empty() && m_addLinkedNotebookRequestIds.empty();
    if (!linkedNotebooksReady) {
        QNDEBUG(QStringLiteral("Linked notebooks are not ready, pending response for ") << m_updateLinkedNotebookRequestIds.size()
                << QStringLiteral(" linked notebook update requests and/or ") << m_addLinkedNotebookRequestIds.size()
                << QStringLiteral(" linked notebook add requests"));
        return;
    }

    bool notebooksReady = m_updateNotebookRequestIds.empty() && m_addNotebookRequestIds.empty();
    if (!notebooksReady) {
        QNDEBUG(QStringLiteral("Notebooks are not ready, pending response for ") << m_updateNotebookRequestIds.size()
                << QStringLiteral(" notebook update requests and/or ") << m_addNotebookRequestIds.size()
                << QStringLiteral(" notebook add requests"));
        return;
    }

    bool notesReady = m_updateNoteRequestIds.empty() && m_addNoteRequestIds.empty() &&
                      m_notesToAddPerAPICallPostponeTimerId.empty() && m_notesToUpdatePerAPICallPostponeTimerId.empty();
    if (!notesReady) {
        QNDEBUG(QStringLiteral("Notes are not ready, pending response for ") << m_updateNoteRequestIds.size()
                << QStringLiteral(" note update requests and/or ") << m_addNoteRequestIds.size()
                << QStringLiteral(" note add requests; also, there are ") << m_notesToAddPerAPICallPostponeTimerId.size()
                << QStringLiteral(" postponed note add requests and/or ") << m_notesToUpdatePerAPICallPostponeTimerId.size()
                << QStringLiteral(" note update requests"));
        return;
    }

    if (m_lastSyncMode == SyncMode::IncrementalSync)
    {
        bool resourcesReady = m_updateResourceRequestIds.empty() && m_addResourceRequestIds.empty() &&
                              m_resourcesWithFindRequestIdsPerFindNoteRequestId.empty() &&
                              m_findNotebookForNotesWithConflictedResourcesRequestIds.empty() &&
                              m_inkNoteResourceDataPerFindNotebookRequestId.empty() &&
                              (m_numPendingInkNoteImageDownloads == 0) &&
                              m_noteGuidForThumbnailDownloadByFindNotebookRequestId.empty() &&
                              m_noteGuidsPendingThumbnailDownload.empty();
        if (!resourcesReady) {
            QNDEBUG(QStringLiteral("Resources are not ready, pending response for ") << m_updateResourceRequestIds.size()
                    << QStringLiteral(" resource update requests and/or ") << m_addResourceRequestIds.size()
                    << QStringLiteral(" resource add requests and/or ") << m_resourcesWithFindRequestIdsPerFindNoteRequestId.size()
                    << QStringLiteral(" resource find note requests and/or ") << m_findNotebookForNotesWithConflictedResourcesRequestIds.size()
                    << QStringLiteral(" resource find notebook requests and/or ") << m_inkNoteResourceDataPerFindNotebookRequestId.size()
                    << QStringLiteral(" resource find notebook for ink note image download processing and/or ") << m_numPendingInkNoteImageDownloads
                    << QStringLiteral(" ink note image downloads and/or ") << m_noteGuidForThumbnailDownloadByFindNotebookRequestId.size()
                    << QStringLiteral(" resource find notebook for note thumbnail download processing and/or ") << m_noteGuidsPendingThumbnailDownload.size()
                    << QStringLiteral(" note thumbnail downloads"));
            return;
        }
    }

    if (syncingLinkedNotebooksContent())
    {
        QNDEBUG(QStringLiteral("Synchronized the whole contents from linked notebooks"));
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
        QNDEBUG(QStringLiteral("Synchronized the whole contents from user's account"));

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
    m_active = false;
}

void RemoteToLocalSynchronizationManager::clear()
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::clear"));

    // NOTE: not clearing authentication tokens and shard ids; it is intentional,
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

    m_allLinkedNotebooks.clear();
    m_listAllLinkedNotebooksRequestId = QUuid();
    m_allLinkedNotebooksListed = false;
    m_pendingAuthenticationTokensForLinkedNotebooks = false;

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
    m_inkNoteResourceDataPerFindNotebookRequestId.clear();
    m_numPendingInkNoteImageDownloads = 0;
    m_noteGuidForThumbnailDownloadByFindNotebookRequestId.clear();
    m_noteGuidsPendingThumbnailDownload.clear();
    m_resourceFoundFlagPerFindResourceRequestId.clear();
    m_resourceConflictedAndRemoteNotesPerNotebookGuid.clear();
    m_findNotebookForNotesWithConflictedResourcesRequestIds.clear();

    m_localUidsOfElementsAlreadyAttemptedToFindByName.clear();

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

void RemoteToLocalSynchronizationManager::handleLinkedNotebookAdded(const LinkedNotebook & linkedNotebook)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::handleLinkedNotebookAdded: linked notebook = ") << linkedNotebook);

    if (!m_allLinkedNotebooksListed) {
        return;
    }

    if (!linkedNotebook.hasGuid()) {
        QNWARNING(QStringLiteral("Detected the addition of linked notebook without guid to local storage!"));
        return;
    }

    auto it = std::find_if(m_allLinkedNotebooks.begin(), m_allLinkedNotebooks.end(),
                           CompareItemByGuid<qevercloud::LinkedNotebook>(linkedNotebook.guid()));
    if (it != m_allLinkedNotebooks.end()) {
        QNINFO(QStringLiteral("Detected the addition of linked notebook to local storage, however such linked notebook is "
                              "already present within the list of all linked notebooks received previously from local storage"));
        *it = linkedNotebook;
        return;
    }

    m_allLinkedNotebooks << linkedNotebook;
}

void RemoteToLocalSynchronizationManager::handleLinkedNotebookUpdated(const LinkedNotebook & linkedNotebook)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::handleLinkedNotebookUpdated: linked notebook = ") << linkedNotebook);

    if (!m_allLinkedNotebooksListed) {
        return;
    }

    if (!linkedNotebook.hasGuid()) {
        QNWARNING(QStringLiteral("Detected the updated linked notebook without guid in local storage!"));
        return;
    }

    auto it = std::find_if(m_allLinkedNotebooks.begin(), m_allLinkedNotebooks.end(),
                           CompareItemByGuid<qevercloud::LinkedNotebook>(linkedNotebook.guid()));
    if (it == m_allLinkedNotebooks.end()) {
        QNINFO(QStringLiteral("Detected the update of linked notebook to local storage, however such linked notebook is "
                              "not present within the list of all linked notebooks received previously from local storage"));
        m_allLinkedNotebooks << linkedNotebook;
        return;
    }

    *it = linkedNotebook;
}

void RemoteToLocalSynchronizationManager::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::timerEvent"));

    if (!pEvent) {
        QNLocalizedString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit failure(errorDescription);
        return;
    }

    int timerId = pEvent->timerId();
    killTimer(timerId);
    QNDEBUG(QStringLiteral("Killed timer with id ") << timerId);

    if (m_paused)
    {
        QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager is being paused. Won't try to download full Note data "
                               "but will return the notes waiting to be downloaded into the common list of notes"));

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

        QNLocalizedString errorDescription;
        qint32 postponeAPICallSeconds = tryToGetFullNoteData(note, errorDescription);
        if (postponeAPICallSeconds > 0)
        {
            int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
            if (Q_UNLIKELY(timerId == 0)) {
                QNLocalizedString errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                                                "due to rate limit exceeding");
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

        QNLocalizedString errorDescription;
        qint32 postponeAPICallSeconds = tryToGetFullNoteData(noteToUpdate, errorDescription);
        if (postponeAPICallSeconds > 0)
        {
            int timerId = startTimer(SEC_TO_MSEC(postponeAPICallSeconds));
            if (timerId == 0) {
                QNLocalizedString errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                                                "due to rate limit exceeding");
                emit failure(errorDescription);
                return;
            }

            m_notesToUpdatePerAPICallPostponeTimerId[timerId] = noteToUpdate;
            emit rateLimitExceeded(postponeAPICallSeconds);
        }
        else if (postponeAPICallSeconds == 0)
        {
            // NOTE: workarounding the stupidity of MSVC 2013
            emitUpdateRequest<Note>(noteToUpdate, static_cast<const Note*>(Q_NULLPTR));
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

    if (m_syncUserPostponeTimerId == timerId) {
        m_syncUserPostponeTimerId = 0;
        start(m_lastUsnOnStart);
        return;
    }

    if (m_syncAccountLimitsPostponeTimerId == timerId) {
        m_syncAccountLimitsPostponeTimerId = 0;
        start(m_lastUsnOnStart);
        return;
    }
}

qint32 RemoteToLocalSynchronizationManager::tryToGetFullNoteData(Note & note, QNLocalizedString & errorDescription)
{
    if (!note.hasGuid()) {
        errorDescription = QT_TR_NOOP("detected attempt to get full note's data for note without guid");
        QNWARNING(errorDescription << QStringLiteral(": ") << note);
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
                                          "exception but the number of seconds to wait is zero or negative");
            errorDescription += QStringLiteral(": ");
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
    qevercloud::SyncChunk * pSyncChunk = Q_NULLPTR;

    QNDEBUG(QStringLiteral("Downloading sync chunks:"));

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

        QNLocalizedString errorDescription;
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = m_noteStore.getSyncChunk(afterUsn, m_maxSyncChunkEntries, filter,
                                                    *pSyncChunk, errorDescription, rateLimitSeconds);
        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += QStringLiteral("\n");
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(rateLimitSeconds);
                emit failure(errorDescription);
                return;
            }

            m_syncChunks.pop_back();

            int timerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            if (Q_UNLIKELY(timerId == 0)) {
                QNLocalizedString errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
                                                                "due to rate limit exceeding");
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
        else if (errorCode != 0)
        {
            QNLocalizedString errorMessage = QT_TR_NOOP("can't download the sync chunks");
            errorMessage += QStringLiteral(": ");
            errorMessage += errorDescription;
            emit failure(errorMessage);
            return;
        }

        QNDEBUG(QStringLiteral("Received sync chunk: ") << *pSyncChunk);
    }

    QNDEBUG(QStringLiteral("Done. Processing tags, saved searches, linked notebooks and notebooks from buffered sync chunks"));

    m_lastSyncChunksDownloadedUsn = afterUsn;
    m_syncChunksDownloaded = true;
    emit syncChunksDownloaded();

    launchSync();
}

const Notebook * RemoteToLocalSynchronizationManager::getNotebookPerNote(const Note & note) const
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::getNotebookPerNote: note = ") << note);

    QString noteGuid = (note.hasGuid() ? note.guid() : QString());
    QString noteLocalUid = note.localUid();

    QPair<QString,QString> key(noteGuid, noteLocalUid);
    QHash<QPair<QString,QString>,Notebook>::const_iterator cit = m_notebooksPerNoteGuids.find(key);
    if (cit == m_notebooksPerNoteGuids.end()) {
        return Q_NULLPTR;
    }
    else {
        return &(cit.value());
    }
}

void RemoteToLocalSynchronizationManager::handleAuthExpiration()
{
    QNINFO(QStringLiteral("Got AUTH_EXPIRED error, pausing the sync and requesting a new authentication token"));

    m_paused = true;
    emit paused(/* pending authentication = */ true);

    m_pendingAuthenticationTokenAndShardId = true;
    emit requestAuthenticationToken();
}

bool RemoteToLocalSynchronizationManager::checkUserAccountSyncState(bool & asyncWait, bool & error, qint32 & afterUsn)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkUsersAccountSyncState"));

    asyncWait = false;
    error = false;

    QNLocalizedString errorDescription;
    qint32 rateLimitSeconds = 0;
    qevercloud::SyncState state;
    qint32 errorCode = m_noteStore.getSyncState(state, errorDescription, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds <= 0) {
            errorDescription += QStringLiteral("\n");
            errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(rateLimitSeconds);
            emit failure(errorDescription);
            error = true;
            return false;
        }

        m_getSyncStateBeforeStartAPICallPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        if (Q_UNLIKELY(m_getSyncStateBeforeStartAPICallPostponeTimerId == 0)) {
            errorDescription = QT_TR_NOOP("can't start timer to postpone the Evernote API call "
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
        QNDEBUG(QStringLiteral("Sync state says the time has come to do the full sync"));
        afterUsn = 0;
        m_lastSyncMode = SyncMode::FullSync;
    }
    else if (state.updateCount == m_lastUpdateCount)
    {
        QNDEBUG(QStringLiteral("Server has no updates for user's data since the last sync"));
        return false;
    }

    return true;
}

bool RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncStates(bool & asyncWait, bool & error)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::checkLinkedNotebooksSyncStates"));

    asyncWait = false;
    error = false;

    if (!m_allLinkedNotebooksListed) {
        QNTRACE(QStringLiteral("The list of all linked notebooks was not obtained from local storage yet, need to wait for it to happen"));
        requestAllLinkedNotebooks();
        asyncWait = true;
        return false;
    }

    if (m_allLinkedNotebooks.empty()) {
        QNTRACE(QStringLiteral("The list of all linked notebooks is empty, nothing to check sync states for"));
        return false;
    }

    if (m_pendingAuthenticationTokensForLinkedNotebooks) {
        QNTRACE(QStringLiteral("Pending authentication tokens for linked notebook flag is set, "
                               "need to wait for auth tokens"));
        asyncWait = true;
        return false;
    }

    const int numAllLinkedNotebooks = m_allLinkedNotebooks.size();
    for(int i = 0; i < numAllLinkedNotebooks; ++i)
    {
        const LinkedNotebook & linkedNotebook = m_allLinkedNotebooks[i];
        if (!linkedNotebook.hasGuid()) {
            QNLocalizedString errorMessage = QT_TR_NOOP("found linked notebook without guid set in local storage");
            emit failure(errorMessage);
            QNWARNING(errorMessage << QStringLiteral(", linked notebook: ") << linkedNotebook);
            error = true;
            return false;
        }

        const QString & linkedNotebookGuid = linkedNotebook.guid();

        auto it = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (it == m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end()) {
            QNINFO(QStringLiteral("Detected missing auth token for one of linked notebooks. Will request auth tokens "
                                  "for all linked notebooks now. Linked notebook without auth token: ") << linkedNotebook);
            requestAuthenticationTokensForAllLinkedNotebooks();
            asyncWait = true;
            return false;
        }

        const QString & authToken = it.value().first;

        auto lastUpdateCountIt = m_lastUpdateCountByLinkedNotebookGuid.find(linkedNotebookGuid);
        if (lastUpdateCountIt == m_lastUpdateCountByLinkedNotebookGuid.end()) {
            lastUpdateCountIt = m_lastUpdateCountByLinkedNotebookGuid.insert(linkedNotebookGuid, 0);
        }
        qint32 lastUpdateCount = lastUpdateCountIt.value();

        qevercloud::SyncState state;
        getLinkedNotebookSyncState(linkedNotebook, authToken, state, asyncWait, error);
        if (asyncWait || error) {
            return false;
        }

        if (state.updateCount == lastUpdateCount) {
            QNTRACE(QStringLiteral("Evernote service has no updates for linked notebook with guid ") << linkedNotebookGuid);
            continue;
        }
        else {
            QNTRACE(QStringLiteral("Detected mismatch in update counts for linked notebook with guid ") << linkedNotebookGuid
                    << QStringLiteral(": last update count = ") << lastUpdateCount << QStringLiteral(", sync state's update count: ")
                    << state.updateCount);
            return true;
        }
    }

    QNTRACE(QStringLiteral("Checked sync states for all linked notebooks, found no updates from Evernote service"));
    return false;
}

void RemoteToLocalSynchronizationManager::authenticationInfoForNotebook(const Notebook & notebook, QString & authToken,
                                                                        QString & shardId, bool & isPublic) const
{
    isPublic = notebook.hasPublished() && notebook.isPublished();

    if (notebook.hasLinkedNotebookGuid())
    {
        auto it = m_authenticationTokensAndShardIdsByLinkedNotebookGuid.find(notebook.linkedNotebookGuid());
        if (Q_UNLIKELY(it == m_authenticationTokensAndShardIdsByLinkedNotebookGuid.end())) {
            QNWARNING(QStringLiteral("Can't download an ink note image: no authentication token and shard id for linked notebook: ")
                      << notebook);
            return;
        }

        authToken = it.value().first;
        shardId = it.value().second;
    }
    else
    {
        authToken = m_authenticationToken;
        shardId = m_shardId;
    }
}

void RemoteToLocalSynchronizationManager::setupInkNoteImageDownloading(const QString & resourceGuid, const int resourceHeight,
                                                                       const int resourceWidth, const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::setupInkNoteImageDownloading: resource guid = ")
            << resourceGuid << QStringLiteral(", resource height = ") << resourceHeight << QStringLiteral(", resource width = ")
            << resourceWidth << QStringLiteral(", notebook: ") << notebook);

    QString authToken, shardId;
    bool isPublicNotebook = false;
    authenticationInfoForNotebook(notebook, authToken, shardId, isPublicNotebook);

    ++m_numPendingInkNoteImageDownloads;

    InkNoteImageDownloader * pDownloader = new InkNoteImageDownloader(m_host, resourceGuid, authToken,
                                                                      shardId, resourceHeight, resourceWidth,
                                                                      /* from public linked notebook = */
                                                                      isPublicNotebook, this);
    QObject::connect(pDownloader, QNSIGNAL(InkNoteImageDownloader,finished,bool,QString,QNLocalizedString),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onInkNoteImageDownloadFinished,bool,QString,QNLocalizedString));
    QThreadPool::globalInstance()->start(pDownloader);
}

void RemoteToLocalSynchronizationManager::setupNoteThumbnailDownloading(const QString & noteGuid, const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::setupNoteThumbnailDownloading: note guid = ") << noteGuid
            << QStringLiteral(", notebook: ") << notebook);

    QString authToken, shardId;
    bool isPublicNotebook = false;
    authenticationInfoForNotebook(notebook, authToken, shardId, isPublicNotebook);

    Q_UNUSED(m_noteGuidsPendingThumbnailDownload.insert(noteGuid))

    NoteThumbnailDownloader * pDownloader = new NoteThumbnailDownloader(m_host, noteGuid, authToken, shardId,
                                                                        /* from public linked notebook = */
                                                                        isPublicNotebook, this);
    QObject::connect(pDownloader, QNSIGNAL(NoteThumbnailDownloader,finished,bool,QString,QNLocalizedString),
                     this, QNSLOT(RemoteToLocalSynchronizationManager,onNoteThumbnailDownloadingFinished,bool,QString,QNLocalizedString));
    QThreadPool::globalInstance()->start(pDownloader);
}

QString RemoteToLocalSynchronizationManager::clientNameForProtocolVersionCheck() const
{
    QString clientName = QCoreApplication::applicationName();
    clientName += QStringLiteral("/");
    clientName += QCoreApplication::applicationVersion();
    clientName += QStringLiteral("; ");

#if defined(Q_OS_WIN32)
    OSVERSIONINFOEX info;
    ZeroMemory(&info, sizeof(OSVERSIONINFOEX));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx(&info);
    clientName += QStringLiteral("Windows/");
    clientName += QString::number(info.dwMajorVersion);
    clientName += QStringLiteral(".");
    clientName += QString::number(info.dwMinorVersion);
#elif defined(Q_OS_MAC)
    switch(QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_9:
        clientName += QStringLiteral("Mac OS/9");
        break;
    case QSysInfo::MV_10_0:
        clientName += QStringLiteral("Mac OS X/10.0");
        break;
    case QSysInfo::MV_10_1:
        clientName += QStringLiteral("Mac OS X/10.1");
        break;
    case QSysInfo::MV_10_2:
        clientName += QStringLiteral("Mac OS X/10.2");
        break;
    case QSysInfo::MV_10_3:
        clientName += QStringLiteral("Mac OS X/10.3");
        break;
    case QSysInfo::MV_10_4:
        clientName += QStringLiteral("Mac OS X/10.4");
        break;
    case QSysInfo::MV_10_5:
        clientName += QStringLiteral("Mac OS X/10.5");
        break;
    case QSysInfo::MV_10_6:
        clientName += QStringLiteral("Mac OS X/10.6");
        break;
    case QSysInfo::MV_10_7:
        clientName += QStringLiteral("Mac OS X/10.7");
        break;
    case QSysInfo::MV_10_8:
        clientName += QStringLiteral("OS X/10.8");
        break;
    case QSysInfo::MV_10_9:
        clientName += QStringLiteral("OS X/10.9");
        break;
    case QSysInfo::MV_10_10:
        clientName += QStringLiteral("OS X/10.10");
        break;
    case QSysInfo::MV_10_11:
        clientName += QStringLiteral("OS X/10.11");
        break;
    // NOTE: intentional fall-through
    case QSysInfo::MV_Unknown:
    default:
        clientName += QStringLiteral("Unknown Apple OS/unknown version");
        break;
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD) || defined(Q_OS_OPENBSD)
    utsname info;
    int res = uname(&info);
    if (Q_UNLIKELY(res != 0))
    {
        clientName += QStringLiteral("Unknown Unix/unknown version");
    }
    else
    {
        clientName += QString::fromUtf8(info.sysname);
        clientName += QStringLiteral("/");
        clientName += QString::fromUtf8(info.release);
        clientName += QStringLiteral(" ");
        clientName += QString::fromUtf8(info.version);
    }
#else
    clientName += QStringLiteral("Unknown OS/unknown version");
#endif

    return clientName;
}

QTextStream & operator<<(QTextStream & strm, const RemoteToLocalSynchronizationManager::SyncMode::type & obj)
{
    switch(obj)
    {
    case RemoteToLocalSynchronizationManager::SyncMode::FullSync:
        strm << QStringLiteral("FullSync");
        break;
    case RemoteToLocalSynchronizationManager::SyncMode::IncrementalSync:
        strm << QStringLiteral("IncrementalSync");
        break;
    default:
        strm << QStringLiteral("<unknown>");
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
                QNLocalizedString error = QT_TR_NOOP("found empty expunged tag guid in the sync chunk");
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
    if (syncChunk.notes.isSet())
    {
        container.append(syncChunk.notes.ref());

        if (!m_expungedNotes.isEmpty())
        {
            const auto & syncChunkNotes = syncChunk.notes.ref();
            auto syncChunkNotesEnd = syncChunkNotes.constEnd();
            for(auto it = syncChunkNotes.constBegin(); it != syncChunkNotesEnd; ++it)
            {
                const qevercloud::Note & note = *it;
                if (!note.guid.isSet()) {
                    continue;
                }

                QList<QString>::iterator eit = std::find(m_expungedNotes.begin(), m_expungedNotes.end(), note.guid.ref());
                if (eit != m_expungedNotes.end()) {
                    QNINFO(QStringLiteral("Note with guid ") << note.guid.ref() << QStringLiteral(" and title ")
                           << (note.title.isSet() ? note.title.ref() : QStringLiteral("<no title>"))
                           << QStringLiteral(" is present both within the sync chunk's notes and in the list of "
                                             "previously collected expunged notes. That most probably means that the note "
                                             "belongs to the shared notebook and it was moved from one shared notebook "
                                             "to another one; removing the note from the list of notes waiting to be expunged"));
                    Q_UNUSED(m_expungedNotes.erase(eit));
                }
            }
        }
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

#define SET_ITEM_TYPE_TO_ERROR() \
    errorDescription += QStringLiteral(": "); \
    errorDescription += QT_TR_NOOP("item type"); \
    errorDescription += QStringLiteral(": "); \
    errorDescription += typeName

#define SET_CANT_FIND_BY_NAME_ERROR() \
    QNLocalizedString errorDescription = QT_TR_NOOP("found item with empty name in the local storage"); \
    SET_ITEM_TYPE_TO_ERROR(); \
    QNWARNING(errorDescription << QStringLiteral(": ") << element)

#define SET_CANT_FIND_BY_GUID_ERROR() \
    QNLocalizedString errorDescription = QT_TR_NOOP("found item with empty guid in the local storage"); \
    SET_ITEM_TYPE_TO_ERROR(); \
    QNWARNING(errorDescription << QStringLiteral(": ") << element)

#define SET_EMPTY_PENDING_LIST_ERROR() \
    QNLocalizedString errorDescription = QT_TR_NOOP("detected attempt to find the element within the list " \
                                                    "of remote elements waiting for processing but that list is empty"); \
    QNWARNING(errorDescription << QStringLiteral(": ") << element)

#define SET_CANT_FIND_IN_PENDING_LIST_ERROR() \
    QNLocalizedString errorDescription = QT_TR_NOOP("can't find item within the list " \
                                                    "of remote elements waiting for processing"); \
    SET_ITEM_TYPE_TO_ERROR(); \
    QNWARNING(errorDescription << QStringLiteral(": ") << element)

template <class ContainerType, class ElementType>
typename ContainerType::iterator RemoteToLocalSynchronizationManager::findItemByName(ContainerType & container,
                                                                                     const ElementType & element,
                                                                                     const QString & typeName)
{
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::findItemByName<") << typeName << QStringLiteral(">"));

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasName()) {
        SET_CANT_FIND_BY_NAME_ERROR();
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        SET_EMPTY_PENDING_LIST_ERROR();
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
            SET_CANT_FIND_IN_PENDING_LIST_ERROR();
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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::findItemByName<") << typeName << QStringLiteral(">"));

    // Attempt to find this data element by name within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasTitle()) {
        SET_CANT_FIND_BY_NAME_ERROR();
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        SET_EMPTY_PENDING_LIST_ERROR();
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
            SET_CANT_FIND_IN_PENDING_LIST_ERROR();
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
    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::findItemByGuid<") << typeName << QStringLiteral(">"));

    // Attempt to find this data element by guid within the list of elements waiting for processing;
    // first simply try the front element from the list to avoid the costly lookup
    if (!element.hasGuid()) {
        SET_CANT_FIND_BY_GUID_ERROR();
        emit failure(errorDescription);
        return container.end();
    }

    if (container.empty()) {
        SET_EMPTY_PENDING_LIST_ERROR();
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
            SET_CANT_FIND_IN_PENDING_LIST_ERROR();
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
        const typename ContainerType::value_type & element = container[i];
        if (!element.guid.isSet()) {
            SET_CANT_FIND_BY_GUID_ERROR();
            emit failure(errorDescription);
            return;
        }

        emitFindByGuidRequest<ElementType>(element.guid.ref());
    }
}

template <class ElementType>
void RemoteToLocalSynchronizationManager::extractExpungedElementsFromSyncChunk(const qevercloud::SyncChunk & syncChunk, QList<QString> & expungedElementGuids)
{
    Q_UNUSED(syncChunk);
    Q_UNUSED(expungedElementGuids);
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
    element.setName(QObject::tr("Conflicted") + QStringLiteral(" ") + typeName + element.name() + QStringLiteral(" (") +
                    currentDateTime + QStringLiteral(")"));
    element.setDirty(true);
}

template <>
void setConflictedBase<Note>(const QString & typeName, Note & note)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

    note.setGuid(QString());
    note.setTitle(QObject::tr("Conflicted") + QStringLiteral(" ") + typeName + (note.hasTitle() ? note.title() : QStringLiteral("note")) +
                  QStringLiteral(" (") + currentDateTime + QStringLiteral(")"));
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
        QNLocalizedString errorDescription = QT_TR_NOOP("detected tag from remote storage which is "
                                                        "to be searched by name in local storage but "
                                                        "it has no name set");
        QNWARNING(errorDescription << QStringLiteral(": ") << tag);
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
        QNLocalizedString errorDescription = QT_TR_NOOP("detected saved search from remote storage which is to be "
                                                        "searched by name in local storage but it has no name set");
        QNWARNING(errorDescription << QStringLiteral(": ") << search);
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
        QNLocalizedString errorDescription = QT_TR_NOOP("detected notebook from remote storage which is to be "
                                                        "searched by name in local storage but it has no name set");
        QNWARNING(errorDescription << QStringLiteral(": ") << notebook);
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

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onFoundDuplicateByName<") << typeName << QStringLiteral(">: ")
            << typeName << QStringLiteral(" = ") << element << QStringLiteral(", requestId  = ") << requestId);

    Q_UNUSED(findElementRequestIds.erase(rit));

    typename ContainerType::iterator it = findItemByName(container, element, typeName);
    if (it == container.end()) {
        return true;
    }

    // The element exists both in the client and in the server
    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QNLocalizedString errorDescription = QT_TR_NOOP("Found item in the sync chunk without the update sequence number");
        SET_ITEM_TYPE_TO_ERROR();
        QNWARNING(errorDescription << QStringLiteral(": ") << remoteElement);
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

    QNDEBUG(QStringLiteral("onFoundDuplicateByGuid<") << typeName << QStringLiteral(">: ")
            << typeName << QStringLiteral(" = ") << element << QStringLiteral(", requestId = ") << requestId);

    typename ContainerType::iterator it = findItemByGuid(container, element, typeName);
    if (it == container.end()) {
        QNLocalizedString errorDescription = QT_TR_NOOP("could not find the remote item by guid whe reported "
                                                        "of duplicate by guid in the local storage");
        SET_ITEM_TYPE_TO_ERROR();
        QNWARNING(errorDescription << QStringLiteral(": ") << element);
        emit failure(errorDescription);
        return true;
    }

    typedef typename ContainerType::value_type RemoteElementType;
    const RemoteElementType & remoteElement = *it;
    if (!remoteElement.updateSequenceNum.isSet()) {
        QNLocalizedString errorDescription = QT_TR_NOOP("found item in remote storage without the update sequence number set");
        SET_ITEM_TYPE_TO_ERROR();
        QNWARNING(errorDescription << QStringLiteral(": ") << remoteElement);
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
                                                              const QNLocalizedString & errorDescription,
                                                              const QString & typeName, ContainerType & container,
                                                              QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onNoDuplicateByGuid<") << typeName << QStringLiteral(">: ")
            << element << QStringLiteral(", errorDescription = ") << errorDescription << QStringLiteral(", requestId = ")
            << requestId);

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
                                                              const QNLocalizedString & errorDescription,
                                                              const QString & typeName, ContainerType & container,
                                                              QSet<QUuid> & findElementRequestIds)
{
    QSet<QUuid>::iterator rit = findElementRequestIds.find(requestId);
    if (rit == findElementRequestIds.end()) {
        return false;
    }

    QNDEBUG(QStringLiteral("RemoteToLocalSynchronizationManager::onNoDuplicateByUniqueKey<") << typeName << QStringLiteral(">: ")
            << element << QStringLiteral(", errorDescription = ") << errorDescription << QStringLiteral(", requestId = ")
            << requestId);

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
    Q_UNUSED(sourceElement);
    Q_UNUSED(targetElement);
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
            unsetLocalUid(elementToUpdate);

            // NOTE: workarounding the stupidity of MSVC 2013
            emitUpdateRequest<ElementType>(elementToUpdate, static_cast<const ElementType*>(Q_NULLPTR));
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
