#include "SynchronizationManager_p.h"
#include <keychain.h>
#include <client/local_storage/LocalStorageManagerThreadWorker.h>
#include <logging/QuteNoteLogger.h>
#include <tools/ApplicationSettings.h>
#include <tools/QuteNoteCheckPtr.h>
#include <Simplecrypt.h>
#include <QApplication>

#define EXPIRATION_TIMESTAMP_KEY "ExpirationTimestamp"
#define NOTE_STORE_URL_KEY "NoteStoreUrl"
#define USER_ID_KEY "UserId"
#define WEB_API_URL_PREFIX_KEY "WebApiUrlPrefix"

namespace qute_note {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    m_maxSyncChunkEntries(50),
    m_pLastSyncState(),
    m_pOAuthWebView(new qevercloud::EvernoteOAuthWebView),
    m_pOAuthResult(),
    m_pNoteStore()
{
    connect(localStorageManagerThreadWorker);
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{}

void SynchronizationManagerPrivate::synchronize()
{
    if (!m_pOAuthResult) {
        authenticate();
        return;
    }

    launchSync();
}

void SynchronizationManagerPrivate::onOAuthSuccess()
{
    // Store OAuth result for future usage within the session
    if (m_pOAuthResult.isNull()) {
        m_pOAuthResult.reset(new qevercloud::EvernoteOAuthWebView::OAuthResult(m_pOAuthWebView->oauthResult()));
    }
    else {
        *m_pOAuthResult = m_pOAuthWebView->oauthResult();
    }

    bool res = storeOAuthResult();
    if (!res) {
        return;
    }

    launchSync();
}

void SynchronizationManagerPrivate::onOAuthFailure()
{
    emit notifyError(QT_TR_NOOP("OAuth failed: ") + m_pOAuthWebView->oauthError());
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

void SynchronizationManagerPrivate::connect(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFinished(bool)), this, SLOT(onOAuthResult(bool)));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationSuceeded), this, SLOT(onOAuthSuccess()));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFailed), this, SLOT(onOAuthFailure()));

    // Connect local signals with localStorageManagerThread's slots
    QObject::connect(this, SIGNAL(addUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onAddUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(updateUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onUpdateUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(findUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onFindUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(deleteUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onDeleteUserRequest(UserWrapper)));
    QObject::connect(this, SIGNAL(expungeUser(UserWrapper)), &localStorageManagerThreadWorker, SLOT(onExpungeUserRequest(UserWrapper)));

    QObject::connect(this, SIGNAL(addNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onAddNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(updateNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onUpdateNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(findNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onFindNotebookRequest(Notebook)));
    QObject::connect(this, SIGNAL(expungeNotebook(Notebook)), &localStorageManagerThreadWorker, SLOT(onExpungeNotebookRequest(Notebook)));

    QObject::connect(this, SIGNAL(addNote(Note,Notebook)), &localStorageManagerThreadWorker, SLOT(onAddNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(updateNote(Note,Notebook)), &localStorageManagerThreadWorker, SLOT(onUpdateNoteRequest(Note,Notebook)));
    QObject::connect(this, SIGNAL(findNote(Note,bool)), &localStorageManagerThreadWorker, SLOT(onFindNoteRequest(Note,bool)));
    QObject::connect(this, SIGNAL(deleteNote(Note)), &localStorageManagerThreadWorker, SLOT(onDeleteNoteRequest(Note)));
    QObject::connect(this, SIGNAL(expungeNote(Note)), &localStorageManagerThreadWorker, SLOT(onExpungeNoteRequest(Note)));

    QObject::connect(this, SIGNAL(addTag(Tag)), &localStorageManagerThreadWorker, SLOT(onAddTagRequest(Tag)));
    QObject::connect(this, SIGNAL(updateTag(Tag)), &localStorageManagerThreadWorker, SLOT(onUpdateTagRequest(Tag)));
    QObject::connect(this, SIGNAL(findTag(Tag)), &localStorageManagerThreadWorker, SLOT(onFindTagRequest(Tag)));
    QObject::connect(this, SIGNAL(deleteTag(Tag)), &localStorageManagerThreadWorker, SLOT(onDeleteTagRequest(Tag)));
    QObject::connect(this, SIGNAL(expungeTag(Tag)), &localStorageManagerThreadWorker, SLOT(onExpungeTagRequest(Tag)));

    QObject::connect(this, SIGNAL(addResource(ResourceWrapper,Note)), &localStorageManagerThreadWorker, SLOT(onAddResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(updateResource(ResourceWrapper,Note)), &localStorageManagerThreadWorker, SLOT(onUpdateResourceRequest(ResourceWrapper,Note)));
    QObject::connect(this, SIGNAL(findResource(ResourceWrapper,bool)), &localStorageManagerThreadWorker, SLOT(onFindResourceRequest(ResourceWrapper,bool)));
    QObject::connect(this, SIGNAL(expungeResource(ResourceWrapper)), &localStorageManagerThreadWorker, SLOT(onExpungeResourceRequest(ResourceWrapper)));

    QObject::connect(this, SIGNAL(addLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onAddLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(updateLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onUpdateLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(findLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onFindLinkedNotebookRequest(LinkedNotebook)));
    QObject::connect(this, SIGNAL(expungeLinkedNotebook(LinkedNotebook)), &localStorageManagerThreadWorker, SLOT(onExpungeLinkedNotebookRequest(LinkedNotebook)));

    QObject::connect(this, SIGNAL(addSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onAddSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(updateSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onUpdateSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(findSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onFindSavedSearchRequest(SavedSearch)));
    QObject::connect(this, SIGNAL(expungeSavedSearch(SavedSearch)), &localStorageManagerThreadWorker, SLOT(onExpungeSavedSearch(SavedSearch)));

    // Connect localStorageManagerThread's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findUserComplete(UserWrapper)), this, SLOT(onFindUserCompleted(UserWrapper)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findUserFailed(UserWrapper,QString)), this, SLOT(onFindUserFailed(UserWrapper)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNotebookComplete(Notebook)), this, SLOT(onFindNotebookCompleted(Notebook)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNotebookFailed(Notebook,QString)), this, SLOT(onFindNotebookFailed(Notebook)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNoteComplete(Note,bool)), this, SLOT(onFindNoteCompleted(Note)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findNoteFailed(Note,bool,QString)), this, SLOT(onFindNoteFailed(Note)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findTagComplete(Tag)), this, SLOT(onFindTagCompleted(Tag)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findTagFailed(Tag,QString)), this, SLOT(onFindTagFailed(Tag)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findResourceComplete(ResourceWrapper,bool)), this, SLOT(onFindResourceCompleted(ResourceWrapper)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findResourceFailed(ResourceWrapper,bool,QString)), this, SLOT(onFindResourceFailed(ResourceWrapper)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookComplete(LinkedNotebook)), this, SLOT(onFindLinkedNotebookCompleted(LinkedNotebook)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findLinkedNotebookFailed(LinkedNotebook,QString)), this, SLOT(onFindLinkedNotebookFailed(LinkedNotebook)));

    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findSavedSearchComplete(SavedSearch)), this, SLOT(onFindSavedSearchCompleted(SavedSearch)));
    QObject::connect(&localStorageManagerThreadWorker, SIGNAL(findSavedSearchFailed(SavedSearch,QString)), this, SLOT(onFindSavedSearchFailed(SavedSearch)));
}

void SynchronizationManagerPrivate::authenticate()
{
    QNDEBUG("Trying to restore persistent authentication settings...");

    ApplicationSettings & appSettings = ApplicationSettings::instance();
    QString keyGroup = "Authentication";

    QVariant tokenExpirationValue = appSettings.value(EXPIRATION_TIMESTAMP_KEY, keyGroup);
    if (tokenExpirationValue.isNull()) {
        QNDEBUG("Failed to restore authentication token's expiration timestamp, "
                "launching OAuth procedure");
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

    qevercloud::Timestamp currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    if (currentTimestamp > tokenExpirationTimestamp) {
        QNDEBUG("Authentication token has already expired, launching OAuth procedure");
        launchOAuth();
        return;
    }

    QNDEBUG("Trying to restore the authentication token...");

    QKeychain::ReadPasswordJob readPasswordJob(QApplication::applicationName() + "_authenticationToken");
    readPasswordJob.setAutoDelete(false);
    readPasswordJob.setKey(QApplication::applicationName());

    QEventLoop loop;
    readPasswordJob.connect(&readPasswordJob, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
    readPasswordJob.start();
    loop.exec();

    QKeychain::Error error = readPasswordJob.error();
    if (error == QKeychain::EntryNotFound) {
        QNDEBUG("Failed to restore the authentication token, launching OAuth procedure");
        launchOAuth();
        return;
    }
    else if (error != QKeychain::NoError) {
        QNWARNING("Attempt to read authentication token returned with error: error code " << error
                  << ", " << readPasswordJob.errorString());
        emit notifyError(readPasswordJob.errorString());
        return;
    }

    QNDEBUG("Successfully restored the autnehtication token");

    if (m_pOAuthResult.isNull()) {
        m_pOAuthResult.reset(new qevercloud::EvernoteOAuthWebView::OAuthResult);
    }

    m_pOAuthResult->authenticationToken = readPasswordJob.textData();
    m_pOAuthResult->expires = tokenExpirationTimestamp;

    QNDEBUG("Restoring persistent note store url");

    QVariant noteStoreUrlValue = appSettings.value(NOTE_STORE_URL_KEY, keyGroup);
    if (noteStoreUrlValue.isNull()) {
        QString error = QT_TR_NOOP("Persistent note store url is unexpectedly empty");
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
        QString error = QT_TR_NOOP("Persistent user id is unexpectedly empty");
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
        QString error = QT_TR_NOOP("Persistent web api url prefix is unexpectedly empty");
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

    launchSync();
}

void SynchronizationManagerPrivate::launchOAuth()
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

    m_pOAuthWebView->authenticate("sandox.evernote.com", consumerKey, consumerSecret);
}

void SynchronizationManagerPrivate::launchSync()
{
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult);

    m_pNoteStore.reset(new qevercloud::NoteStore(m_pOAuthResult->noteStoreUrl,
                                                 m_pOAuthResult->authenticationToken));

    if (m_pLastSyncState.isNull()) {
        QNDEBUG("The client has never synchronized with the remote service, "
                "performing full sync");
        launchFullSync();
        return;
    }

    qevercloud::SyncState syncState = m_pNoteStore->getSyncState(m_pOAuthResult->authenticationToken);
    if (syncState.fullSyncBefore > m_pLastSyncState->currentTime) {
        QNDEBUG("Server's current sync state's fullSyncBefore is larger that the timestamp of last sync "
                "(" << syncState.fullSyncBefore << " > " << m_pLastSyncState->currentTime << "), "
                "continue with full sync");
        launchFullSync();
        return;
    }

    if (syncState.updateCount == m_pLastSyncState->updateCount) {
        QNDEBUG("Server's current sync state's updateCount is equal to the one from the last sync "
                "(" << syncState.updateCount << "), the server has no updates, sending changes");
        sendChanges();
        return;
    }

    QNDEBUG("Performing incremental sync");
    launchIncrementalSync();
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

    QEventLoop loop;
    writePasswordJob.connect(&writePasswordJob, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
    writePasswordJob.start();
    loop.exec();

    QKeychain::Error error = writePasswordJob.error();
    if (error != QKeychain::NoError) {
        QNWARNING("Attempt to write autnehtication token failed with error: error code = " << error
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

void SynchronizationManagerPrivate::onFindUserCompleted(UserWrapper user)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindUserFailed(UserWrapper user)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindNotebookCompleted(Notebook notebook)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindNotebookFailed(Notebook notebook)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindNoteCompleted(Note note)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindNoteFailed(Note note)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindTagCompleted(Tag tag)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindTagFailed(Tag tag)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindResourceCompleted(ResourceWrapper resource)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindResourceFailed(ResourceWrapper resource)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindLinkedNotebookCompleted(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindLinkedNotebookFailed(LinkedNotebook linkedNotebook)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindSavedSearchCompleted(SavedSearch savedSearch)
{
    // TODO: implement
}

void SynchronizationManagerPrivate::onFindSavedSearchFailed(SavedSearch savedSearch)
{
    // TODO: implement
}

} // namespace qute_note
