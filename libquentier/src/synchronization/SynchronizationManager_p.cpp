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

#include "SynchronizationManager_p.h"
#include <quentier/utility/Utility.h>
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/Printable.h>
#include <QApplication>

#define EXPIRATION_TIMESTAMP_KEY "ExpirationTimestamp"
#define LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX "LinkedNotebookExpirationTimestamp_"
#define LINKED_NOTEBOOK_AUTH_TOKEN_KEY_PART "_LinkedNotebookAuthToken_"
#define READ_LINKED_NOTEBOOK_AUTH_TOKEN_JOB "readLinkedNotebookAuthToken"
#define READ_LINKED_NOTEBOOK_SHARD_ID_JOB "readLinkedNotebookShardId"
#define WRITE_LINKED_NOTEBOOK_AUTH_TOKEN_JOB "writeLinkedNotebookAuthToken"
#define WRITE_LINKED_NOTEBOOK_SHARD_ID_JOB "writeLinkedNotebookShardId"
#define NOTE_STORE_URL_KEY "NoteStoreUrl"
#define USER_ID_KEY "UserId"
#define WEB_API_URL_PREFIX_KEY "WebApiUrlPrefix"

#define LAST_SYNC_PARAMS_KEY_GROUP "last_sync_params"
#define LAST_SYNC_UPDATE_COUNT_KEY "last_sync_update_count"
#define LAST_SYNC_TIME_KEY         "last_sync_time"
#define LAST_SYNC_LINKED_NOTEBOOKS_PARAMS "last_sync_linked_notebooks_params"
#define LINKED_NOTEBOOK_GUID_KEY "linked_notebook_guid"
#define LINKED_NOTEBOOK_LAST_UPDATE_COUNT_KEY "linked_notebook_last_update_count"
#define LINKED_NOTEBOOK_LAST_SYNC_TIME_KEY "linked_notebook_last_sync_time"

