#include "UserAdapter.h"
#include "UserAdapterAccessException.h"

namespace qute_note {

UserAdapter::UserAdapter(evernote::edam::User & externalEnUser) :
    IUser(),
    m_pEnUser(&externalEnUser),
    m_isConst(false)
{}

UserAdapter::UserAdapter(const evernote::edam::User & externalEnUser) :
    IUser(),
    m_pEnUser(const_cast<evernote::edam::User*>(&externalEnUser)),
    m_isConst(true)
{}

UserAdapter::UserAdapter(const UserAdapter & other) :
    IUser(other),
    m_pEnUser(other.m_pEnUser),
    m_isConst(other.m_isConst)
{}

UserAdapter::~UserAdapter()
{}

const evernote::edam::User & UserAdapter::GetEnUser() const
{
    return *m_pEnUser;
}

evernote::edam::User & UserAdapter::GetEnUser()
{
    if (m_isConst) {
        throw UserAdapterAccessException("Attempt to access non-const reference "
                                         "to User from constant UserAdapter");
    }

    return *m_pEnUser;
}

} // namespace qute_note
