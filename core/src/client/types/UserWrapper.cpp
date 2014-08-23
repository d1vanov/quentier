#include "UserWrapper.h"

namespace qute_note {

UserWrapper::UserWrapper(UserWrapper && other) :
    IUser(std::move(other)),
    m_qecUser(std::move(other.m_qecUser))
{}

UserWrapper & UserWrapper::operator=(UserWrapper && other)
{
    IUser::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_qecUser = std::move(other.m_qecUser);
    }

    return *this;
}

UserWrapper::~UserWrapper()
{}

const qevercloud::User & UserWrapper::GetEnUser() const
{
    return m_qecUser;
}

qevercloud::User & UserWrapper::GetEnUser()
{
    return m_qecUser;
}

} // namespace qute_note