namespace quentier {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(const QString & consumerKey, const QString & consumerSecret,
                                                             LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    m_consumerKey(consumerKey),
    m_consumerSecret(consumerSecret),
    m_maxSyncChunkEntries(50),
    m_lastUpdateCount(-1),
    m_lastSyncTime(-1),
    m_cachedLinkedNotebookLastUpdateCountByGuid(),
    m_cachedLinkedNotebookLastSyncTimeByGuid(),
    m_onceReadLastSyncParams(false),
    m_noteStore(QSharedPointer<qevercloud::NoteStore>(new qevercloud::NoteStore)),
    m_authContext(AuthContext::Blank),
    m_launchSyncPostponeTimerId(-1),
    m_OAuthWebView(),
    m_OAuthResult(),
    m_remoteToLocalSyncManager(localStorageManagerThreadWorker, m_noteStore.getQecNoteStore()),
    m_sendLocalChangesManager(localStorageManagerThreadWorker, m_noteStore.getQecNoteStore()),
    m_linkedNotebookGuidsAndShareKeysWaitingForAuth(),
    m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid(),
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid(),
    m_authenticateToLinkedNotebooksPostponeTimerId(-1),
    m_receivedRequestToAuthenticateToLinkedNotebooks(false),
    m_readAuthTokenJob(QApplication::applicationName() + "_read_auth_token"),
    m_readShardIdJob(QApplication::applicationName() + "_read_shard_id"),
    m_readingAuthToken(false),
    m_readingShardId(false),
    m_writeAuthTokenJob(QApplication::applicationName() + "_write_auth_token"),
    m_writeShardIdJob(QApplication::applicationName() + "_write_shard_id"),
    m_writingAuthToken(false),
    m_writingShardId(false),
    m_readLinkedNotebookAuthTokenJobsByGuid(),
    m_readLinkedNotebookShardIdJobsByGuid(),
    m_writeLinkedNotebookAuthTokenJobsByGuid(),
    m_writeLinkedNotebookShardIdJobsByGuid(),
    m_linkedNotebookGuidsWithoutLocalAuthData(),
    m_shouldRepeatIncrementalSyncAfterSendingChanges(false),
    m_paused(false),
    m_remoteToLocalSyncWasActiveOnLastPause(false)
{
    m_readAuthTokenJob.setAutoDelete(false);
    m_readAuthTokenJob.setKey(QApplication::applicationName() + "_auth_token");

    m_readShardIdJob.setAutoDelete(false);
    m_readShardIdJob.setKey(QApplication::applicationName() + "_shard_id");

    m_writeAuthTokenJob.setAutoDelete(false);
    m_writeShardIdJob.setAutoDelete(false);

    createConnections();
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{}

bool SynchronizationManagerPrivate::active() const
{
    return m_remoteToLocalSyncManager.active() || m_sendLocalChangesManager.active();
}

bool SynchronizationManagerPrivate::paused() const
{
    return m_paused;
}

void SynchronizationManagerPrivate::synchronize()
{
    clear();
    authenticate(AuthContext::SyncLaunch);
}

void SynchronizationManagerPrivate::pause()
{
    QNDEBUG("SynchronizationManagerPrivate::pause");

    if (m_remoteToLocalSyncManager.active()) {
        m_paused = true;
        m_remoteToLocalSyncWasActiveOnLastPause = true;
        emit pauseRemoteToLocalSync();
    }

    if (m_sendLocalChangesManager.active()) {
        m_paused = true;
        m_remoteToLocalSyncWasActiveOnLastPause = false;
        emit pauseSendingLocalChanges();
    }
}

void SynchronizationManagerPrivate::resume()
{
    QNDEBUG("SynchronizationManagerPrivate::resume");

    if (!m_paused) {
        QNINFO("Wasn't paused; not doing anything on attempt to resume");
        return;
    }

    m_paused = false;

    if (m_remoteToLocalSyncWasActiveOnLastPause) {
        m_remoteToLocalSyncManager.resume();
    }
    else {
        m_sendLocalChangesManager.resume();
    }
}

void SynchronizationManagerPrivate::stop()
{
    QNDEBUG("SynchronizationManagerPrivate::stop");

    emit stopRemoteToLocalSync();
    emit stopSendingLocalChanges();
}

void SynchronizationManagerPrivate::onOAuthResult(bool result)
{
    if (result) {
        onOAuthSuccess();
    }
    else {
        onOAuthFailure();
    }
}

void SynchronizationManagerPrivate::onOAuthSuccess()
{
    m_OAuthResult = m_OAuthWebView.oauthResult();

    m_noteStore.setNoteStoreUrl(m_OAuthResult.noteStoreUrl);
    m_noteStore.setAuthenticationToken(m_OAuthResult.authenticationToken);

    launchStoreOAuthResult();
}

void SynchronizationManagerPrivate::onOAuthFailure()
{
    QNLocalizedString error = QT_TR_NOOP("OAuth failed");
    QString oauthError = m_OAuthWebView.oauthError();
    if (!oauthError.isEmpty()) {
        error += ": ";
        error += oauthError;
    }

    emit notifyError(error);
}

void SynchronizationManagerPrivate::onKeychainJobFinished(QKeychain::Job * job)
{
    if (!job) {
        QNLocalizedString error = QT_TR_NOOP("qtkeychain error: null pointer to keychain job on finish");
        emit notifyError(error);
        return;
    }

    if (job == &m_readAuthTokenJob)
    {
        onReadAuthTokenFinished();
    }
    else if (job == &m_readShardIdJob)
    {
        onReadShardIdFinished();
    }
    else if (job == &m_writeAuthTokenJob)
    {
        onWriteAuthTokenFinished();
    }
    else if (job == &m_writeShardIdJob)
    {
        onWriteShardIdFinished();
    }
    else
    {
        typedef QHash<QString,QSharedPointer<QKeychain::WritePasswordJob> >::iterator WriteJobIter;
        WriteJobIter writeLinkedNotebookAuthTokenJobsEnd = m_writeLinkedNotebookAuthTokenJobsByGuid.end();
        for(WriteJobIter it = m_writeLinkedNotebookAuthTokenJobsByGuid.begin();
            it != writeLinkedNotebookAuthTokenJobsEnd; ++it)
        {
            const auto & cachedJob = it.value();
            if (cachedJob.data() == job)
            {
                if (job->error() != QKeychain::NoError) {
                    QNLocalizedString error = QT_TR_NOOP("error saving linked notebook's authentication token to the keychain");
                    error += ": ";
                    error += ToString(job->error());
                    error += ": ";
                    error += job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);
                }

                Q_UNUSED(m_writeLinkedNotebookAuthTokenJobsByGuid.erase(it))
                return;
            }
        }

        WriteJobIter writeLinkedNotebookShardIdJobsEnd = m_writeLinkedNotebookShardIdJobsByGuid.end();
        for(WriteJobIter it = m_writeLinkedNotebookShardIdJobsByGuid.begin();
            it != writeLinkedNotebookShardIdJobsEnd; ++it)
        {
            const auto & cachedJob = it.value();
            if (cachedJob.data() == job)
            {
                if (job->error() != QKeychain::NoError) {
                    QNLocalizedString error = QT_TR_NOOP("error saving linked notebook's shard id to the keychain");
                    error += ": ";
                    error += ToString(job->error());
                    error += ": ";
                    error += job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);
                }

                Q_UNUSED(m_writeLinkedNotebookShardIdJobsByGuid.erase(it))
                return;
            }

        }

        typedef QHash<QString,QSharedPointer<QKeychain::ReadPasswordJob> >::iterator ReadJobIter;
        ReadJobIter readLinkedNotebookAuthTokenJobsEnd = m_readLinkedNotebookAuthTokenJobsByGuid.end();
        for(ReadJobIter it = m_readLinkedNotebookAuthTokenJobsByGuid.begin();
            it != readLinkedNotebookAuthTokenJobsEnd; ++it)
        {
            const auto & cachedJob = it.value();
            if (cachedJob.data() == job)
            {
                if (job->error() == QKeychain::NoError)
                {
                    QNDEBUG("Successfully read the authentication token for linked notebook from the keychain: "
                            "linked notebook guid: " << it.key());
                    m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid[it.key()].first = cachedJob->textData();
                }
                else if (job->error() == QKeychain::EntryNotFound)
                {
                    QNDEBUG("Could not find authentication token for linked notebook in the keychain: "
                            "linked notebook guid: " << it.key());
                    Q_UNUSED(m_linkedNotebookGuidsWithoutLocalAuthData.insert(it.key()))
                }
                else
                {
                    QNLocalizedString error = QT_TR_NOOP("error reading linked notebook's authentication token from the keychain");
                    error += ": ";
                    error += ToString(job->error());
                    error += ": ";
                    error += job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);

                    // Try to recover by making user to authenticate again in the blind hope that
                    // the next time the persistence of auth settings in the keychain would work
                    Q_UNUSED(m_linkedNotebookGuidsWithoutLocalAuthData.insert(it.key()))
                }

                authenticateToLinkedNotebooks();
                Q_UNUSED(m_readLinkedNotebookAuthTokenJobsByGuid.erase(it))
                return;
            }
        }

        ReadJobIter readLinkedNotebookShardIdJobsEnd = m_readLinkedNotebookShardIdJobsByGuid.end();
        for(ReadJobIter it = m_readLinkedNotebookShardIdJobsByGuid.begin();
            it != readLinkedNotebookShardIdJobsEnd; ++it)
        {
            const auto & cachedJob = it.value();
            if (cachedJob.data() == job)
            {
                if (job->error() == QKeychain::NoError)
                {
                    QNDEBUG("Successfully read the shard id for linked notebook from the keychain: "
                            "linked notebook guid: " << it.key());
                    m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid[it.key()].second = cachedJob->textData();
                }
                else if (job->error() == QKeychain::EntryNotFound)
                {
                    QNDEBUG("Could not find shard id for linked notebook in the keychain: "
                            "linked notebook guid: " << it.key());
                    Q_UNUSED(m_linkedNotebookGuidsWithoutLocalAuthData.insert(it.key()))
                }
                else
                {
                    QNLocalizedString error = QT_TR_NOOP("error reading linked notebook's shard id from the keychain");
                    error += ": ";
                    error += ToString(job->error());
                    error += ": ";
                    error += job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);

                    // Try to recover by making user to authenticate again in the blind hope that
                    // the next time the persistence of auth settings in the keychain would work
                    Q_UNUSED(m_linkedNotebookGuidsWithoutLocalAuthData.insert(it.key()))
                }

                authenticateToLinkedNotebooks();
                Q_UNUSED(m_readLinkedNotebookShardIdJobsByGuid.erase(it))
                return;
            }
        }

        QNLocalizedString error = QT_TR_NOOP("unknown keychain job finished event");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
}

void SynchronizationManagerPrivate::onRequestAuthenticationToken()
{
    QNDEBUG("SynchronizationManagerPrivate::onRequestAuthenticationToken");

    if (validAuthentication()) {
        QNDEBUG("Found valid auth token and shard id, returning them");
        emit sendAuthenticationTokenAndShardId(m_OAuthResult.authenticationToken, m_OAuthResult.shardId, m_OAuthResult.expires);
        return;
    }

    authenticate(AuthContext::SyncLaunch);
}

void SynchronizationManagerPrivate::onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString, QString> > linkedNotebookGuidsAndShareKeys)
{
    QNDEBUG("SynchronizationManagerPrivate::onRequestAuthenticationTokensForLinkedNotebooks");

    m_receivedRequestToAuthenticateToLinkedNotebooks = true;
    m_linkedNotebookGuidsAndShareKeysWaitingForAuth = linkedNotebookGuidsAndShareKeys;

    authenticateToLinkedNotebooks();
}

