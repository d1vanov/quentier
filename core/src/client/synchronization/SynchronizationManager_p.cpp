#include "SynchronizationManager_p.h"
#include <keychain.h>
#include <client/Utility.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>
#include <tools/ApplicationSettings.h>
#include <tools/QuteNoteCheckPtr.h>
#include <tools/Printable.h>
#include <Simplecrypt.h>
#include <QApplication>

#define EXPIRATION_TIMESTAMP_KEY "ExpirationTimestamp"
#define LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX "LinkedNotebookExpirationTimestamp_"
#define LINKED_NOTEBOOK_AUTH_TOKEN_KEY_PART "_LinkedNotebookAuthToken_"
#define READ_LINKED_NOTEBOOK_AUTH_TOKEN_JOB "readLinkedNotebookAuthToken"
#define WRITE_LINKED_NOTEBOOK_AUTH_TOKEN_JOB "writeLinkedNotebookAuthToken"
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

namespace qute_note {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    m_maxSyncChunkEntries(50),
    m_lastUpdateCount(-1),
    m_lastSyncTime(-1),
    m_cachedLinkedNotebookLastUpdateCountByGuid(),
    m_cachedLinkedNotebookLastSyncTimeByGuid(),
    m_onceReadLastSyncParams(false),
    m_noteStore(QSharedPointer<qevercloud::NoteStore>(new qevercloud::NoteStore)),
    m_authContext(AuthContext::Blank),
    m_launchSyncPostponeTimerId(-1),
    m_pOAuthWebView(new qevercloud::EvernoteOAuthWebView),
    m_pOAuthResult(),
    m_remoteToLocalSyncManager(localStorageManagerThreadWorker, m_noteStore.getQecNoteStore()),
    m_sendLocalChangesManager(localStorageManagerThreadWorker, m_noteStore.getQecNoteStore()),
    m_linkedNotebookGuidsAndShareKeysWaitingForAuth(),
    m_cachedLinkedNotebookAuthTokensByGuid(),
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid(),
    m_authenticateToLinkedNotebooksPostponeTimerId(-1),
    m_receivedRequestToAuthenticateToLinkedNotebooks(false),
    m_readAuthTokenJob(QApplication::applicationName() + "_read_auth_token"),
    m_writeAuthTokenJob(QApplication::applicationName() + "_write_auth_token"),
    m_readLinkedNotebookAuthTokenJobsByGuid(),
    m_writeLinkedNotebookAuthTokenJobsByGuid(),
    m_linkedNotebookGuidsWithoutLocalAuthData(),
    m_shouldRepeatIncrementalSyncAfterSendingChanges(false)
{
    m_readAuthTokenJob.setAutoDelete(false);
    m_readAuthTokenJob.setKey(QApplication::applicationName() + "_auth_token");

    m_writeAuthTokenJob.setAutoDelete(false);

    createConnections();
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{}

void SynchronizationManagerPrivate::synchronize()
{
    clear();
    authenticate(AuthContext::SyncLaunch);
}

void SynchronizationManagerPrivate::onPauseRequest()
{
    QNDEBUG("SynchronizationManagerPrivate::onPauseRequest");

    if (m_remoteToLocalSyncManager.active()) {
        emit pauseRemoteToLocalSync();
    }

    if (m_sendLocalChangesManager.active()) {
        emit pauseSendingLocalChanges();
    }
}

void SynchronizationManagerPrivate::onStopRequest()
{
    QNDEBUG("SynchronizationManagerPrivate::onStopRequest");

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
    *m_pOAuthResult = m_pOAuthWebView->oauthResult();

    m_noteStore.setNoteStoreUrl(m_pOAuthResult->noteStoreUrl);
    m_noteStore.setAuthenticationToken(m_pOAuthResult->authenticationToken);

    launchStoreOAuthResult();
}

void SynchronizationManagerPrivate::onOAuthFailure()
{
    QString error = QT_TR_NOOP("OAuth failed");
    QString oauthError = m_pOAuthWebView->oauthError();
    if (!oauthError.isEmpty()) {
        error += ": ";
        error += oauthError;
    }

    emit notifyError(error);
}

void SynchronizationManagerPrivate::onKeychainJobFinished(QKeychain::Job * job)
{
    if (!job) {
        QString error = QT_TR_NOOP("QKeychain error: null pointer on keychain job finish");
        emit notifyError(error);
        return;
    }

    if (job == &m_readAuthTokenJob)
    {
        onReadAuthTokenFinished();
    }
    else if (job == &m_writeAuthTokenJob)
    {
        onWriteAuthTokenFinished();
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
                    QString error = QT_TR_NOOP("Error writing the linked notebook's authentication token: error code = ") +
                                    ToQString(job->error()) + ": " + job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);
                }

                (void)m_writeLinkedNotebookAuthTokenJobsByGuid.erase(it);
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
                if (job->error() == QKeychain::NoError) {
                    QNDEBUG("Successfully read authentication token for linked notebook from the keychain. "
                            "linked notebook guid: " << it.key());
                    m_cachedLinkedNotebookAuthTokensByGuid[it.key()] = cachedJob->textData();
                }
                else if (job->error() == QKeychain::EntryNotFound) {
                    QNDEBUG("Could find authentication token for linked notebook in the keychain: "
                            "linked notebook guid: " << it.key());
                    (void)m_linkedNotebookGuidsWithoutLocalAuthData.insert(it.key());
                }
                else {
                    QString error = QT_TR_NOOP("Error reading the linked notebook's auth token: error code = ") +
                                    ToQString(job->error()) + ": " + job->errorString();
                    QNWARNING(error);
                    emit notifyError(error);
                    return;
                }

                authenticateToLinkedNotebooks();
                (void)m_readLinkedNotebookAuthTokenJobsByGuid.erase(it);
                return;
            }
        }

        QString error = QT_TR_NOOP("Unknown keychain job finished event");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
}

