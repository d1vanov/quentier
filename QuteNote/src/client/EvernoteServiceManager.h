#ifndef __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include "CredentialsModel.h"

template <class T>
class Singleton;

class EvernoteOAuthBrowser;

class EvernoteServiceManager: public QObject
{
    Q_OBJECT
public:
    // User interface

    /**
     * @brief connect - attempts to connect to Evernote service using
     * prespecified credentials and obtained OAuth tokens. Emits
     * statusText in case of both success and failure
     */
    void connect();

    /**
     * @brief disconnect - disconnects from Evernote service. Emits statusText.
     */
    void disconnect();

private:
    // standard constructor does nothing but exists as private method
    EvernoteServiceManager();
    friend class Singleton<EvernoteServiceManager>;

public:
    // Application interface
    static EvernoteServiceManager & Instance();

    bool setCredentials(const CredentialsModel & credentials,
                        QString & errorMessage);

    CredentialsModel & getCredentials() { return m_credentials; }
    const CredentialsModel & getCredentials() const { return m_credentials; }

    bool CheckAuthenticationState(QString & errorMessage) const;

    void GetHostName(QString & hostname) const;

signals:
    void statusText(QString, const int);
    void showAuthWebPage(QUrl);

public slots:
    void onOAuthSuccess(QString key, QString secret);
    void onOAuthFailure(QString message);
    void onRequestToShowAuthorizationPage(QUrl authUrl);
    void onConsumerKeyAndSecretSet(QString key, QString secret);
    void onUserNameAndPasswordSet(QString name, QString password);

private:
    EvernoteServiceManager(const EvernoteServiceManager & other) = delete;
    EvernoteServiceManager & operator=(const EvernoteServiceManager & other) = delete;

private:
    enum EAuthorizationState
    {
        EAS_AUTHORIZED,
        EAS_UNAUTHORIZED_NEVER_ATTEMPTED,
        EAS_UNAUTHORIZED_CREDENTIALS_REJECTED,
        EAS_UNAUTHORIZED_QUIT,
        EAS_UNAUTHORIZED_INTERNAL_ERROR
    };

private:
    CredentialsModel     m_credentials;
    EAuthorizationState  m_authorizationState;
    QString              m_evernoteHostName;
    std::pair<QString,QString> m_OAuthTokens;
};

#endif // __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
