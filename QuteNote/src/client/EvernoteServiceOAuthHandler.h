#ifndef __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
#define __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H

#include <QObject>
#include <QString>
#include <kqoauthmanager.h>

class EvernoteServiceManager;

class KQOAuthRequest;

class EvernoteServiceOAuthHandler: public QObject
{
    Q_OBJECT
public:
    explicit EvernoteServiceOAuthHandler(QObject * parent = nullptr);
    virtual ~EvernoteServiceOAuthHandler() override;

    bool getAccess(QString & errorMessage);
    void xauth();

private:
    void getKQOAuthManagerErrorDescription(const KQOAuthManager::KQOAuthError errorCode,
                                           QString & errorMessage) const;

signals:
    void accessGranted(QString key, QString secret);
    void accessDenied(QString errorMessage);

private slots:
    void onTemporaryTokenReceived(QString temporaryToken,
                                  QString temporaryTokenSecret);
    void onAuthorizationReceived(QString token, QString verifier);
    void onAccessTokenReceived(QString token, QString tokenSecret);
    void onRequestReady(QByteArray response);

private:
    EvernoteServiceOAuthHandler() = delete;
    EvernoteServiceOAuthHandler(const EvernoteServiceOAuthHandler & other) = delete;
    EvernoteServiceOAuthHandler & operator=(const EvernoteServiceOAuthHandler & other) = delete;

private:
    EvernoteServiceManager & m_manager;
    KQOAuthManager * m_pOAuthManager;
    KQOAuthRequest * m_pOAuthRequest;
    QString m_OAuthKey;
    QString m_OAuthSecret;
};

#endif // __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
