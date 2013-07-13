#ifndef __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
#define __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H

#include <QString>
#include <QSettings>
#include "../tools/Singleton.h"
#include "CredentialsModel.h"

class EvernoteServiceManager
{
private:
    // standard constructor does nothing but exists as private method
    EvernoteServiceManager();

public:
    friend class qutenote_tools::Singleton<EvernoteServiceManager>;
    static EvernoteServiceManager & Instance();

    bool setCredentials(const CredentialsModel & credentials,
                        const char *& errorMessage);

    CredentialsModel & getCredentials() { return m_credentials; }
    const CredentialsModel & getCredentials() const { return m_credentials; }

//    /**
//     * @brief Authenticate - attempts to authenticate to Evernote service
//     * @param username
//     * @param password
//     * @param err_str - contains error message if authentication was not successful
//     * @return true if authentication was successful, false otherwise
//     */
//    bool Authenticate(const std::string & username,
//                      const std::string & password,
//                      const char *& err_str);

    bool CheckAuthentication(const char *&) const { /* TODO: implement */ return true; }

    void GetHostName(QString & hostname) const;

    // Intefrace for OAuth handler
    void notifyAuthorizationSuccess(const QSettings &) { /* TODO: implement */ }
    void notifyAuthorizationFailure(const char *&) { /* TODO: implement */ }

private:
    EvernoteServiceManager(const EvernoteServiceManager & other) = delete;
    EvernoteServiceManager & operator=(const EvernoteServiceManager & other) = delete;

private:
    enum EAuthorizationState
    {
        EAS_AUTHORIZED,
        EAS_UNAUTHORIZED_NEVER_ATTEMPTED,
        EAS_UNAUTHORIZED_CREDENTIALS_REJECTED,
        EAS_UNAUTHORIZED_QUIT
    };

private:
    CredentialsModel     m_credentials;
    EAuthorizationState  m_authorizationState;
    QString              m_evernoteHostName;
    QSettings            m_OAuthSettings;
};

#endif // __QUTE_NOTE__CLIENT__EVERNOTE_SERVICE_MANAGER_H
