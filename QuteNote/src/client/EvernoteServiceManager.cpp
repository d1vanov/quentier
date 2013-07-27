#include "EvernoteServiceManager.h"
#include "../gui/MainWindow.h"
#include "../gui/EvernoteOAuthBrowser.h"
#include "../tools/Singleton.h"
#include "../../SimpleCrypt/src/simplecrypt.h"
#include <QFile>
#include <QMessageBox>

void EvernoteServiceManager::connect()
{
    // TODO: implement
}

void EvernoteServiceManager::disconnect()
{
    // TODO: implement
}

EvernoteServiceManager::EvernoteServiceManager() :
    m_credentials(this),
    m_authorizationState(EAS_UNAUTHORIZED_NEVER_ATTEMPTED),
    m_evernoteHostName("https://sandbox.evernote.com")
{}

EvernoteServiceManager & EvernoteServiceManager::Instance()
{
    return Singleton<EvernoteServiceManager>::Instance();
}

bool EvernoteServiceManager::setCredentials(const CredentialsModel & credentials,
                                            QString & errorMessage)
{
    if (credentials.Empty(errorMessage)) {
        return false;
    }

    m_credentials = credentials;
    return true;
}

bool EvernoteServiceManager::CheckAuthenticationState(QString & errorMessage) const
{
    switch(m_authorizationState)
    {
    case EAS_AUTHORIZED:
        return true;
    case EAS_UNAUTHORIZED_NEVER_ATTEMPTED:
        errorMessage = tr("Not authorized yet");
        return false;
    case EAS_UNAUTHORIZED_CREDENTIALS_REJECTED:
        errorMessage = tr("Last authorization attempt failed: credentials rejected");
        return false;
    case EAS_UNAUTHORIZED_QUIT:
        errorMessage = tr("Not yet authorized after last quit");
        return false;
    case EAS_UNAUTHORIZED_INTERNAL_ERROR:
        errorMessage = tr("Not authorized due to internal error, please contact developers");
        return false;
    default:
        errorMessage = tr("Not authorized: unknown error, please contact developers");
        return false;
    }
}

void EvernoteServiceManager::GetHostName(QString & hostname) const
{
    hostname = m_evernoteHostName;
}

void EvernoteServiceManager::onOAuthSuccess(QString key, QString secret)
{
    SimpleCrypt crypto(CredentialsModel::RANDOM_CRYPTO_KEY);
    QString encryptedKey = crypto.encryptToString(key);
    QString encryptedSecret = crypto.encryptToString(secret);
    QFile tokensFile(":/enc_data/oautk.dat");
    if (!tokensFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::information(0, tr("Error: cannot open file with OAuth tokens: "),
                                 tokensFile.errorString());
        m_authorizationState = EAS_UNAUTHORIZED_INTERNAL_ERROR;
        emit statusText(tr("Got OAuth tokens but unable to save them encrypted in file"), 0);
        return;
    }

    tokensFile.write(encryptedKey.toAscii());
    tokensFile.write(encryptedSecret.toAscii());
    tokensFile.close();

    m_authorizationState = EAS_AUTHORIZED;
    emit statusText("Successfully authenticated to Evernote!", 2000);
}

void EvernoteServiceManager::onOAuthFailure(QString message)
{
    // TODO: think twice how should I deduce the state here
    m_authorizationState = EAS_UNAUTHORIZED_CREDENTIALS_REJECTED;
    emit statusText("Unable to authenticate to Evernote: " + message, 0);
}

void EvernoteServiceManager::onRequestToShowAuthorizationPage(QUrl authUrl)
{
    emit showAuthWebPage(authUrl);
}

void EvernoteServiceManager::onConsumerKeyAndSecretSet(QString key, QString secret)
{
    m_credentials.SetConsumerKey(key);
    m_credentials.SetConsumerSecret(secret);
}

void EvernoteServiceManager::onUserNameAndPasswordSet(QString name, QString password)
{
    m_credentials.SetUsername(name);
    m_credentials.SetPassword(password);
}
