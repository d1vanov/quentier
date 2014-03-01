#include "UserWrapper.h"

namespace qute_note {

UserWrapper::UserWrapper() :
    IUser()
{}

UserWrapper::UserWrapper(const UserWrapper & other) :
    IUser(other),
    m_enUser(other.m_enUser)
{}

UserWrapper & UserWrapper::operator=(const UserWrapper & other)
{
    if (this != &other)
    {
        if (other.IsDirty()) {
            SetDirty();
        }
        else {
            SetClean();
        }

        if (other.IsLocal()) {
            SetLocal();
        }
        else {
            SetNonLocal();
        }

        m_enUser = other.m_enUser;
    }

    return *this;
}

UserWrapper::~UserWrapper()
{}

const evernote::edam::User & UserWrapper::GetEnUser() const
{
    return m_enUser;
}

evernote::edam::User & UserWrapper::GetEnUser()
{
    return m_enUser;
}

} // namespace qute_note
