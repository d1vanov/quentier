#include "UserWrapper.h"

namespace qute_note {

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
