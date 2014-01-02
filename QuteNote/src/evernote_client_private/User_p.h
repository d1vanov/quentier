#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__USER_PRIVATE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__USER_PRIVATE_H

#include <ctime>
#include <QString>

namespace qute_note {

class UserPrivate
{
public:
    UserPrivate() = default;
    UserPrivate(const UserPrivate & other) = default;
    UserPrivate(UserPrivate && other) = default;
    UserPrivate & operator=(const UserPrivate & other);
    UserPrivate & operator=(UserPrivate && other);

    int32_t   m_id;
    QString   m_username;
    QString   m_email;
    QString   m_name;
    QString   m_timezone;
    uint32_t  m_privilegeLevel;
    time_t    m_creationTimestamp;
    time_t    m_modificationTimestamp;
    time_t    m_deletionTimestamp;
    bool      m_isActive;

};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__USER_PRIVATE_H
