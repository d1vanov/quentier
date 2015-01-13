#include "SynchronizationManager_p.h"
#include <keychain.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>
#include <tools/ApplicationSettings.h>
#include <tools/QuteNoteCheckPtr.h>
#include <tools/EventLoopWithExitStatus.h>
#include <Simplecrypt.h>
#include <QApplication>

#define EXPIRATION_TIMESTAMP_KEY "ExpirationTimestamp"
#define NOTE_STORE_URL_KEY "NoteStoreUrl"
#define USER_ID_KEY "UserId"
#define WEB_API_URL_PREFIX_KEY "WebApiUrlPrefix"
#define SEC_TO_MSEC(sec) (sec * 100)

namespace qute_note {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    m_maxSyncChunkEntries(50),
    m_lastUpdateCount(-1),
    m_lastSyncTime(-1),
    m_noteStore(QSharedPointer<qevercloud::NoteStore>(new qevercloud::NoteStore)),
    m_launchSyncPostponeTimerId(-1),
    m_pOAuthWebView(new qevercloud::EvernoteOAuthWebView),
    m_pOAuthResult(),
    m_remoteToLocalSyncManager(localStorageManagerThreadWorker, m_noteStore.getQecNoteStore()),
    m_linkedNotebookGuidsAndShareKeysWaitingForAuth(),
    m_cachedLinkedNotebookAuthTokensByGuid(),
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid(),
    m_authenticateToLinkedNotebooksPostponeTimerId(-1),
    m_receivedRequestToAuthenticateToLinkedNotebooks(false),
    m_readAuthTokenMutex(),
    m_writeAuthTokenMutex(),
    m_authenticationMutex()
{
    createConnections();
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{
    m_readAuthTokenMutex.unlock();
    m_writeAuthTokenMutex.unlock();
    m_authenticationMutex.unlock();
}

void SynchronizationManagerPrivate::synchronize()
{
    clear();

    if (!authenticate()) {
        return;
    }

    launchSync();
}

void SynchronizationManagerPrivate::onOAuthRequest(QString host, QString consumerKey,
                                                   QString consumerSecret)
{
    m_pOAuthWebView->authenticate(host, consumerKey, consumerSecret);
}

void SynchronizationManagerPrivate::onRequestAuthenticationTokensForLinkedNotebooks(QList<QPair<QString, QString> > linkedNotebookGuidsAndShareKeys)
{
    QNDEBUG("SynchronizationManagerPrivate::onRequestAuthenticationTokensForLinkedNotebooks");

    m_receivedRequestToAuthenticateToLinkedNotebooks = true;
    m_linkedNotebookGuidsAndShareKeysWaitingForAuth = linkedNotebookGuidsAndShareKeys;

    const int numLinkedNotebooks = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.size();
    if (numLinkedNotebooks == 0) {
        QNDEBUG("No linked notebooks, sending empty authentication tokens back immediately");
        emit sendAuthenticationTokensForLinkedNotebooks(QHash<QString,QString>());
        return;
    }

    for(auto it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.begin();
        it != m_linkedNotebookGuidsAndShareKeysWaitingForAuth.end(); )
    {
        const QPair<QString,QString> & pair = *it;

        const QString & guid     = pair.first;
        const QString & shareKey = pair.second;

        qevercloud::AuthenticationResult authResult;
        QString errorDescription;
        qint32 rateLimitSeconds = 0;
        qint32 errorCode = m_noteStore.authenticateToSharedNotebook(shareKey, authResult,
                                                                    errorDescription, rateLimitSeconds);
        if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
        {
            if (!authenticate()) {
                return;
            }

            errorCode = m_noteStore.authenticateToSharedNotebook(shareKey, authResult,
                                                                 errorDescription, rateLimitSeconds);
        }

        if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (rateLimitSeconds < 0) {
                errorDescription = QT_TR_NOOP("Internal error: RATE_LIMIT_REACHED exception was caught "
                                              "but the number of seconds to wait is negative");
                emit notifyError(errorDescription);
                return;
            }

            m_authenticateToLinkedNotebooksPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
            return;
        }
        else if (errorCode != 0)
        {
            emit notifyError(errorDescription);
            return;
        }

        m_cachedLinkedNotebookAuthTokensByGuid[guid] = authResult.authenticationToken;
        m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid[guid] = authResult.expiration;

        it = m_linkedNotebookGuidsAndShareKeysWaitingForAuth.erase(it);
    }

    // TODO: cache obtained auth tokens
    emit sendAuthenticationTokensForLinkedNotebooks(m_cachedLinkedNotebookAuthTokensByGuid);
}

void SynchronizationManagerPrivate::onRemoteToLocalSyncFinished(qint32 lastUpdateCount, qint32 lastSyncTime)
{
    // TODO: implement
    m_lastUpdateCount = lastUpdateCount;
    m_lastSyncTime = lastSyncTime;
}

void SynchronizationManagerPrivate::createConnections()
{
    // Connections with OAuth handler
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFinished(bool)), this, SLOT(onOAuthResult(bool)));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationSuceeded), this, SLOT(onOAuthSuccess()));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFailed), this, SLOT(onOAuthFailure()));

    // Connections with remote to local synchronization manager
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(error(QString)), this, SIGNAL(notifyError(QString)));
    QObject::connect(&m_remoteToLocalSyncManager, SIGNAL(finished()), this, SLOT(onRemoteToLocalSyncFinished()));
}

