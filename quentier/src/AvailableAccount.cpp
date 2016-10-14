#include "AvailableAccount.h"

AvailableAccount::AvailableAccount() :
    m_username(),
    m_userPersistenceStoragePath(),
    m_isLocal(true)
{}

AvailableAccount::AvailableAccount(const QString & username,
                                   const QString & userPersistenceStoragePath,
                                   const bool isLocal) :
    m_username(username),
    m_userPersistenceStoragePath(userPersistenceStoragePath),
    m_isLocal(isLocal)
{}