void SynchronizationManagerPrivate::onRequestLastSyncParameters()
{
    if (m_onceReadLastSyncParams) {
        emit sendLastSyncParameters(m_lastUpdateCount, m_lastSyncTime, m_cachedLinkedNotebookLastUpdateCountByGuid,
                                    m_cachedLinkedNotebookLastSyncTimeByGuid);
        return;
    }

    readLastSyncParameters();

    emit sendLastSyncParameters(m_lastUpdateCount, m_lastSyncTime, m_cachedLinkedNotebookLastUpdateCountByGuid,
                                m_cachedLinkedNotebookLastSyncTimeByGuid);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncFinished(qint32 lastUpdateCount, qevercloud::Timestamp lastSyncTime,
                                                                QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid,
                                                                QHash<QString,qevercloud::Timestamp> lastSyncTimeByLinkedNotebookGuid)
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncFinished: lastUpdateCount = " << lastUpdateCount
            << ", lastSyncTime = " << lastSyncTime << ", last update count per linked notebook = "
            << lastUpdateCountByLinkedNotebookGuid << "\nlastSyncTimeByLinkedNotebookGuid = "
            << lastSyncTimeByLinkedNotebookGuid);

    m_lastUpdateCount = lastUpdateCount;
    m_lastSyncTime = lastSyncTime;
    m_cachedLinkedNotebookLastUpdateCountByGuid = lastUpdateCountByLinkedNotebookGuid;
    m_cachedLinkedNotebookLastSyncTimeByGuid = lastSyncTimeByLinkedNotebookGuid;

    updatePersistentSyncSettings();

    m_onceReadLastSyncParams = true;
    emit notifyRemoteToLocalSyncDone();

    sendChanges();
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncPaused(bool pendingAuthenticaton)
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncPaused: pending authentication = "
            << (pendingAuthenticaton ? "true" : "false"));
    emit remoteToLocalSyncPaused(pendingAuthenticaton);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncStopped()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncStopped");
    emit remoteToLocalSyncStopped();
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncChunksDownloaded()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncChunksDownloaded");
    emit progress(QT_TR_NOOP("Downloaded synch user account's sync chunks"), 0.125);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncFullNotesContentDownloaded()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncFullNotesContentDownloaded");
    emit progress(QT_TR_NOOP("Downloaded full content of notes from user's account"), 0.25);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncExpungedFromServerToClient()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncExpungedFromServerToClient");
    emit progress(QT_TR_NOOP("Expunged local items which were also expunged in the remote service"), 0.375);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncLinkedNotebooksSyncChunksDownloaded()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncLinkedNotebooksSyncChunksDownloaded");
    emit progress(QT_TR_NOOP("Downloaded sync chunks from linked notebooks"), 0.5);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncLinkedNotebooksFullNotesContentDownloaded()
{
    QNDEBUG("SynchronizationManagerPrivate::onRemoteToLocalSyncLinkedNotebooksFullNotesContentDownloaded");
    emit progress(QT_TR_NOOP("Downloaded the full content of notes from linked notebooks"), 0.625);
}

void SynchronizationManagerPrivate::onShouldRepeatIncrementalSync()
{
    QNDEBUG("SynchronizationManagerPrivate::onShouldRepeatIncrementalSync");

    m_shouldRepeatIncrementalSyncAfterSendingChanges = true;
    emit willRepeatRemoteToLocalSyncAfterSendingChanges();
}

void SynchronizationManagerPrivate::onConflictDetectedDuringLocalChangesSending()
{
    QNDEBUG("SynchronizationManagerPrivate::onConflictDetectedDuringLocalChangesSending");

    emit detectedConflictDuringLocalChangesSending();

    m_sendLocalChangesManager.stop();

    // NOTE: the detection of non-synchronized state with respect to remote service often precedes the actual conflict detection;
    // need to drop this flag to prevent launching the incremental sync after sending the local changes after the incremental sync
    // which we'd launch now
    m_shouldRepeatIncrementalSyncAfterSendingChanges = false;

    launchIncrementalSync();
}

void SynchronizationManagerPrivate::onLocalChangesSent(qint32 lastUpdateCount, QHash<QString,qint32> lastUpdateCountByLinkedNotebookGuid)
{
    QNDEBUG("SynchronizationManagerPrivate::onLocalChangesSent: last update count = " << lastUpdateCount
            << ", last update count per linked notebook guid: " << lastUpdateCountByLinkedNotebookGuid);

    m_lastUpdateCount = lastUpdateCount;
    m_cachedLinkedNotebookLastUpdateCountByGuid = lastUpdateCountByLinkedNotebookGuid;

    updatePersistentSyncSettings();

    if (m_shouldRepeatIncrementalSyncAfterSendingChanges) {
        QNDEBUG("Repeating the incremental sync after sending the changes");
        m_shouldRepeatIncrementalSyncAfterSendingChanges = false;
        launchIncrementalSync();
        return;
    }

    QNINFO("Finished the whole synchronization procedure!");
    emit notifyFinish();
}

void SynchronizationManagerPrivate::onSendLocalChangesPaused(bool pendingAuthenticaton)
{
    QNDEBUG("SynchronizationManagerPrivate::onSendLocalChangesPaused: pending authentication = "
            << (pendingAuthenticaton ? "true" : "false"));
    emit sendLocalChangesPaused(pendingAuthenticaton);
}

void SynchronizationManagerPrivate::onSendLocalChangesStopped()
{
    QNDEBUG("SynchronizationManagerPrivate::onSendLocalChangesStopped");
    emit sendLocalChangesStopped();
}

void SynchronizationManagerPrivate::onSendingLocalChangesReceivedUsersDirtyObjects()
{
    QNDEBUG("SynchronizationManagerPrivate::onSendingLocalChangesReceivedUsersDirtyObjects");
    emit progress(QT_TR_NOOP("Prepared new and modified data from user's account for sending back to the remote service"), 0.75);
}

void SynchronizationManagerPrivate::onSendingLocalChangesReceivedAllDirtyObjects()
{
    QNDEBUG("SynchronizationManagerPrivate::onSendingLocalChangesReceivedAllDirtyObjects");
    emit progress(QT_TR_NOOP("Prepared new and modified data from linked notebooks for sending back to the remote service"), 0.875);
}

void SynchronizationManagerPrivate::onRateLimitExceeded(qint32 secondsToWait)
{
    QNDEBUG("SynchronizationManagerPrivate::onRateLimitExceeded");
    emit rateLimitExceeded(secondsToWait);
}

