#ifndef __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
#define __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H

#include <QObject>
#include <QString>
#include <QSettings>

class KQOAuthManager;
class KQOAuthRequest;

class EvernoteServiceOAuthHandler: public QObject
{
    Q_OBJECT
public:
    explicit EvernoteServiceOAuthHandler(QObject * parent = nullptr);
    virtual ~EvernoteServiceOAuthHandler() override;

    void getAccess();
    void xauth();

private slots:
    void onTemporaryTokenReceived(QString temporaryToken,
                                  QString temporaryTokenSecret);
    void onAuthorizationReceived(QString token, QString verifier);
    void onAccessTokenReceived(QString token, QString tokenSecret);
    void onAuthorizedRequestDone();
    void onRequestReady(QByteArray);

private:
    KQOAuthManager * m_pOAuthManager;
    KQOAuthRequest * m_pOAuthRequest;
    QSettings m_OAuthSettings;
};

#endif // __QUTE_NOTE__EVERNOTE_SERVICE_OAUTH_HANDLER_H