void SynchronizationManagerPrivate::onRequestAuthenticationToken()
{
    if (validAuthentication()) {
        QString error = QT_TR_NOOP("Unexpected AUTH_EXPIRED error while the cached authentication token "
                                   "seems to be valid");
        QNWARNING(error << ": " << *m_pOAuthResult);
        emit notifyError(error);
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

void SynchronizationManagerPrivate::onRateLimitExceeded(qint32 secondsToWait)
{
    QNDEBUG("SynchronizationManagerPrivate::onRateLimitExceeded");
    emit rateLimitExceeded(secondsToWait);
}

void SynchronizationManagerPrivate::createConnections()
{
    // Connections with OAuth handler
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFinished(bool)), this, SLOT(onOAuthResult(bool)));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationSuceeded), this, SLOT(onOAuthSuccess()));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFailed), this, SLOT(onOAuthFailure()));

    // Connections with remote to local synchronization manager
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(error(QString)), this, SIGNAL(notifyError(QString)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(finished(qint32,qevercloud::Timestamp,QHash<QString,qint32>,
                                                                  QHash<QString,qevercloud::Timestamp>)),
                     this, SLOT(onRemoteToLocalSyncFinished(qint32,qevercloud::Timestamp,QHash<QString,qint32>,
                                                            QHash<QString,qevercloud::Timestamp>)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(rateLimitExceeded(qint32)), this, SLOT(onRateLimitExceeded(qint32)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(requestAuthenticationToken()),
                     this, SLOT(onRequestAuthenticationToken()));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(requestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> >)),
                     this, SLOT(onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> >)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(paused(bool)), this, SLOT(onRemoteToLocalSyncPaused(bool)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(stopped()), this, SLOT(onRemoteToLocalSyncStopped()));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(requestLastSyncParameters()), this, SLOT(onRequestLastSyncParameters()));
    QObject::connect(this, SIGNAL(pauseRemoteToLocalSync()), &m_remoteToLocalSyncManager, SLOT(pause()));
    QObject::connect(this, SIGNAL(stopRemoteToLocalSync()), &m_remoteToLocalSyncManager, SLOT(stop()));
    QObject::connect(this, SIGNAL(sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString>,QHash<QString,qevercloud::Timestamp>)),
                     &m_remoteToLocalSyncManager, SLOT(onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString,QString>,QHash<QString,qevercloud::Timestamp>)));
    QObject::connect(this, SIGNAL(sendLastSyncParameters(qint32,qevercloud::Timestamp,QHash<QString,qint32>,QHash<QString,qevercloud::Timestamp>)),
                     &m_remoteToLocalSyncManager, SLOT(onLastSyncParametersReceived(qint32,qevercloud::Timestamp,QHash<QString,qint32>,QHash<QString,qevercloud::Timestamp>)));

    // Connections with send local changes manager
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(failure(QString)), this, SIGNAL(notifyError(QString)));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(finished(qint32,QHash<QString,qint32>)),
                     this, SLOT(onLocalChangesSent(qint32,QHash<QString,qint32>)));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(rateLimitExceeded(qint32)), this, SLOT(onRateLimitExceeded(qint32)));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(requestAuthenticationToken()),
                     this, SLOT(onRequestAuthenticationToken()));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(requestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> >)),
                     this, SLOT(onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString,QString> >)));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(shouldRepeatIncrementalSync()), this, SLOT(onShouldRepeatIncrementalSync()));
    QObject::connect(&m_sendLocalChangesManager, SIGNAL(conflictDetected()), this, SLOT(onConflictDetectedDuringLocalChangesSending()));
    QObject::connect(this, SIGNAL(sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString>,QHash<QString,qevercloud::Timestamp>)),
                     &m_sendLocalChangesManager, SLOT(onAuthenticationTokensForLinkedNotebooksReceived(QHash<QString,QString>,QHash<QString,qevercloud::Timestamp>)));

    // TODO: continue with other connections

    // Connections with read/write password jobs
    QObject::connect(&m_readAuthTokenJob, SIGNAL(finished(QKeychain::Job*)), this, SLOT(onKeychainJobFinished(QKeychain::Job*)));
    QObject::connect(&m_writeAuthTokenJob, SIGNAL(finished(QKeychain::Job*)), this, SLOT(onKeychainJobFinished(QKeychain::Job*)));
}