void SynchronizationManagerPrivate::createConnections()
{
    // Connections with OAuth handler
    QObject::connect(&m_OAuthWebView, QNSIGNAL(qevercloud::EvernoteOAuthWebView,authenticationFinished,bool),
                     this, QNSLOT(SynchronizationManagerPrivate,onOAuthResult,bool));
    QObject::connect(&m_OAuthWebView, QNSIGNAL(qevercloud::EvernoteOAuthWebView,authenticationSuceeded),
                     this, QNSLOT(SynchronizationManagerPrivate,onOAuthSuccess));
    QObject::connect(&m_OAuthWebView, QNSIGNAL(qevercloud::EvernoteOAuthWebView,authenticationFailed),
                     this, QNSLOT(SynchronizationManagerPrivate,onOAuthFailure));

    // Connections with remote to local synchronization manager
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,failure,QNLocalizedString),
                     this, QNSIGNAL(SynchronizationManagerPrivate,notifyError,QNLocalizedString));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,finished,qint32,qevercloud::Timestamp,QHash<QString,qint32>,
                                                           QHash<QString,qevercloud::Timestamp>),
                     this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncFinished,qint32,qevercloud::Timestamp,QHash<QString,qint32>,
                                  QHash<QString,qevercloud::Timestamp>));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,rateLimitExceeded,qint32),
                     this, QNSLOT(SynchronizationManagerPrivate,onRateLimitExceeded,qint32));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,requestAuthenticationToken),
                     this, QNSLOT(SynchronizationManagerPrivate,onRequestAuthenticationToken));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,requestAuthenticationTokensForLinkedNotebooks,QList<QPair<QString,QString> >),
                     this, QNSLOT(SynchronizationManagerPrivate,onRequestAuthenticationTokensForLinkedNotebooks,QList<QPair<QString,QString> >));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,paused,bool), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncPaused,bool));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,stopped), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncStopped));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,requestLastSyncParameters), this, QNSLOT(SynchronizationManagerPrivate,onRequestLastSyncParameters));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,syncChunksDownloaded), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncChunksDownloaded));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,fullNotesContentsDownloaded), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncFullNotesContentDownloaded));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,expungedFromServerToClient), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncExpungedFromServerToClient));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,linkedNotebooksSyncChunksDownloaded), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncLinkedNotebooksSyncChunksDownloaded));
    QObject::connect(&m_remoteToLocalSyncManager, QNSIGNAL(RemoteToLocalSynchronizationManager,linkedNotebooksFullNotesContentsDownloaded), this, QNSLOT(SynchronizationManagerPrivate,onRemoteToLocalSyncLinkedNotebooksFullNotesContentDownloaded));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,pauseRemoteToLocalSync), &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,pause));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,resumeRemoteToLocalSync), &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,resume));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,stopRemoteToLocalSync), &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,stop));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,sendAuthenticationTokenAndShardId,QString,QString,qevercloud::Timestamp),
                     &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,onAuthenticationTokenReceived,QString,QString,qevercloud::Timestamp));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,sendAuthenticationTokensForLinkedNotebooks,QHash<QString,QPair<QString,QString> >,QHash<QString,qevercloud::Timestamp>),
                     &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,onAuthenticationTokensForLinkedNotebooksReceived,QHash<QString,QPair<QString,QString> >,QHash<QString,qevercloud::Timestamp>));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,sendLastSyncParameters,qint32,qevercloud::Timestamp,QHash<QString,qint32>,QHash<QString,qevercloud::Timestamp>),
                     &m_remoteToLocalSyncManager, QNSLOT(RemoteToLocalSynchronizationManager,onLastSyncParametersReceived,qint32,qevercloud::Timestamp,QHash<QString,qint32>,QHash<QString,qevercloud::Timestamp>));

    // Connections with send local changes manager
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,failure,QNLocalizedString), this, QNSIGNAL(SynchronizationManagerPrivate,notifyError,QNLocalizedString));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,finished,qint32,QHash<QString,qint32>),
                     this, QNSLOT(SynchronizationManagerPrivate,onLocalChangesSent,qint32,QHash<QString,qint32>));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,rateLimitExceeded,qint32),
                     this, QNSLOT(SynchronizationManagerPrivate,onRateLimitExceeded,qint32));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,requestAuthenticationToken),
                     this, QNSLOT(SynchronizationManagerPrivate,onRequestAuthenticationToken));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,requestAuthenticationTokensForLinkedNotebooks,QList<QPair<QString,QString> >),
                     this, QNSLOT(SynchronizationManagerPrivate,onRequestAuthenticationTokensForLinkedNotebooks,QList<QPair<QString,QString> >));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,shouldRepeatIncrementalSync), this, QNSLOT(SynchronizationManagerPrivate,onShouldRepeatIncrementalSync));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,conflictDetected), this, QNSLOT(SynchronizationManagerPrivate,onConflictDetectedDuringLocalChangesSending));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,paused,bool), this, QNSLOT(SynchronizationManagerPrivate,onSendLocalChangesPaused,bool));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,stopped), this, QNSLOT(SynchronizationManagerPrivate,onSendLocalChangesStopped));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,receivedUserAccountDirtyObjects), this, QNSLOT(SynchronizationManagerPrivate,onSendingLocalChangesReceivedUsersDirtyObjects));
    QObject::connect(&m_sendLocalChangesManager, QNSIGNAL(SendLocalChangesManager,receivedAllDirtyObjects), this, QNSLOT(SynchronizationManagerPrivate,onSendingLocalChangesReceivedAllDirtyObjects));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,sendAuthenticationTokensForLinkedNotebooks,QHash<QString,QPair<QString,QString> >,QHash<QString,qevercloud::Timestamp>),
                     &m_sendLocalChangesManager, QNSLOT(SendLocalChangesManager,onAuthenticationTokensForLinkedNotebooksReceived,QHash<QString,QPair<QString,QString> >,QHash<QString,qevercloud::Timestamp>));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,pauseSendingLocalChanges), &m_sendLocalChangesManager, QNSLOT(SendLocalChangesManager,pause));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,resumeSendingLocalChanges), &m_sendLocalChangesManager, QNSLOT(SendLocalChangesManager,resume));
    QObject::connect(this, QNSIGNAL(SynchronizationManagerPrivate,stopSendingLocalChanges), &m_sendLocalChangesManager, QNSLOT(SendLocalChangesManager,stop));

    // Connections with read/write password jobs
    QObject::connect(&m_readAuthTokenJob, QNSIGNAL(QKeychain::ReadPasswordJob,finished,QKeychain::Job*), this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
    QObject::connect(&m_readShardIdJob, QNSIGNAL(QKeychain::ReadPasswordJob,finished,QKeychain::Job*), this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
    QObject::connect(&m_writeAuthTokenJob, QNSIGNAL(QKeychain::WritePasswordJob,finished,QKeychain::Job*), this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
    QObject::connect(&m_writeShardIdJob, QNSIGNAL(QKeychain::WritePasswordJob,finished,QKeychain::Job*), this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
}

void SynchronizationManagerPrivate::readLastSyncParameters()
{
    QNDEBUG("SynchronizationManagerPrivate::readLastSyncParameters");

    m_lastSyncTime = 0;
    m_lastUpdateCount = 0;
    m_cachedLinkedNotebookLastUpdateCountByGuid.clear();
    m_cachedLinkedNotebookLastSyncTimeByGuid.clear();

    ApplicationSettings appSettings;

    const QString keyGroup = QString(LAST_SYNC_PARAMS_KEY_GROUP) + "/";
    QVariant lastUpdateCountVar = appSettings.value(keyGroup + LAST_SYNC_UPDATE_COUNT_KEY);
    if (!lastUpdateCountVar.isNull())
    {
        bool conversionResult = false;
        m_lastUpdateCount = lastUpdateCountVar.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read last update count from persistent application settings");
            m_lastUpdateCount = 0;
        }
    }

    QVariant lastSyncTimeVar = appSettings.value(keyGroup + LAST_SYNC_TIME_KEY);
    if (!lastUpdateCountVar.isNull())
    {
        bool conversionResult = false;
        m_lastSyncTime = lastSyncTimeVar.toLongLong(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read last sync time from persistent application settings");
            m_lastSyncTime = 0;
        }
    }

    int numLinkedNotebooksSyncParams = appSettings.beginReadArray(LAST_SYNC_LINKED_NOTEBOOKS_PARAMS);
    for(int i = 0; i < numLinkedNotebooksSyncParams; ++i)
    {
        appSettings.setArrayIndex(i);

        QString guid = appSettings.value(LINKED_NOTEBOOK_GUID_KEY).toString();
        if (guid.isEmpty()) {
            QNWARNING("Couldn't read linked notebook's guid from persistent application settings");
            continue;
        }

        QVariant lastUpdateCountVar = appSettings.value(LINKED_NOTEBOOK_LAST_UPDATE_COUNT_KEY);
        bool conversionResult = false;
        qint32 lastUpdateCount = lastUpdateCountVar.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read linked notebook's last update count from persistent application settings");
            continue;
        }

        QVariant lastSyncTimeVar = appSettings.value(LINKED_NOTEBOOK_LAST_SYNC_TIME_KEY);
        conversionResult = false;
        qevercloud::Timestamp lastSyncTime = lastSyncTimeVar.toLongLong(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read linked notebook's last sync time from persistent application settings");
            continue;
        }

        m_cachedLinkedNotebookLastUpdateCountByGuid[guid] = lastUpdateCount;
        m_cachedLinkedNotebookLastSyncTimeByGuid[guid] = lastSyncTime;
    }
    appSettings.endArray();

    m_onceReadLastSyncParams = true;
}

void SynchronizationManagerPrivate::authenticate(const AuthContext::type authContext)
{
    QNDEBUG("SynchronizationManagerPrivate::authenticate: auth context = " << authContext);
    m_authContext = authContext;

    if (validAuthentication()) {
        QNDEBUG("Found already valid authentication info");
        finalizeAuthentication();
        return;
    }

    QNTRACE("Trying to restore persistent authentication settings...");

    ApplicationSettings appSettings;
    QString keyGroup = "Authentication/";

    QVariant tokenExpirationValue = appSettings.value(keyGroup + EXPIRATION_TIMESTAMP_KEY);
    if (tokenExpirationValue.isNull()) {
        QNINFO("Authentication token expiration timestamp was not found within application settings, "
               "assuming it has never been written & launching the OAuth procedure");
        launchOAuth();
        return;
    }

    bool conversionResult = false;
    qevercloud::Timestamp tokenExpirationTimestamp = tokenExpirationValue.toLongLong(&conversionResult);
    if (!conversionResult) {
        QNLocalizedString error = QT_TR_NOOP("failed to convert QVariant with authentication token expiration timestamp "
                                             "to actual timestamp");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (checkIfTimestampIsAboutToExpireSoon(tokenExpirationTimestamp)) {
        QNINFO("Authentication token stored in persistent application settings is about to expire soon enough, "
               "launching the OAuth procedure");
        launchOAuth();
        return;
    }

    m_OAuthResult.expires = tokenExpirationTimestamp;

    QNTRACE("Restoring persistent note store url");

    QVariant noteStoreUrlValue = appSettings.value(keyGroup + NOTE_STORE_URL_KEY);
    if (noteStoreUrlValue.isNull()) {
        QNLocalizedString error = QT_TR_NOOP("can't find note store url within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString noteStoreUrl = noteStoreUrlValue.toString();
    if (noteStoreUrl.isEmpty()) {
        QNLocalizedString error = QT_TR_NOOP("can't convert note store url from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_OAuthResult.noteStoreUrl = noteStoreUrl;

    QNDEBUG("Restoring persistent user id");

    QVariant userIdValue = appSettings.value(keyGroup + USER_ID_KEY);
    if (userIdValue.isNull()) {
        QNLocalizedString error = QT_TR_NOOP("can't find user id within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    conversionResult = false;
    qevercloud::UserID userId = userIdValue.toInt(&conversionResult);
    if (!conversionResult) {
        QNLocalizedString error = QT_TR_NOOP("can't convert user id from QVariant to qint32");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_OAuthResult.userId = userId;

    QNDEBUG("Restoring persistent web api url prefix");

    QVariant webApiUrlPrefixValue = appSettings.value(keyGroup + WEB_API_URL_PREFIX_KEY);
    if (webApiUrlPrefixValue.isNull()) {
        QNLocalizedString error = QT_TR_NOOP("can't find web API url prefix within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString webApiUrlPrefix = webApiUrlPrefixValue.toString();
    if (webApiUrlPrefix.isEmpty()) {
        QNLocalizedString error = QT_TR_NOOP("can't convert web api url prefix from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_OAuthResult.webApiUrlPrefix = webApiUrlPrefix;

    QNDEBUG("Trying to restore the authentication token and the shard id from the keychain");

    m_readingAuthToken = true;
    m_readAuthTokenJob.start();

    m_readingShardId = true;
    m_readShardIdJob.start();
}

void SynchronizationManagerPrivate::launchOAuth()
{
    m_OAuthWebView.authenticate("sandbox.evernote.com", m_consumerKey, m_consumerSecret);
}

void SynchronizationManagerPrivate::launchSync()
{
    QNDEBUG("SynchronizationManagerPrivate::launchSync");

    if (!m_onceReadLastSyncParams) {
        readLastSyncParameters();
    }

    if (m_lastUpdateCount <= 0) {
        QNDEBUG("The client has never synchronized with the remote service, "
                "performing the full sync");
        launchFullSync();
        return;
    }

    QNDEBUG("Performing incremental sync");
    launchIncrementalSync();
}

void SynchronizationManagerPrivate::launchFullSync()
{
    QNDEBUG("SynchronizationManagerPrivate::launchFullSync");
    m_remoteToLocalSyncManager.start();
}

void SynchronizationManagerPrivate::launchIncrementalSync()
{
    QNDEBUG("SynchronizationManagerPrivate::launchIncrementalSync: m_lastUpdateCount = " << m_lastUpdateCount);
    m_remoteToLocalSyncManager.start(m_lastUpdateCount);
}

void SynchronizationManagerPrivate::sendChanges()
{
    QNDEBUG("SynchronizationManagerPrivate::sendChanges");
    m_sendLocalChangesManager.start(m_lastUpdateCount, m_cachedLinkedNotebookLastUpdateCountByGuid);
}

void SynchronizationManagerPrivate::launchStoreOAuthResult()
{
    m_writeAuthTokenJob.setTextData(m_OAuthResult.authenticationToken);
    m_writingAuthToken = true;
    m_writeAuthTokenJob.start();

    m_writeShardIdJob.setTextData(m_OAuthResult.shardId);
    m_writingShardId = true;
    m_writeShardIdJob.start();
}

void SynchronizationManagerPrivate::finalizeStoreOAuthResult()
{
    ApplicationSettings appSettings;
    QString keyGroup = "Authentication/";

    appSettings.setValue(keyGroup + NOTE_STORE_URL_KEY, m_OAuthResult.noteStoreUrl);
    appSettings.setValue(keyGroup + EXPIRATION_TIMESTAMP_KEY, m_OAuthResult.expires);
    appSettings.setValue(keyGroup + USER_ID_KEY, m_OAuthResult.userId);
    appSettings.setValue(keyGroup + WEB_API_URL_PREFIX_KEY, m_OAuthResult.webApiUrlPrefix);

    QNDEBUG("Successfully wrote the authentication result info to the application settings. "
            "Token expiration timestamp = " << printableDateTimeFromTimestamp(m_OAuthResult.expires)
            << ", user id = " << m_OAuthResult.userId << ", web API url prefix = "
            << m_OAuthResult.webApiUrlPrefix);

    finalizeAuthentication();
}

void SynchronizationManagerPrivate::finalizeAuthentication()
{
    QNDEBUG("SynchronizationManagerPrivate::finalizeAuthentication: result = " << m_OAuthResult);

    switch(m_authContext)
    {
    case AuthContext::Blank:
    {
        QNLocalizedString error = QT_TR_NOOP("incorrect authentication context: blank");
        emit notifyError(error);
        break;
    }
    case AuthContext::SyncLaunch:
        launchSync();
        break;
    case AuthContext::AuthToLinkedNotebooks:
        authenticateToLinkedNotebooks();
        break;
    default:
    {
        QNLocalizedString error = QT_TR_NOOP("unknown authentication context");
        error += ": ";
        error += ToString(m_authContext);
        emit notifyError(error);
        break;
    }
    }

    m_authContext = AuthContext::Blank;
}

void SynchronizationManagerPrivate::timerEvent(QTimerEvent * pTimerEvent)
{
    if (Q_UNLIKELY(!pTimerEvent)) {
        QNLocalizedString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    int timerId = pTimerEvent->timerId();
    killTimer(timerId);

    QNDEBUG("Timer event for timer id " << timerId);

    if (timerId == m_launchSyncPostponeTimerId) {
        QNDEBUG("Re-launching the sync procedure due to RATE_LIMIT_REACHED exception "
                "when trying to get the sync state the last time");
        launchSync();
        return;
    }
    else if (timerId == m_authenticateToLinkedNotebooksPostponeTimerId) {
        QNDEBUG("Re-attempting to authenticate to remaining linked (shared) notebooks");
        onRequestAuthenticationTokensForLinkedNotebooks(m_linkedNotebookGuidsAndShareKeysWaitingForAuth);
        return;
    }
}

void SynchronizationManagerPrivate::clear()
{
    m_lastUpdateCount = -1;
    m_lastSyncTime = -1;
    m_cachedLinkedNotebookLastUpdateCountByGuid.clear();
    m_cachedLinkedNotebookLastSyncTimeByGuid.clear();
    m_onceReadLastSyncParams = false;

    m_authContext = AuthContext::Blank;

    m_launchSyncPostponeTimerId = -1;

    // NOTE: don't do anything with m_OAuthWebView
    m_OAuthResult = qevercloud::EvernoteOAuthWebView::OAuthResult();

    m_remoteToLocalSyncManager.stop();
    m_sendLocalChangesManager.stop();

    m_linkedNotebookGuidsAndShareKeysWaitingForAuth.clear();
    m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid.clear();
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.clear();

    m_authenticateToLinkedNotebooksPostponeTimerId = -1;
    m_receivedRequestToAuthenticateToLinkedNotebooks = false;

    m_readLinkedNotebookAuthTokenJobsByGuid.clear();
    m_readLinkedNotebookShardIdJobsByGuid.clear();
    m_writeLinkedNotebookAuthTokenJobsByGuid.clear();
    m_writeLinkedNotebookShardIdJobsByGuid.clear();

    m_linkedNotebookGuidsWithoutLocalAuthData.clear();

    m_shouldRepeatIncrementalSyncAfterSendingChanges = false;

    m_paused = false;
    m_remoteToLocalSyncWasActiveOnLastPause = false;
}

bool SynchronizationManagerPrivate::validAuthentication() const
{
    if (m_OAuthResult.expires == static_cast<qint64>(0)) {
        // The value is not set
        return false;
    }

    return !checkIfTimestampIsAboutToExpireSoon(m_OAuthResult.expires);
}

bool SynchronizationManagerPrivate::checkIfTimestampIsAboutToExpireSoon(const qevercloud::Timestamp timestamp) const
{
    qevercloud::Timestamp currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    if (currentTimestamp - timestamp < SIX_HOURS_IN_MSEC) {
        return true;
    }

    return false;
}

void SynchronizationManagerPrivate::authenticateToLinkedNotebooks()
{
    const int numLinkedNotebooks = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.size();
    if (numLinkedNotebooks == 0) {
        QNDEBUG("No linked notebooks waiting for authentication, sending the cached auth tokens, shard ids and expiration times");
        emit sendAuthenticationTokensForLinkedNotebooks(m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid,
                                                        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid);
        return;
    }

    ApplicationSettings appSettings;
    QString keyGroup = "Authentication/";

    QHash<QString,QPair<QString,QString> > authTokensAndShardIdsToCacheByGuid;
    QHash<QString,qevercloud::Timestamp> authTokenExpirationTimestampsToCacheByGuid;

    for(auto it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.begin();
        it != m_linkedNotebookGuidsAndShareKeysWaitingForAuth.end(); )
    {
        const QPair<QString,QString> & pair = *it;

        const QString & guid     = pair.first;
        const QString & shareKey = pair.second;

        bool forceRemoteAuth = false;
        auto linkedNotebookAuthTokenIt = m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid.find(guid);
        if (linkedNotebookAuthTokenIt == m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid.end())
        {
            auto noAuthDataIt = m_linkedNotebookGuidsWithoutLocalAuthData.find(guid);
            if (noAuthDataIt != m_linkedNotebookGuidsWithoutLocalAuthData.end())
            {
                forceRemoteAuth = true;
                Q_UNUSED(m_linkedNotebookGuidsWithoutLocalAuthData.erase(noAuthDataIt))
            }
            else
            {
                QNDEBUG("Haven't found the authentication token and shard id for linked notebook guid " << guid
                        << " in the local cache, will try to read them from the keychain");

                // 1) Set up the job of reading the authentication token
                QSharedPointer<QKeychain::ReadPasswordJob> pReadAuthTokenJob;
                auto readJobIt = m_readLinkedNotebookAuthTokenJobsByGuid.find(guid);
                if (readJobIt == m_readLinkedNotebookAuthTokenJobsByGuid.end()) {
                    pReadAuthTokenJob = QSharedPointer<QKeychain::ReadPasswordJob>(new QKeychain::ReadPasswordJob(READ_LINKED_NOTEBOOK_AUTH_TOKEN_JOB));
                    readJobIt = m_readLinkedNotebookAuthTokenJobsByGuid.insert(guid, pReadAuthTokenJob);
                }
                else {
                    pReadAuthTokenJob = readJobIt.value();
                }

                pReadAuthTokenJob->setKey(guid);
                QObject::connect(pReadAuthTokenJob.data(), QNSIGNAL(QKeychain::ReadPasswordJob,finished,QKeychain::Job*),
                                 this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
                pReadAuthTokenJob->start();

                // 2) Set up the job reading the shard id
                QSharedPointer<QKeychain::ReadPasswordJob> pReadShardIdJob;
                readJobIt = m_readLinkedNotebookShardIdJobsByGuid.find(guid);
                if (readJobIt == m_readLinkedNotebookShardIdJobsByGuid.end()) {
                    pReadShardIdJob = QSharedPointer<QKeychain::ReadPasswordJob>(new QKeychain::ReadPasswordJob(READ_LINKED_NOTEBOOK_SHARD_ID_JOB));
                    readJobIt = m_readLinkedNotebookShardIdJobsByGuid.insert(guid, pReadShardIdJob);
                }
                else {
                    pReadShardIdJob = readJobIt.value();
                }

                pReadShardIdJob->setKey(guid);
                QObject::connect(pReadShardIdJob.data(), QNSIGNAL(QKeychain::ReadPasswordJob,finished,QKeychain::Job*),
                                 this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
                pReadShardIdJob->start();

                ++it;
                continue;
            }
        }

        if (!forceRemoteAuth)
        {
            auto linkedNotebookAuthTokenExpirationIt = m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.find(guid);
            if (linkedNotebookAuthTokenExpirationIt == m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.end())
            {
                QVariant expirationTimeVariant = appSettings.value(keyGroup + LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX + guid);
                if (!expirationTimeVariant.isNull())
                {
                    bool conversionResult = false;
                    qevercloud::Timestamp expirationTime = expirationTimeVariant.toLongLong(&conversionResult);
                    if (conversionResult) {
                        linkedNotebookAuthTokenExpirationIt = m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.insert(guid, expirationTime);
                    }
                    else {
                        QNWARNING("Can't convert linked notebook's authentication token's expiration time from QVariant retrieved from "
                                  "app settings into timestamp: linked notebook guid = " << guid << ", variant = "
                                  << expirationTimeVariant);
                    }
                }
            }

            if ( (linkedNotebookAuthTokenExpirationIt != m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.end()) &&
                 !checkIfTimestampIsAboutToExpireSoon(linkedNotebookAuthTokenExpirationIt.value()) )
            {
                QNDEBUG("Found authentication data for linked notebook guid " << guid << " + verified its expiration timestamp");
                it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.erase(it);
                continue;
            }
        }

        QNDEBUG("Authentication data for linked notebook guid " << guid << " was either not found in local cache "
                "(and/or app settings / keychain) or has expired, need to receive that from remote Evernote service");

        if (m_authenticateToLinkedNotebooksPostponeTimerId >= 0) {
            QNDEBUG("Authenticate to linked notebook postpone timer is active, will wait to preserve the breach "
                    "of Evernote rate API limit");
            ++it;
            continue;
        }

        if (m_authContext != AuthContext::Blank) {
            QNDEBUG("Authentication context variable is not set to blank which means that authentication must be in progress: "
                    << m_authContext << "; won't attempt to call remote Evernote API at this time");
            ++it;
            continue;
        }

        qevercloud::AuthenticationResult authResult;
        QNLocalizedString errorDescription;
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = m_noteStore.authenticateToSharedNotebook(shareKey, authResult,
                                                                    errorDescription, rateLimitSeconds);
        if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (validAuthentication()) {
                QNLocalizedString error = QT_TR_NOOP("unexpected AUTH_EXPIRED error");
                error += ": ";
                error += errorDescription;
                emit notifyError(error);
            }
            else {
                authenticate(AuthContext::AuthToLinkedNotebooks);
            }

            ++it;
            continue;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds <= 0) {
                errorDescription += "\n";
                errorDescription += QT_TR_NOOP("rate limit reached but the number of seconds to wait is incorrect");
                errorDescription += ": ";
                errorDescription += QString::number(rateLimitSeconds);
                emit notifyError(errorDescription);
                return;
            }

            m_authenticateToLinkedNotebooksPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));

            ++it;
            continue;
        }
        else if (errorCode != 0)
        {
            emit notifyError(errorDescription);
            return;
        }

        QString shardId;
        if (authResult.user.isSet() && authResult.user->shardId.isSet()) {
            shardId = authResult.user->shardId.ref();
        }

        m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid[guid] = QPair<QString, QString>(authResult.authenticationToken, shardId);
        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid[guid] = authResult.expiration;

        QPair<QString,QString> & authTokenAndShardId = authTokensAndShardIdsToCacheByGuid[guid];
        authTokenAndShardId.first = authResult.authenticationToken;
        authTokenAndShardId.second = shardId;

        authTokenExpirationTimestampsToCacheByGuid[guid] = authResult.expiration;

        it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.erase(it);
    }

    if (m_linkedNotebookGuidsAndShareKeysWaitingForAuth.empty()) {
        QNDEBUG("Retrieved authentication data for all requested linked notebooks, sending the answer now");
        emit sendAuthenticationTokensForLinkedNotebooks(m_cachedLinkedNotebookAuthTokensAndShardIdsByGuid,
                                                        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid);
    }

    // Caching linked notebook's authentication token's expiration time in app settings
    typedef QHash<QString,qevercloud::Timestamp>::const_iterator ExpirationTimeCIter;
    ExpirationTimeCIter authTokenExpirationTimesToCacheEnd = authTokenExpirationTimestampsToCacheByGuid.end();
    for(ExpirationTimeCIter it = authTokenExpirationTimestampsToCacheByGuid.begin();
        it != authTokenExpirationTimesToCacheEnd; ++it)
    {
        QString key = LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX + it.key();
        appSettings.setValue(keyGroup + key, QVariant(it.value()));
    }

    // Caching linked notebook's authentication tokens and shard ids in the keychain
    QString appName = QApplication::applicationName();

    for(auto it = authTokensAndShardIdsToCacheByGuid.begin(), end = authTokensAndShardIdsToCacheByGuid.end(); it != end; ++it)
    {
        const QString & guid = it.key();
        const QString & token = it.value().first;
        const QString & shardId = it.value().second;

        QString key = appName + LINKED_NOTEBOOK_AUTH_TOKEN_KEY_PART + guid;

        // 1) Set up the job writing the auth token to the keychain
        QSharedPointer<QKeychain::WritePasswordJob> pWriteAuthTokenJob(new QKeychain::WritePasswordJob(WRITE_LINKED_NOTEBOOK_AUTH_TOKEN_JOB));
        Q_UNUSED(m_writeLinkedNotebookAuthTokenJobsByGuid.insert(key, pWriteAuthTokenJob))
        pWriteAuthTokenJob->setKey(key);
        pWriteAuthTokenJob->setTextData(token);
        QObject::connect(pWriteAuthTokenJob.data(), QNSIGNAL(QKeychain::WritePasswordJob,finished,QKeychain::Job*),
                         this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
        pWriteAuthTokenJob->start();

        // 2) Set up the job writing the shard id to the keychain
        QSharedPointer<QKeychain::WritePasswordJob> pWriteShardIdJob(new QKeychain::WritePasswordJob(WRITE_LINKED_NOTEBOOK_SHARD_ID_JOB));
        Q_UNUSED(m_writeLinkedNotebookShardIdJobsByGuid.insert(key, pWriteShardIdJob))
        pWriteShardIdJob->setKey(key);
        pWriteShardIdJob->setTextData(shardId);
        QObject::connect(pWriteShardIdJob.data(), QNSIGNAL(QKeychain::WritePasswordJob,finished,QKeychain::Job*),
                         this, QNSLOT(SynchronizationManagerPrivate,onKeychainJobFinished,QKeychain::Job*));
        pWriteShardIdJob->start();
    }
}

void SynchronizationManagerPrivate::onReadAuthTokenFinished()
{
    m_readingAuthToken = false;

    QKeychain::Error errorCode = m_readAuthTokenJob.error();
    if (errorCode == QKeychain::EntryNotFound)
    {
        QNLocalizedString error = QT_TR_NOOP("unexpectedly missing OAuth token in the keychain");
        error += ": ";
        error += m_readAuthTokenJob.errorString();
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to read the authentication token returned with error: error code " << errorCode
                  << ", " << m_readAuthTokenJob.errorString());
        QNLocalizedString error = QT_TR_NOOP("can't read stored auth token from the keychain");
        error += ": ";
        error += m_readAuthTokenJob.errorString();
        emit notifyError(error);
        return;
    }

    QNDEBUG("Successfully restored the authentication token");
    m_OAuthResult.authenticationToken = m_readAuthTokenJob.textData();

    if (!m_readingShardId) {
        finalizeAuthentication();
    }
}

void SynchronizationManagerPrivate::onReadShardIdFinished()
{
    m_readingShardId = false;

    QKeychain::Error errorCode = m_readShardIdJob.error();
    if (errorCode == QKeychain::EntryNotFound)
    {
        QNLocalizedString error = QT_TR_NOOP("unexpectedly missing OAuth shard id in the keychain");
        error += ": ";
        error += m_readShardIdJob.errorString();
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to read the shard id returned with error: error code " << errorCode
                  << ", " << m_readShardIdJob.errorString());
        QNLocalizedString error = QT_TR_NOOP("can't read stored shard id from the keychain");
        error += ": ";
        error += m_readShardIdJob.errorString();
        emit notifyError(error);
        return;
    }

    QNDEBUG("Successfully restored the shard id");
    m_OAuthResult.shardId = m_readShardIdJob.textData();

    if (!m_readingAuthToken) {
        finalizeAuthentication();
    }
}

void SynchronizationManagerPrivate::onWriteAuthTokenFinished()
{
    m_writingAuthToken = false;

    QKeychain::Error errorCode = m_writeAuthTokenJob.error();
    if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to write the authentication token returned with error: error code " << errorCode
                  << ", " << m_writeAuthTokenJob.errorString());
        QNLocalizedString error = QT_TR_NOOP("Can't write oauth token to the keychain");
        error += ": ";
        error += m_writeAuthTokenJob.errorString();
        emit notifyError(error);
        return;
    }

    QNDEBUG("Successfully stored the authentication token in the keychain");

    if (!m_writingShardId) {
        finalizeStoreOAuthResult();
    }
}

void SynchronizationManagerPrivate::onWriteShardIdFinished()
{
    m_writingShardId = false;

    QKeychain::Error errorCode = m_writeShardIdJob.error();
    if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to write the shard id returned with error: error code " << errorCode
                  << ", " << m_writeShardIdJob.errorString());
        QNLocalizedString error = QT_TR_NOOP("Can't write oauth shard id to the keychain");
        error += ": ";
        error += m_writeShardIdJob.errorString();
        emit notifyError(error);
        return;
    }

    QNDEBUG("Successfully stored the shard id in the keychain");

    if (!m_writingAuthToken) {
        finalizeStoreOAuthResult();
    }
}

void SynchronizationManagerPrivate::updatePersistentSyncSettings()
{
    QNDEBUG("SynchronizationManagerPrivate::updatePersistentSyncSettings");

    ApplicationSettings appSettings;

    const QString keyGroup = QString(LAST_SYNC_PARAMS_KEY_GROUP) + "/";
    appSettings.setValue(keyGroup + LAST_SYNC_UPDATE_COUNT_KEY, m_lastUpdateCount);
    appSettings.setValue(keyGroup + LAST_SYNC_TIME_KEY, m_lastSyncTime);

    int numLinkedNotebooksSyncParams = m_cachedLinkedNotebookLastUpdateCountByGuid.size();
    appSettings.beginWriteArray(LAST_SYNC_LINKED_NOTEBOOKS_PARAMS, numLinkedNotebooksSyncParams);

    int counter = 0;
    auto updateCountEnd = m_cachedLinkedNotebookLastUpdateCountByGuid.end();
    auto syncTimeEnd = m_cachedLinkedNotebookLastSyncTimeByGuid.end();
    for(auto updateCountIt = m_cachedLinkedNotebookLastUpdateCountByGuid.begin(); updateCountIt != updateCountEnd; ++updateCountIt)
    {
        const QString & guid = updateCountIt.key();
        auto syncTimeIt = m_cachedLinkedNotebookLastSyncTimeByGuid.find(guid);
        if (syncTimeIt == syncTimeEnd) {
            QNWARNING("Detected inconsistent last sync parameters for one of linked notebooks: last update count is present "
                      "while last sync time is not, skipping writing the persistent settings entry for this linked notebook");
            continue;
        }

        appSettings.setArrayIndex(counter);
        appSettings.setValue(LINKED_NOTEBOOK_LAST_UPDATE_COUNT_KEY, updateCountIt.value());
        appSettings.setValue(LINKED_NOTEBOOK_LAST_SYNC_TIME_KEY, syncTimeIt.value());

        ++counter;
    }

    appSettings.endArray();

    QNTRACE("Wrote " << counter << " last sync params entries for linked notebooks");
}

} // namespace quentier
