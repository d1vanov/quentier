#include "User.h"
#include "../evernote_client_private/User_p.h"

namespace qute_note {

User::User() :
    d_ptr(new UserPrivate)
{}

User::User(const User & other) :
    d_ptr(new UserPrivate(*(other.d_ptr)))
{}

User::User(User && other) :
    d_ptr(nullptr)
{
    d_ptr.swap(other.d_ptr);
}

User & User::operator=(const User & other)
{
    if (this != &other) {
        *d_ptr = *(other.d_ptr);
    }

    return *this;
}

User & User::operator=(User && other)
{
    if (this != &other) {
        d_ptr.swap(other.d_ptr);
    }

    return *this;
}

User::~User()
{}

int32_t User::id() const
{
    Q_D(const User);
    return d->m_id;
}

void User::setId(const int32_t id)
{
    Q_D(User);
    d->m_id = id;
}

const QString User::username() const
{
    Q_D(const User);
    return d->m_username;
}

void User::setUsername(const QString & username)
{
    Q_D(User);
    d->m_username = username;
}

const QString User::email() const
{
    Q_D(const User);
    return d->m_email;
}

void User::setEmail(const QString & email)
{
    Q_D(User);
    d->m_email = email;
}

const QString User::name() const
{
    Q_D(const User);
    return d->m_name;
}

void User::setName(const QString & name)
{
    Q_D(User);
    d->m_name = name;
}

const QString User::timezone() const
{
    Q_D(const User);
    return d->m_timezone;
}

void User::setTimezone(const QString & timezone)
{
    Q_D(User);
    d->m_timezone = timezone;
}

uint32_t User::privilegeLevel() const
{
    Q_D(const User);
    return d->m_privilegeLevel;
}

time_t User::creationTimestamp() const
{
    Q_D(const User);
    return d->m_creationTimestamp;
}

void User::setCreationTimestamp(const time_t creationTimestamp)
{
    Q_D(User);
    d->m_creationTimestamp = creationTimestamp;
}

time_t User::modificationTimestamp() const
{
    Q_D(const User);
    return d->m_modificationTimestamp;
}

void User::setModificationTimestamp(const time_t modificationTimestamp)
{
    Q_D(User);
    d->m_modificationTimestamp = modificationTimestamp;
}

time_t User::deletionTimestamp() const
{
    Q_D(const User);
    return d->m_deletionTimestamp;
}

void User::setDeletionTimestamp(const time_t deletionTimestamp)
{
    Q_D(User);
    d->m_deletionTimestamp = deletionTimestamp;
}

bool User::isActive() const
{
    Q_D(const User);
    return d->m_isActive;
}

void User::setActive(const bool is_active)
{
    Q_D(User);
    d->m_isActive = is_active;
}

QTextStream & User::Print(QTextStream & strm) const
{
    Q_D(const User);

    strm << "User {\n" << " username (login) = " << d->m_username << ", \n";
    strm << "email = " << d->m_email << ", \n";
    strm << "name = " << d->m_name << ", \n";
    strm << "timezone = " << d->m_timezone << ", \n";
    strm << "privilege level = " << d->m_privilegeLevel << ", \n";
    strm << "creation timestamp = " << d->m_creationTimestamp << ", \n";
    strm << "modification timestamp = " << d->m_modificationTimestamp << ", \n";
    strm << "deletion timestamp = " << d->m_deletionTimestamp << ", \n";
    strm << "isActive = " << (d->m_isActive ? "true" : "false") << "\n }; \n";

    return strm;
}

}