void SynchronizationManagerPrivate::readLastSyncParameters()
{
    QNDEBUG("SynchronizationManagerPrivate::readLastSyncParameters");

    m_lastSyncTime = 0;
    m_lastUpdateCount = 0;
    m_cachedLinkedNotebookLastUpdateCountByGuid.clear();
    m_cachedLinkedNotebookLastSyncTimeByGuid.clear();

    ApplicationSettings & settings = ApplicationSettings::instance();

    const QString keyGroup = LAST_SYNC_PARAMS_KEY_GROUP;
    QVariant lastUpdateCountVar = settings.value(LAST_SYNC_UPDATE_COUNT_KEY, keyGroup);
    if (!lastUpdateCountVar.isNull())
    {
        bool conversionResult = false;
        m_lastUpdateCount = lastUpdateCountVar.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read last update count from persistent application settings");
            m_lastUpdateCount = 0;
        }
    }

    QVariant lastSyncTimeVar = settings.value(LAST_SYNC_TIME_KEY, keyGroup);
    if (!lastUpdateCountVar.isNull())
    {
        bool conversionResult = false;
        m_lastSyncTime = lastSyncTimeVar.toLongLong(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read last sync time from persistent application settings");
            m_lastSyncTime = 0;
        }
    }

    int numLinkedNotebooksSyncParams = settings.beginReadArray(LAST_SYNC_LINKED_NOTEBOOKS_PARAMS);
    for(int i = 0; i < numLinkedNotebooksSyncParams; ++i)
    {
        settings.setArrayIndex(i);

        QString guid = settings.value(LINKED_NOTEBOOK_GUID_KEY).toString();
        if (guid.isEmpty()) {
            QNWARNING("Couldn't read linked notebook's guid from persistent application settings");
            continue;
        }

        QVariant lastUpdateCountVar = settings.value(LINKED_NOTEBOOK_LAST_UPDATE_COUNT_KEY);
        bool conversionResult = false;
        qint32 lastUpdateCount = lastUpdateCountVar.toInt(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read linked notebook's last update count from persistent application settings");
            continue;
        }

        QVariant lastSyncTimeVar = settings.value(LINKED_NOTEBOOK_LAST_SYNC_TIME_KEY);
        conversionResult = false;
        qevercloud::Timestamp lastSyncTime = lastSyncTimeVar.toLongLong(&conversionResult);
        if (!conversionResult) {
            QNWARNING("Couldn't read linked notebook's last sync time from persistent application settings");
            continue;
        }

        m_cachedLinkedNotebookLastUpdateCountByGuid[guid] = lastUpdateCount;
        m_cachedLinkedNotebookLastSyncTimeByGuid[guid] = lastSyncTime;
    }
    settings.endArray();

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

    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    QVariant tokenExpirationValue = appSettings.value(EXPIRATION_TIMESTAMP_KEY, keyGroup);
    if (tokenExpirationValue.isNull()) {
        QNINFO("Authentication token expiration timestamp was not found within application settings, "
               "assuming it has never been written & launching the OAuth procedure");
        launchOAuth();
        return;
    }

    bool conversionResult = false;
    qevercloud::Timestamp tokenExpirationTimestamp = tokenExpirationValue.toLongLong(&conversionResult);
    if (!conversionResult) {
        QString error = QT_TR_NOOP("Failed to convert QVariant with authentication token expiration timestamp "
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

    m_pOAuthResult->expires = tokenExpirationTimestamp;

    QNTRACE("Restoring persistent note store url");

    QVariant noteStoreUrlValue = appSettings.value(NOTE_STORE_URL_KEY, keyGroup);
    if (noteStoreUrlValue.isNull()) {
        QString error = QT_TR_NOOP("Can't find note store url within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString noteStoreUrl = noteStoreUrlValue.toString();
    if (noteStoreUrl.isEmpty()) {
        QString error = QT_TR_NOOP("Can't convert note store url from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_pOAuthResult->noteStoreUrl = noteStoreUrl;

    QNDEBUG("Restoring persistent user id");

    QVariant userIdValue = appSettings.value(USER_ID_KEY, keyGroup);
    if (userIdValue.isNull()) {
        QString error = QT_TR_NOOP("Can't find user id within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    conversionResult = false;
    qevercloud::UserID userId = userIdValue.toInt(&conversionResult);
    if (!conversionResult) {
        QString error = QT_TR_NOOP("Can't convert user id from QVariant to qint32");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_pOAuthResult->userId = userId;

    QNDEBUG("Restoring persistent web api url prefix");

    QVariant webApiUrlPrefixValue = appSettings.value(WEB_API_URL_PREFIX_KEY, keyGroup);
    if (webApiUrlPrefixValue.isNull()) {
        QString error = QT_TR_NOOP("Can't find web API url prefix within persistent application settings");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString webApiUrlPrefix = webApiUrlPrefixValue.toString();
    if (webApiUrlPrefix.isEmpty()) {
        QString error = QT_TR_NOOP("Can't convert web api url prefix from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_pOAuthResult->webApiUrlPrefix = webApiUrlPrefix;

    QNDEBUG("Trying to restore the authentication token from the keychain");
    m_readAuthTokenJob.start();
}

void SynchronizationManagerPrivate::launchOAuth()
{
    // TODO: move consumer key and consumer secret out of core library; they should be set by the application
    // (client of the library) but not hardcoded in the library itself

    SimpleCrypt crypto(0xB87F6B9);

    QString consumerKey = crypto.decryptToString(QByteArray("AwP9s05FRUQgWDvIjk7sru7uV3H5QCBk1W1"
                                                            "fiJsnZrc5qCOs75z4RrgmnN0awQ8d7X7PBu"
                                                            "HHF8JDM33DcxXHSh5Kt8fjQoz5HrPOu8i66"
                                                            "frlRFKguciY7xULwrXdqgECazLB9aveoIo7f"
                                                            "SDRK9FEM0tTOjfdYVGyC3XW86WH42AT/hVpG"
                                                            "QqGKDYNPSJktaeMQ0wVoi7u1iUCMN7L7boxl3"
                                                            "jUO1hw6EjfnO4+f6svSocjkTmrt0qagvxpb1g"
                                                            "xrjdYzOF/7XO9SB8wq0pPihnIvMQAMlVhW9lG"
                                                            "2stiXrgAL0jebYmsHWo1XFNPUN2MGi7eM22uW"
                                                            "fVFAoUW128Qgu/W4+W3dbetmV3NEDjxojsvBn"
                                                            "koe2J8ZyeZ+Ektss4HrzBGTnmH1x0HzZOrMR9"
                                                            "zm9BFP7JADvc2QTku"));
    if (consumerKey.isEmpty()) {
        emit notifyError(QT_TR_NOOP("Can't decrypt the consumer key"));
        return;
    }

    QString consumerSecret = crypto.decryptToString(QByteArray("AwNqc1pRUVDQqMs4frw+fU1N9NJay4hlBoiEXZ"
                                                               "sBC7bffG0TKeA3+INU0F49tVjvLNvU3J4haezDi"
                                                               "wrAPVgCBsADTKz5ss3woXfHlyHVUQ7C41Q8FFS6"
                                                               "EPpgT2tM1835rb8H3+FAHF+2mu8QBIVhe0dN1js"
                                                               "S9+F+iTWKyTwMRO1urOLaF17GEHemW5YLlsl3MN"
                                                               "U5bz9Kq8Uw/cWOuo3S2849En8ZFbYmUE9DDGsO7"
                                                               "eRv9lkeMe8PQ5F1GSVMV8grB71nz4E1V4wVrHR1"
                                                               "vRm4WchFO/y2lWzq4DCrVjnqZNCWOrgwH6dnOpHg"
                                                               "glvnsCSN2YhB+i9LhTLvM8qZHN51HZtXEALwSoWX"
                                                               "UZ3fD5jspD0MRZNN+wr+0zfwVovoPIwo7SSkcqIY"
                                                               "1P2QSi+OcisJLerkthnyAPouiatyDYC2PDLhu25iu"
                                                               "09ONDC0KA=="));
    if (consumerSecret.isEmpty()) {
        emit notifyError(QT_TR_NOOP("Can't decrypt the consumer secret"));
        return;
    }

    m_pOAuthWebView->authenticate("sandbox.evernote.com", consumerKey, consumerSecret);
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
    m_writeAuthTokenJob.setTextData(m_pOAuthResult->authenticationToken);
    m_writeAuthTokenJob.start();
}

void SynchronizationManagerPrivate::finalizeStoreOAuthResult()
{
    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    appSettings.setValue(NOTE_STORE_URL_KEY, m_pOAuthResult->noteStoreUrl, keyGroup);
    appSettings.setValue(EXPIRATION_TIMESTAMP_KEY, m_pOAuthResult->expires, keyGroup);
    appSettings.setValue(USER_ID_KEY, m_pOAuthResult->userId, keyGroup);
    appSettings.setValue(WEB_API_URL_PREFIX_KEY, m_pOAuthResult->webApiUrlPrefix, keyGroup);

    QNDEBUG("Successfully wrote authentication result info to the application settings. "
            "Token expiration timestamp = " << PrintableDateTimeFromTimestamp(m_pOAuthResult->expires)
            << ", user id = " << m_pOAuthResult->userId << ", web API url prefix = "
            << m_pOAuthResult->webApiUrlPrefix);

    finalizeAuthentication();
}

void SynchronizationManagerPrivate::finalizeAuthentication()
{
    QNDEBUG("SynchronizationManagerPrivate::finalizeAuthentication: result = " << *m_pOAuthResult);

    switch(m_authContext)
    {
    case AuthContext::Blank:
    {
        QString error = QT_TR_NOOP("Internal error: incorrect authentication context: blank");
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
        QString error = QT_TR_NOOP("Internal error: unknown authentication context: ");
        error += ToQString(m_authContext);
        emit notifyError(error);
        break;
    }
    }

    m_authContext = AuthContext::Blank;
}

void SynchronizationManagerPrivate::timerEvent(QTimerEvent * pTimerEvent)
{
    if (!pTimerEvent) {
        QString errorDescription = QT_TR_NOOP("Qt error: detected null pointer to QTimerEvent");
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

    // NOTE: don't do anything with m_pOauthWebView
    *m_pOAuthResult = qevercloud::EvernoteOAuthWebView::OAuthResult();

    m_remoteToLocalSyncManager.stop();
    m_sendLocalChangesManager.stop();

    m_linkedNotebookGuidsAndShareKeysWaitingForAuth.clear();
    m_cachedLinkedNotebookAuthTokensByGuid.clear();
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.clear();

    m_authenticateToLinkedNotebooksPostponeTimerId = -1;
    m_receivedRequestToAuthenticateToLinkedNotebooks = false;

    m_readLinkedNotebookAuthTokenJobsByGuid.clear();
    m_writeLinkedNotebookAuthTokenJobsByGuid.clear();

    m_linkedNotebookGuidsWithoutLocalAuthData.clear();
}

bool SynchronizationManagerPrivate::validAuthentication() const
{
    if (m_pOAuthResult->expires == static_cast<qint64>(0)) {
        // The value is not set
        return false;
    }

    return !checkIfTimestampIsAboutToExpireSoon(m_pOAuthResult->expires);
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
        QNDEBUG("No linked notebooks waiting for authentication, sending empty authentication tokens back immediately");
        emit sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString>(),
                                                        QHash<QString,qevercloud::Timestamp>());
        return;
    }

    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    QHash<QString,QString> authTokensToCacheByGuid;
    QHash<QString,qevercloud::Timestamp> authTokenExpirationTimestampsToCacheByGuid;

    for(auto it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.begin();
        it != m_linkedNotebookGuidsAndShareKeysWaitingForAuth.end(); )
    {
        const QPair<QString,QString> & pair = *it;

        const QString & guid     = pair.first;
        const QString & shareKey = pair.second;

        bool forceRemoteAuth = false;
        auto linkedNotebookAuthTokenIt = m_cachedLinkedNotebookAuthTokensByGuid.find(guid);
        if (linkedNotebookAuthTokenIt == m_cachedLinkedNotebookAuthTokensByGuid.end())
        {
            auto noAuthDataIt = m_linkedNotebookGuidsWithoutLocalAuthData.find(guid);
            if (noAuthDataIt != m_linkedNotebookGuidsWithoutLocalAuthData.end())
            {
                forceRemoteAuth = true;
                (void)m_linkedNotebookGuidsWithoutLocalAuthData.erase(noAuthDataIt);
            }
            else
            {
                QNDEBUG("Haven't found the authentication token for linked notebook guid " << guid
                        << " in the local cache, will try to read it from the keychain");

                QSharedPointer<QKeychain::ReadPasswordJob> job;
                auto readJobIt = m_readLinkedNotebookAuthTokenJobsByGuid.find(guid);
                if (readJobIt == m_readLinkedNotebookAuthTokenJobsByGuid.end()) {
                    job = QSharedPointer<QKeychain::ReadPasswordJob>(new QKeychain::ReadPasswordJob(READ_LINKED_NOTEBOOK_AUTH_TOKEN_JOB));
                    readJobIt = m_readLinkedNotebookAuthTokenJobsByGuid.insert(guid, job);
                }
                else {
                    job = readJobIt.value();
                }

                job->setKey(guid);

                QObject::connect(job.data(), SIGNAL(finished(QKeychain::Job*)),
                                 this, SLOT(onKeychainJobFinished(QKeychain::Job*)));

                job->start();
                continue;
            }
        }

        if (!forceRemoteAuth)
        {
            auto linkedNotebookAuthTokenExpirationIt = m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.find(guid);
            if (linkedNotebookAuthTokenExpirationIt == m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.end())
            {
                QVariant expirationTimeVariant = appSettings.value(LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX + guid, keyGroup);
                if (!expirationTimeVariant.isNull())
                {
                    bool conversionResult = false;
                    qevercloud::Timestamp expirationTime = expirationTimeVariant.toLongLong(&conversionResult);
                    if (conversionResult) {
                        linkedNotebookAuthTokenExpirationIt = m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.insert(guid, expirationTime);
                    }
                    else {
                        QNWARNING("Can't convert linked notebook's authentication token from QVariant retrieved from "
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
            continue;
        }

        if (m_authContext != AuthContext::Blank) {
            QNDEBUG("Authentication context variable is not set to blank which means that authentication must be in progress: "
                    << m_authContext << "; won't attempt to call remote Evernote API at this time");
            continue;
        }

        qevercloud::AuthenticationResult authResult;
        QString errorDescription;
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = m_noteStore.authenticateToSharedNotebook(shareKey, authResult,
                                                                    errorDescription, rateLimitSeconds);
        if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (validAuthentication()) {
                errorDescription = QT_TR_NOOP("Internal error: unexpected AUTH_EXPIRED error");
                emit notifyError(errorDescription);
            }
            else {
                authenticate(AuthContext::AuthToLinkedNotebooks);
            }

            continue;
        }
        else if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Internal error: RATE_LIMIT_REACHED exception was caught "
                                              "but the number of seconds to wait is negative");
                emit notifyError(errorDescription);
                return;
            }

            m_authenticateToLinkedNotebooksPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            continue;
        }
        else if (errorCode != 0)
        {
            emit notifyError(errorDescription);
            return;
        }

        m_cachedLinkedNotebookAuthTokensByGuid[guid] = authResult.authenticationToken;
        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid[guid] = authResult.expiration;

        authTokensToCacheByGuid[guid] = authResult.authenticationToken;
        authTokenExpirationTimestampsToCacheByGuid[guid] = authResult.expiration;

        it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.erase(it);
    }

    if (m_linkedNotebookGuidsAndShareKeysWaitingForAuth.empty()) {
        QNDEBUG("Retrieved authentication data for all requested linked notebooks, sending the answer now");
        // TODO: ensure expiration timestamps are already fetched by this point
        emit sendAuthenticationTokensForLinkedNotebooks(m_cachedLinkedNotebookAuthTokensByGuid,
                                                        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid);
    }

    // Caching linked notebook's authentication token's expiration time in app settings
    typedef QHash<QString,qevercloud::Timestamp>::const_iterator ExpirationTimeCIter;
    ExpirationTimeCIter authTokenExpirationTimesToCacheEnd = authTokenExpirationTimestampsToCacheByGuid.end();
    for(ExpirationTimeCIter it = authTokenExpirationTimestampsToCacheByGuid.begin();
        it != authTokenExpirationTimesToCacheEnd; ++it)
    {
        QString key = LINKED_NOTEBOOK_EXPIRATION_TIMESTAMP_KEY_PREFIX + it.key();
        appSettings.setValue(key, QVariant(it.value()), keyGroup);
    }

    // Caching linked notebook's authentication tokens in the keychain
    QString appName = QApplication::applicationName();

    typedef QHash<QString,QString>::const_iterator AuthTokenCIter;
    AuthTokenCIter authTokensToCacheEnd = authTokensToCacheByGuid.end();
    for(AuthTokenCIter it = authTokensToCacheByGuid.begin();
        it != authTokensToCacheEnd; ++it)
    {
        const QString & guid = it.key();
        const QString & token = it.value();

        QString key = appName + LINKED_NOTEBOOK_AUTH_TOKEN_KEY_PART + guid;
        QSharedPointer<QKeychain::WritePasswordJob> job(new QKeychain::WritePasswordJob(WRITE_LINKED_NOTEBOOK_AUTH_TOKEN_JOB));
        (void)m_writeLinkedNotebookAuthTokenJobsByGuid.insert(key, job);
        job->setKey(key);
        job->setTextData(token);

        QObject::connect(job.data(), SIGNAL(finished(QKeychain::Job *)),
                         this, SLOT(onKeychainJobFinished(QKeychain::Job *)));

        job->start();
    }
}

void SynchronizationManagerPrivate::onReadAuthTokenFinished()
{
    QKeychain::Error errorCode = m_readAuthTokenJob.error();
    if (errorCode == QKeychain::EntryNotFound)
    {
        QString error = QT_TR_NOOP("Unexpectedly missing OAuth token in password storage: ") +
                        m_readAuthTokenJob.errorString();
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to read authentication token returned with error: error code " << errorCode
                  << ", " << m_readAuthTokenJob.errorString());
        emit notifyError(QT_TR_NOOP("Can't read stored auth token from the keychain: ") +
                         m_readAuthTokenJob.errorString());
        return;
    }

    QNDEBUG("Successfully restored the authentication token");
    m_pOAuthResult->authenticationToken = m_readAuthTokenJob.textData();

    finalizeAuthentication();
}

void SynchronizationManagerPrivate::onWriteAuthTokenFinished()
{
    QKeychain::Error errorCode = m_writeAuthTokenJob.error();
    if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to read authentication token returned with error: error code " << errorCode
                  << ", " << m_writeAuthTokenJob.errorString());
        emit notifyError(QT_TR_NOOP("Can't write oauth token to the keychain: ") +
                         m_writeAuthTokenJob.errorString());
        return;
    }

    QNDEBUG("Successfully stored the authentication token in the keychain");
    finalizeStoreOAuthResult();
}

void SynchronizationManagerPrivate::updatePersistentSyncSettings()
{
    QNDEBUG("SynchronizationManagerPrivate::updatePersistentSyncSettings");

    ApplicationSettings & settings = ApplicationSettings::instance();

    const QString keyGroup = LAST_SYNC_PARAMS_KEY_GROUP;
    settings.setValue(LAST_SYNC_UPDATE_COUNT_KEY, m_lastUpdateCount, keyGroup);
    settings.setValue(LAST_SYNC_TIME_KEY, m_lastSyncTime, keyGroup);

    int numLinkedNotebooksSyncParams = m_cachedLinkedNotebookLastUpdateCountByGuid.size();
    settings.beginWriteArray(LAST_SYNC_LINKED_NOTEBOOKS_PARAMS, numLinkedNotebooksSyncParams);

    size_t counter = 0;
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

        settings.setArrayIndex(counter);
        settings.setValue(LINKED_NOTEBOOK_LAST_UPDATE_COUNT_KEY, updateCountIt.value());
        settings.setValue(LINKED_NOTEBOOK_LAST_SYNC_TIME_KEY, syncTimeIt.value());

        ++counter;
    }

    settings.endArray();

    QNTRACE("Wrote " << counter << " last sync params entries for linked notebooks");
}

} // namespace qute_note