bool SynchronizationManagerPrivate::authenticate()
{
    QNDEBUG("Trying to restore persistent authentication settings...");

    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    QVariant tokenExpirationValue = appSettings.value(EXPIRATION_TIMESTAMP_KEY, keyGroup);
    if (tokenExpirationValue.isNull())
    {
        QNDEBUG("Failed to restore authentication token's expiration timestamp, "
                "launching OAuth procedure");
        bool res = oauth();
        if (!res) {
            return false;
        }

        tokenExpirationValue = appSettings.value(EXPIRATION_TIMESTAMP_KEY, keyGroup);
        if (tokenExpirationValue.isNull()) {
            QString error = QT_TR_NOOP("Can't restore OAuth expiration timestamp right after OAuth");
            emit notifyError(error);
            return false;
        }
    }

    bool conversionResult = false;
    qevercloud::Timestamp tokenExpirationTimestamp = tokenExpirationValue.toLongLong(&conversionResult);
    if (!conversionResult) {
        QString error = QT_TR_NOOP("Failed to convert QVariant with authentication token expiration timestamp "
                                   "to actual timestamp");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    qevercloud::Timestamp currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    if (currentTimestamp > tokenExpirationTimestamp)
    {
        QNDEBUG("Authentication token has already expired, launching OAuth procedure");
        bool res = oauth();
        if (!res) {
            return false;
        }

        tokenExpirationValue = appSettings.value(EXPIRATION_TIMESTAMP_KEY, keyGroup);
        if (tokenExpirationValue.isNull()) {
            QString error = QT_TR_NOOP("Can't restore OAuth expiration timestamp right after OAuth");
            emit notifyError(error);
            return false;
        }

        conversionResult = false;
        tokenExpirationTimestamp = tokenExpirationValue.toLongLong(&conversionResult);
        if (!conversionResult) {
            QString error = QT_TR_NOOP("Failed to convert QVariant with authentication token "
                                       "expiration timestamp to actual timestamp");
            QNWARNING(error);
            emit notifyError(error);
            return false;
        }

        if (currentTimestamp > tokenExpirationTimestamp) {
            QString error = QT_TR_NOOP("Internal error: OAuth token expiration timestamp "
                                       "is less than current timestamp right after OAuth");
            emit notifyError(error);
            return false;
        }
    }

    QNDEBUG("Trying to restore the authentication token...");

    QKeychain::ReadPasswordJob readPasswordJob(QApplication::applicationName() + "_authenticationToken");
    readPasswordJob.setAutoDelete(false);
    readPasswordJob.setKey(QApplication::applicationName());

    int readPasswordJobReturnCode = -1;
    {
        QMutexLocker locker(&m_readAuthTokenMutex);

        QTimer timer;
        timer.setInterval(SEC_TO_MSEC(10));
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&readPasswordJob, SIGNAL(finished(QKeychain::Job*)), SLOT(exitAsSuccess()));

        QMetaObject::invokeMethod(&readPasswordJob, "start", Qt::QueuedConnection);
        loop.exec();
    }

    QString error;
    if (readPasswordJobReturnCode == -1) {
        error = QT_TR_NOOP("Internal error: incorrect return code from reading password event loop");
    }
    else if (readPasswordJobReturnCode == EventLoopWithExitStatus::ExitStatus::Timeout) {
        error = QT_TR_NOOP("Timeout hit when waiting for reading password event loop to finish");
    }
    else if (readPasswordJobReturnCode == EventLoopWithExitStatus::ExitStatus::Failure) {
        error = QT_TR_NOOP("Failure during reading password event loop");
    }

    if (readPasswordJobReturnCode != EventLoopWithExitStatus::ExitStatus::Success) {
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    QKeychain::Error errorCode = readPasswordJob.error();
    if (errorCode == QKeychain::EntryNotFound)
    {
        QString error = QT_TR_NOOP("Unexpectedly missing OAuth token in password storage: ");
        emit notifyError(error + readPasswordJob.errorString());
        return false;
    }
    else if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to read authentication token returned with error: error code " << errorCode
                  << ", " << readPasswordJob.errorString());
        emit notifyError(readPasswordJob.errorString());
        return false;
    }

    QNDEBUG("Successfully restored the autnehtication token");

    if (m_pOAuthResult.isNull()) {
        m_pOAuthResult = QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>(new qevercloud::EvernoteOAuthWebView::OAuthResult);
    }

    m_pOAuthResult->authenticationToken = readPasswordJob.textData();
    m_pOAuthResult->expires = tokenExpirationTimestamp;

    QNDEBUG("Restoring persistent note store url");

    QVariant noteStoreUrlValue = appSettings.value(NOTE_STORE_URL_KEY, keyGroup);
    if (noteStoreUrlValue.isNull()) {
        error = QT_TR_NOOP("Persistent note store url is unexpectedly empty");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    QString noteStoreUrl = noteStoreUrlValue.toString();
    if (noteStoreUrl.isEmpty()) {
        error = QT_TR_NOOP("Can't convert note store url from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    m_pOAuthResult->noteStoreUrl = noteStoreUrl;

    QNDEBUG("Restoring persistent user id");

    QVariant userIdValue = appSettings.value(USER_ID_KEY, keyGroup);
    if (userIdValue.isNull()) {
        error = QT_TR_NOOP("Persistent user id is unexpectedly empty");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    conversionResult = false;
    qevercloud::UserID userId = userIdValue.toInt(&conversionResult);
    if (!conversionResult) {
        error = QT_TR_NOOP("Can't convert user id from QVariant to qint32");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    m_pOAuthResult->userId = userId;

    QNDEBUG("Restoring persistent web api url prefix");

    QVariant webApiUrlPrefixValue = appSettings.value(WEB_API_URL_PREFIX_KEY, keyGroup);
    if (webApiUrlPrefixValue.isNull()) {
        error = QT_TR_NOOP("Persistent web api url prefix is unexpectedly empty");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    QString webApiUrlPrefix = webApiUrlPrefixValue.toString();
    if (webApiUrlPrefix.isEmpty()) {
        error = QT_TR_NOOP("Can't convert web api url prefix from QVariant to QString");
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    m_pOAuthResult->webApiUrlPrefix = webApiUrlPrefix;

    QNDEBUG("Finished authentication, result: " << *m_pOAuthResult);
    return true;
}

bool SynchronizationManagerPrivate::oauth()
{
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
        return false;
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
        return false;
    }

    // Synchronous waiting for OAuth procedure to get done, with all the necessary precautions;
    // Such waits are not recommended in general but it's just too much trouble to keep things asynchronous,
    // the chain of calls necessary to implement goes crazy really fast

    int authenticationEventLoopReturnCode = -1;
    {
        QMutexLocker locker(&m_authenticationMutex);

        QTimer timer;
        timer.setInterval(SEC_TO_MSEC(10 * 60));    // 10 minutes should be enough
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(m_pOAuthWebView.data(), SIGNAL(authenticationSuceeded()), SLOT(exitAsSuccess()));
        loop.connect(m_pOAuthWebView.data(), SIGNAL(authenticationFailed()), SLOT(exitAsFailure()));

        timer.start();
        QMetaObject::invokeMethod(m_pOAuthWebView.data(),
                                  "authenticate", Qt::QueuedConnection,
                                  Q_ARG(QString, "sandbox.evernote.com"),
                                  Q_ARG(QString, consumerKey),
                                  Q_ARG(QString, consumerSecret));

        authenticationEventLoopReturnCode = loop.exec();
    }

    QString error;
    if (authenticationEventLoopReturnCode == -1) {
        error = QT_TR_NOOP("Internal error: incorrect return status from OAuth event loop");
    }
    else if (authenticationEventLoopReturnCode == EventLoopWithExitStatus::ExitStatus::Timeout) {
        error = QT_TR_NOOP("Timeout hit while waiting for OAuth procedure to complete");
    }
    else if (authenticationEventLoopReturnCode == EventLoopWithExitStatus::ExitStatus::Failure) {
        error = QT_TR_NOOP("OAuth failed: ") + m_pOAuthWebView->oauthError();
    }

    if (authenticationEventLoopReturnCode != EventLoopWithExitStatus::ExitStatus::Success) {
        emit notifyError(error);
        return false;
    }

    // Storing the OAuth result now
    if (m_pOAuthResult.isNull()) {
        m_pOAuthResult = QSharedPointer<qevercloud::EvernoteOAuthWebView::OAuthResult>(new qevercloud::EvernoteOAuthWebView::OAuthResult(m_pOAuthWebView->oauthResult()));
    }
    else {
        *m_pOAuthResult = m_pOAuthWebView->oauthResult();
    }

    bool res = storeOAuthResult();
    if (!res) {
        return false;
    }

    return true;
}

void SynchronizationManagerPrivate::launchSync()
{
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult);

    m_noteStore.setNoteStoreUrl(m_pOAuthResult->noteStoreUrl);
    m_noteStore.setAuthenticationToken(m_pOAuthResult->authenticationToken);

    if (m_lastUpdateCount <= 0) {
        QNDEBUG("The client has never synchronized with the remote service, "
                "performing full sync");
        launchFullSync();
        return;
    }

    qevercloud::SyncState syncState;
    if (!tryToGetSyncState(syncState)) {
        return;
    }

    if ((m_lastUpdateCount >= 0) && (syncState.updateCount <= m_lastUpdateCount)) {
        QNDEBUG("Server's update count " << syncState.updateCount << " is less than or equal to the last cached "
                "update count " << m_lastUpdateCount << "; there's no need to perform "
                "the sync from remote to local storage, only send local changes (if any)");
        sendChanges();
        return;
    }

    if (syncState.fullSyncBefore > m_lastSyncTime) {
        QNDEBUG("Server's current sync state's fullSyncBefore is larger that the timestamp of last sync "
                "(" << syncState.fullSyncBefore << " > " << m_lastSyncTime << "), "
                "continue with full sync");
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
    // TODO: implement
}

void SynchronizationManagerPrivate::sendChanges()
{
    // TODO: implement
}

bool SynchronizationManagerPrivate::storeOAuthResult()
{
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult);

    // Store authentication token in the keychain
    QKeychain::WritePasswordJob writePasswordJob(QApplication::applicationName() + "_authenticationToken");
    writePasswordJob.setAutoDelete(false);
    writePasswordJob.setKey(QApplication::applicationName());
    writePasswordJob.setTextData(m_pOAuthResult->authenticationToken);

    int writePasswodJobReturnCode = -1;
    {
        QMutexLocker locker(&m_writeAuthTokenMutex);

        QTimer timer;
        timer.setInterval(SEC_TO_MSEC(5));
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;
        loop.connect(&timer, SIGNAL(timeout()), SLOT(exitAsTimeout()));
        loop.connect(&writePasswordJob, SIGNAL(finished(QKeychain::Job*)), SLOT(exitAsSuccess()));

        QMetaObject::invokeMethod(&writePasswordJob, "start", Qt::QueuedConnection);
        loop.exec();
    }

    QString error;
    if (writePasswodJobReturnCode == -1) {
        error = QT_TR_NOOP("Internal error: incorrect return code from writinig password event loop");
    }
    else if (writePasswodJobReturnCode == EventLoopWithExitStatus::ExitStatus::Timeout) {
        error = QT_TR_NOOP("Timeout hit when waiting for writing password event loop to finish");
    }
    else if (writePasswodJobReturnCode == EventLoopWithExitStatus::ExitStatus::Failure) {
        error = QT_TR_NOOP("Failure during writing password event loop");
    }

    if (writePasswodJobReturnCode != EventLoopWithExitStatus::ExitStatus::Success) {
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    QKeychain::Error errorCode = writePasswordJob.error();
    if (errorCode != QKeychain::NoError) {
        QNWARNING("Attempt to write autnehtication token failed with error: error code = " << errorCode
                  << ", " << writePasswordJob.errorString());
        emit notifyError(writePasswordJob.errorString());
        return false;
    }

    QNDEBUG("Successfully wrote the authentication token to the system keychain");

    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    appSettings.setValue(NOTE_STORE_URL_KEY, m_pOAuthResult->noteStoreUrl, keyGroup);
    appSettings.setValue(EXPIRATION_TIMESTAMP_KEY, m_pOAuthResult->expires, keyGroup);
    appSettings.setValue(USER_ID_KEY, m_pOAuthResult->userId, keyGroup);
    appSettings.setValue(WEB_API_URL_PREFIX_KEY, m_pOAuthResult->webApiUrlPrefix, keyGroup);

    QNDEBUG("Successfully wrote other authentication result info to the application settings. "
            "Token expiration timestamp = " << m_pOAuthResult->expires << " ("
            << QDateTime::fromMSecsSinceEpoch(m_pOAuthResult->expires).toString(Qt::ISODate) << "), "
            << ", user id = " << m_pOAuthResult->userId << ", web API url prefix = "
            << m_pOAuthResult->webApiUrlPrefix);

    return true;
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

bool SynchronizationManagerPrivate::tryToGetSyncState(qevercloud::SyncState & syncState)
{
    QString error;
    qint32 rateLimitSeconds = 0;
    qint32 errorCode = m_noteStore.getSyncState(syncState, error, rateLimitSeconds);
    if (errorCode == qevercloud::EDAMErrorCode::AUTH_EXPIRED)
    {
        QNDEBUG("caught AUTH_EXPIRED exception, will attempt to re-authorize");
        if (!authenticate()) {
            return false;
        }

        errorCode = m_noteStore.getSyncState(syncState, error, rateLimitSeconds);
    }

    if (errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (rateLimitSeconds < 0) {
            error = QT_TR_NOOP("Internal error: RATE_LIMIT_REACHED exception was caught "
                               "but the number of seconds to wait is negative");
            emit notifyError(error);
            return false;
        }

        m_launchSyncPostponeTimerId = startTimer(SEC_TO_MSEC(rateLimitSeconds));
        return false;
    }
    else
    {
        emit notifyError(error);
        return false;
    }

    return true;
}

void SynchronizationManagerPrivate::clear()
{
    m_lastUpdateCount = -1;
    m_lastSyncTime = -1;

    m_launchSyncPostponeTimerId = -1;

    m_pOAuthWebView.reset();
    m_pOAuthResult.clear();

    m_linkedNotebookGuidsAndShareKeysWaitingForAuth.clear();
    m_cachedLinkedNotebookAuthTokensByGuid.clear();
    m_cachedLinkedNotebookAuthTokenExpirationTimeByGuid.clear();

    m_authenticateToLinkedNotebooksPostponeTimerId = -1;
    m_receivedRequestToAuthenticateToLinkedNotebooks = false;
}

} // namespace qute_note
