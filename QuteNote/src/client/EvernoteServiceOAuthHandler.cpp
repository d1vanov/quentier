#include "EvernoteServiceOAuthHandler.h"
#include "EvernoteServiceManager.h"
#include <sstream>
#include <QtCore>
#if QT_VERSION < 0x050000
#include <QCoreApplication>
#else
#include <QApplication>
#endif
#include <QStringList>
#include <QDebug>
#include <kqoauthrequest.h>
#include <kqoauthrequest_xauth.h>
#include <kqoauthmanager.h>

EvernoteServiceOAuthHandler::EvernoteServiceOAuthHandler(QObject * parent) :
    QObject(parent),
    m_pOAuthManager(new KQOAuthManager(this)),
    m_pOAuthRequest(new KQOAuthRequest)
{
    m_pOAuthRequest->setEnableDebugOutput(false);
}

EvernoteServiceOAuthHandler::~EvernoteServiceOAuthHandler()
{
    if (m_pOAuthRequest != nullptr) {
        delete m_pOAuthRequest;
    }

    if (m_pOAuthManager != nullptr) {
        delete m_pOAuthManager;
    }
}

bool EvernoteServiceOAuthHandler::getAccess(QString & errorMessage)
{
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onTemporaryTokenReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(authorizationReceived(QString,QString)),
                     this, SLOT(onAuthorizationReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onAccessTokenReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(requestReady(QByteArray)),
                     this, SLOT(onRequestReady(QByteArray)));

    QString evernoteHostname;
    EvernoteServiceManager::Instance().GetHostName(evernoteHostname);

    m_pOAuthRequest->initRequest(KQOAuthRequest::TemporaryCredentials,
                                 QUrl(evernoteHostname + QString("/oauth")));
    const CredentialsModel & credentials = EvernoteServiceManager::Instance().getCredentials();
    m_pOAuthRequest->setConsumerKey(QString(credentials.GetConsumerKey()));
    m_pOAuthRequest->setConsumerSecretKey(QString(credentials.GetConsumerSecret()));

    m_pOAuthManager->setHandleUserAuthorization(true);

    if (!m_pOAuthRequest->isValid()) {
        errorMessage = "Failed to authorize: invalid request";
        return false;
    }

    m_pOAuthManager->executeRequest(m_pOAuthRequest);
    return true;
}

void EvernoteServiceOAuthHandler::xauth()
{
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onAccessTokenReceived(QString,QString)));

    QString evernoteHostname;
    EvernoteServiceManager::Instance().GetHostName(evernoteHostname);

    KQOAuthRequest_XAuth * pOAuthRequest = new KQOAuthRequest_XAuth(this);
    pOAuthRequest->initRequest(KQOAuthRequest::AccessToken,
                               QUrl(evernoteHostname + QString("/oauth")));

    const CredentialsModel & credentials = EvernoteServiceManager::Instance().getCredentials();

    pOAuthRequest->setConsumerKey(QString(credentials.GetConsumerKey()));
    pOAuthRequest->setConsumerSecretKey(QString(credentials.GetConsumerSecret()));
    pOAuthRequest->setXAuthLogin(QString(credentials.GetUsername()),
                                 QString(credentials.GetPassword()));

    m_pOAuthManager->executeRequest(pOAuthRequest);
}

void EvernoteServiceOAuthHandler::getKQOAuthManagerErrorDescription(const KQOAuthManager::KQOAuthError errorCode,
                                                                    const char *& errorMessage) const
{
    switch(errorCode)
    {
    case KQOAuthManager::NetworkError:
        errorMessage = "network error";
        break;
    case KQOAuthManager::RequestEndpointError:
        errorMessage = "request endpoint error";
        break;
    case KQOAuthManager::RequestValidationError:
        errorMessage = "request is invalid";
        break;
    case KQOAuthManager::RequestUnauthorized:
        errorMessage = "request is unauthorized";
        break;
    case KQOAuthManager::RequestError:
        errorMessage = "request is invalid (NULL)";
        break;
    case KQOAuthManager::ManagerError:
        errorMessage = "KQOAuth manager error";
        break;
    case KQOAuthManager::NoError:
        break;
    }
}

void EvernoteServiceOAuthHandler::onTemporaryTokenReceived(QString temporaryToken,
                                                           QString temporaryTokenSecret)
{
    qDebug() << "Temporary token received: token: " << temporaryToken
             << ", token secret: " << temporaryTokenSecret;

    QString evernoteHostname;
    EvernoteServiceManager::Instance().GetHostName(evernoteHostname);

    QUrl userAuthURL(QString(evernoteHostname) + QString("/OAuth.action"));

    KQOAuthManager::KQOAuthError lastError = m_pOAuthManager->lastError();
    if (lastError == KQOAuthManager::NoError)
    {
        qDebug() << "Asking for user's permission to access protected resources. "
                 << "Opening URL: " << userAuthURL;
        m_pOAuthManager->getUserAuthorization(userAuthURL);
    }
    else
    {
        const char * errorMessage = nullptr;
        std::stringstream strm;
        strm << "Failed to get user authorization: ";

        getKQOAuthManagerErrorDescription(lastError, errorMessage);
        strm << errorMessage;
        errorMessage = strm.str().c_str();

        qDebug() << errorMessage;
        EvernoteServiceManager::Instance().notifyAuthorizationFailure(errorMessage);
    }
}

void EvernoteServiceOAuthHandler::onAuthorizationReceived(QString token,
                                                          QString verifier)
{
    qDebug() << "User authorization received: token: " << token
             << ", verifier:: " << verifier;

    QString evernoteHostname;
    EvernoteServiceManager::Instance().GetHostName(evernoteHostname);

    QUrl userAuthURL(QString(evernoteHostname) + QString("/oauth"));

    m_pOAuthManager->getUserAccessTokens(userAuthURL);
    KQOAuthManager::KQOAuthError lastError = m_pOAuthManager->lastError();
    if (lastError != KQOAuthManager::NoError)
    {
        const char * errorMessage = nullptr;
        std::stringstream strm;
        strm << "Failed to get user access tokens: ";

        getKQOAuthManagerErrorDescription(lastError, errorMessage);
        strm << errorMessage;
        errorMessage = strm.str().c_str();

        qDebug() << errorMessage;
        EvernoteServiceManager::Instance().notifyAuthorizationFailure(errorMessage);
    }
}

void EvernoteServiceOAuthHandler::onAccessTokenReceived(QString token,
                                                        QString tokenSecret)
{
    qDebug() << "Access token received: token: " << token
             << ", token secret: " << tokenSecret;

    m_OAuthSettings.setValue("oauth_token", token);
    m_OAuthSettings.setValue("oauth_token_secret", tokenSecret);

    EvernoteServiceManager::Instance().notifyAuthorizationSuccess(m_OAuthSettings);
}

void EvernoteServiceOAuthHandler::onRequestReady(QByteArray response)
{
    qDebug() << "Response from the service: " << response;

    if (response == "") {
        const char * error = "Got empty response from the server";
        EvernoteServiceManager::Instance().notifyAuthorizationFailure(error);
    }
}

// TODO: continue from here
