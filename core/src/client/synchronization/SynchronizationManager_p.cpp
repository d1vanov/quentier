#include "SynchronizationManager_p.h"
#include <keychain.h>
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>
#include <Simplecrypt.h>
#include <QApplication>

namespace qute_note {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(LocalStorageManagerThread & localStorageManagerThread) :
    m_pLastSyncState(),
    m_pOAuthWebView(new qevercloud::EvernoteOAuthWebView),
    m_pOAuthResult()
{
    connect(localStorageManagerThread);
    // TODO: implement
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{}

void SynchronizationManagerPrivate::synchronize()
{
    // TODO: implement
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
    if (error == QKeychain::NoError) {
        QNDEBUG("Successfully wrote authentication token to the keychain");
        // TODO: continue the synchronization after authentication
        return;
    }
    else {
        QNWARNING("Attempt to write autnehtication token failed with error: error code = " << error
                  << ", " << writePasswordJob.errorString());
        emit notifyError(writePasswordJob.errorString());
    }

    // TODO: store the expiraton time of the token in some simple place like QSettings
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

void SynchronizationManagerPrivate::connect(LocalStorageManagerThread & localStorageManagerThread)
{
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFinished(bool)), this, SLOT(onOAuthResult(bool)));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationSuceeded), this, SLOT(onOAuthSuccess()));
    QObject::connect(m_pOAuthWebView.data(), SIGNAL(authenticationFailed), this, SLOT(onOAuthFailure()));

    // TODO: implement
    Q_UNUSED(localStorageManagerThread)
}

void SynchronizationManagerPrivate::authenticate()
{
    // TODO: implement
    // Plan:
    // 1) Check with qtkeychain whether the authentication token is available
    // 2) If not, manually call the slot which should continue the synchronization procedure
    // 3) Otherwise, initialize OAuth authentication using QEverCloud facilities

    // TODO: retrieve the persistently stored autentication token expiration time,
    // see whether the token stored previously would still be valid, if not,
    // launch the OAuth procedure right away

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
        QNDEBUG("Failed to restore the authentication token, launching the OAuth authorization procedure");
        // TODO: launch the OAuth authorization procedure
    }
    else if (error == QKeychain::NoError) {
        QNDEBUG("Successfully restored the autnehtication token");
        // TODO: manually invoke the slot which should continue the synchronization process
    }
    else {
        QNWARNING("Attempt to read authentication token returned with error: error code " << error
                  << ", " << readPasswordJob.errorString());
        emit notifyError(readPasswordJob.errorString());
    }
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

} // namespace qute_note
