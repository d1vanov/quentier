#include "EvernoteServiceOAuthHandler.h"
#include "EvernoteServiceManager.h"
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
    QObject(parent)
{
    m_pOAuthRequest = new KQOAuthRequest;
    m_pOAuthManager = new KQOAuthManager(this);

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

void EvernoteServiceOAuthHandler::getAccess()
{
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onTemporaryTokenReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(authorizationReceived(QString,QString)),
                     this, SLOT(onAuthorizationReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onAccessTokenReceived(QString,QString)));
    QObject::connect(m_pOAuthManager, SIGNAL(requestReady(QByteArray)),
                     this, SLOT(onRequestReady(QByteArray)));

    m_pOAuthRequest->initRequest(KQOAuthRequest::TemporaryCredentials,
                                 QUrl("https://sandbox.evernote.com/oauth"));
    m_pOAuthRequest->setConsumerKey("d-ivanov-1988");
    m_pOAuthRequest->setConsumerSecretKey("57dc98f899b77cd9");

    m_pOAuthManager->setHandleUserAuthorization(true);

    if (!m_pOAuthRequest->isValid()) {
        qDebug() << "Invalid request";
        QMessageBox::warning(0,"Failed to Authorize","getAccess :: Invalid request");
        // TODO: notify EvernoteServiceManager object, access it via singleton
    }

    m_pOAuthManager->executeRequest(m_pOAuthRequest);
}

void EvernoteServiceOAuthHandler::xauth()
{
    QObject::connect(m_pOAuthManager, SIGNAL(accessTokenReceived(QString,QString)),
                     this, SLOT(onAccessTokenReceived(QString,QString)));

    KQOAuthRequest_XAuth *oauthRequest = new KQOAuthRequest_XAuth(this);
    oauthRequest->initRequest(KQOAuthRequest::AccessToken, QUrl("https://sandbox.evernote.com/oauth"));
    oauthRequest->setConsumerKey("d-ivanov-1988");
    oauthRequest->setConsumerSecretKey("57dc98f899b77cd9");
    oauthRequest->setXAuthLogin("d_ivanov_1988", "rjhg^djky^lefk^1988");

    oauthManager->executeRequest(oauthRequest);
}

// TODO: continue from here
