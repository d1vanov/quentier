#include "User_p.h"

namespace qute_note {

UserPrivate & UserPrivate::operator=(const UserPrivate & other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_username = other.m_username;
        m_email = other.m_email;
        m_name = other.m_name;
        m_timezone = other.m_timezone;
        m_privilegeLevel = other.m_privilegeLevel;
        m_creationTimestamp = other.m_creationTimestamp;
        m_modificationTimestamp = other.m_modificationTimestamp;
        m_deletionTimestamp = other.m_deletionTimestamp;
        m_isActive = other.m_isActive;
    }

    return *this;
}

UserPrivate & UserPrivate::operator=(UserPrivate && other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_username = other.m_username;
        m_email = other.m_email;
        m_name = other.m_name;
        m_timezone = other.m_timezone;
        m_privilegeLevel = other.m_privilegeLevel;
        m_creationTimestamp = other.m_creationTimestamp;
        m_modificationTimestamp = other.m_modificationTimestamp;
        m_deletionTimestamp = other.m_deletionTimestamp;
        m_isActive = other.m_isActive;
    }

    return *this;
}

}
