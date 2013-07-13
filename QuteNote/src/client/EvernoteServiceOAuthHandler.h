#ifndef __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
#define __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <kqoauthmanager.h>

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
                                           const char *& errorMessage) const;

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
    KQOAuthManager * m_pOAuthManager;
    KQOAuthRequest * m_pOAuthRequest;
    QSettings m_OAuthSettings;
};

#endif // __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
