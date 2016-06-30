#include <quentier/types/UserAdapter.h>
#include <quentier/exception/UserAdapterAccessException.h>

namespace quentier {

UserAdapter::UserAdapter(qevercloud::User & externalEnUser) :
    IUser(),
    m_pEnUser(&externalEnUser),
    m_isConst(false)
{}

UserAdapter::UserAdapter(const qevercloud::User & externalEnUser) :
    IUser(),
    m_pEnUser(const_cast<qevercloud::User*>(&externalEnUser)),
    m_isConst(true)
{}

UserAdapter::UserAdapter(const UserAdapter & other) :
    IUser(other),
    m_pEnUser(other.m_pEnUser),
    m_isConst(other.m_isConst)
{}

UserAdapter::UserAdapter(UserAdapter && other) :
    IUser(std::move(other)),
    m_pEnUser(std::move(other.m_pEnUser)),
    m_isConst(std::move(other.m_isConst))
{}

UserAdapter::~UserAdapter()
{}

const qevercloud::User & UserAdapter::GetEnUser() const
{
    return *m_pEnUser;
}

qevercloud::User & UserAdapter::GetEnUser()
{
    if (m_isConst) {
        throw UserAdapterAccessException("Attempt to access non-const reference "
                                         "to User from constant UserAdapter");
    }

    return *m_pEnUser;
}

} // namespace quentier
