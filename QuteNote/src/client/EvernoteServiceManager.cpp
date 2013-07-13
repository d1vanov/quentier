#include "EvernoteServiceManager.h"
#include "../tools/Singleton.h"

EvernoteServiceManager::EvernoteServiceManager()
{
    m_evernoteHostName = "https://sandbox.evernote.com";
}

EvernoteServiceManager & EvernoteServiceManager::Instance()
{
    return qutenote_tools::Singleton<EvernoteServiceManager>::Instance();
}

bool EvernoteServiceManager::setCredentials(const CredentialsModel & credentials,
                                            const char *& errorMessage)
{
    if (credentials.Empty(errorMessage)) {
        return false;
    }

    m_credentials = credentials;
    return true;
}

void EvernoteServiceManager::GetHostName(QString & hostname) const
{
    hostname = m_evernoteHostName;
}
