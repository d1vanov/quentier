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
    m_manager(EvernoteServiceManager::Instance()),
    m_pOAuthManager(new KQOAuthManager(this)),
    m_pOAuthRequest(new KQOAuthRequest)
{
    m_pOAuthRequest->setEnableDebugOutput(false);
    m_pOAuthManager->setHandleUserAuthorization(true);
    m_pOAuthManager->setHandleAuthorizationPageOpening(false);

    QObject::connect(this, SIGNAL(accessGranted(std::pair<QString,QString>)),
                     &m_manager, SLOT(onOAuthSuccess(std::pair<QString,QString>)));
    QObject::connect(this, SIGNAL(accessDenied(QString)),
                     &m_manager, SLOT(onOAuthFailure(QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(authorizationPageRequested(QUrl)),
                     &m_manager, SLOT(onRequestToShowAuthorizationPage(QUrl)));
}

EvernoteServiceOAuthHandler::~EvernoteServiceOAuthHandler()
{
    if (m_pOAuthRequest != nullptr) {
        delete m_pOAuthRequest;
        m_pOAuthRequest = nullptr;
    }

    if (m_pOAuthManager != nullptr) {
        m_pOAuthManager->disconnect(this, SLOT(onTemporaryTokenReceived(QString,QString)));
        m_pOAuthManager->disconnect(this, SLOT(onAuthorizationReceived(QString,QString)));
        m_pOAuthManager->disconnect(this, SLOT(onAccessTokenReceived(QString,QString)));
        m_pOAuthManager->disconnect(this, SLOT(onRequestReady(QByteArray)));
        m_pOAuthManager->disconnect(&m_manager, SLOT(onRequestToShowAuthorizationPage(QUrl)));
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
    m_manager.GetHostName(evernoteHostname);

    m_pOAuthRequest->initRequest(KQOAuthRequest::TemporaryCredentials,
                                 QUrl(evernoteHostname + QString("/oauth")));
    const CredentialsModel & credentials = m_manager.getCredentials();
    m_pOAuthRequest->setConsumerKey(credentials.GetConsumerKey());
    m_pOAuthRequest->setConsumerSecretKey(credentials.GetConsumerSecret());

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
    m_manager.GetHostName(evernoteHostname);

    KQOAuthRequest_XAuth * pOAuthRequest = new KQOAuthRequest_XAuth(this);
    pOAuthRequest->initRequest(KQOAuthRequest::AccessToken,
                               QUrl(evernoteHostname + QString("/oauth")));

    const CredentialsModel & credentials = m_manager.getCredentials();

    pOAuthRequest->setConsumerKey(credentials.GetConsumerKey());
    pOAuthRequest->setConsumerSecretKey(credentials.GetConsumerSecret());
    pOAuthRequest->setXAuthLogin(credentials.GetUsername(),
                                 credentials.GetPassword());

    m_pOAuthManager->executeRequest(pOAuthRequest);
}

void EvernoteServiceOAuthHandler::getKQOAuthManagerErrorDescription(const KQOAuthManager::KQOAuthError errorCode,
                                                                    QString & errorMessage) const
{
    switch(errorCode)
    {
    case KQOAuthManager::NetworkError:
        errorMessage = tr("network error");
        break;
    case KQOAuthManager::RequestEndpointError:
        errorMessage = tr("request endpoint error");
        break;
    case KQOAuthManager::RequestValidationError:
        errorMessage = tr("request is invalid");
        break;
    case KQOAuthManager::RequestUnauthorized:
        errorMessage = tr("request is unauthorized");
        break;
    case KQOAuthManager::RequestError:
        errorMessage = tr("request is invalid (NULL)");
        break;
    case KQOAuthManager::ManagerError:
        errorMessage = tr("KQOAuth manager error");
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
    m_manager.GetHostName(evernoteHostname);

    QUrl userAuthURL(evernoteHostname + QString("/OAuth.action"));

    KQOAuthManager::KQOAuthError lastError = m_pOAuthManager->lastError();
    if (lastError == KQOAuthManager::NoError)
    {
        qDebug() << "Asking for user's permission to access protected resources. "
                 << "Opening URL: " << userAuthURL;
        m_pOAuthManager->getUserAuthorization(userAuthURL);
    }
    else
    {
        QString errorMessage;
        getKQOAuthManagerErrorDescription(lastError, errorMessage);
        errorMessage.prepend(tr("Failed to get user authorization: "));

        qDebug() << errorMessage;
        emit accessDenied(errorMessage);
    }
}

void EvernoteServiceOAuthHandler::onAuthorizationReceived(QString token,
                                                          QString verifier)
{
    qDebug() << "User authorization received: token: " << token
             << ", verifier:: " << verifier;

    QString evernoteHostname;
    m_manager.GetHostName(evernoteHostname);

    QUrl userAuthURL(evernoteHostname + QString("/oauth"));

    m_pOAuthManager->getUserAccessTokens(userAuthURL);
    KQOAuthManager::KQOAuthError lastError = m_pOAuthManager->lastError();
    if (lastError != KQOAuthManager::NoError)
    {
        QString errorMessage;
        getKQOAuthManagerErrorDescription(lastError, errorMessage);
        errorMessage.prepend(tr("Failed to get user access tokens: "));

        qDebug() << errorMessage;
        emit accessDenied(errorMessage);
    }
}

void EvernoteServiceOAuthHandler::onAccessTokenReceived(QString token,
                                                        QString tokenSecret)
{
    qDebug() << "Access token received: token: " << token
             << ", token secret: " << tokenSecret;

    m_OAuthTokens.first = token;
    m_OAuthTokens.second = tokenSecret;

    emit accessGranted(m_OAuthTokens);
}

void EvernoteServiceOAuthHandler::onRequestReady(QByteArray response)
{
    qDebug() << "Response from the service: " << response;

    if (response.isEmpty()) {
        QString errorMessage = tr("Got empty response from the server");
        emit accessDenied(errorMessage);
    }
}
