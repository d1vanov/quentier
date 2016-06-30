#include "data/UserWrapperData.h"
#include <quentier/types/UserWrapper.h>

namespace quentier {

UserWrapper::UserWrapper() :
    d(new UserWrapperData)
{}

UserWrapper::UserWrapper(const UserWrapper & other) :
    IUser(other),
    d(other.d)
{}

UserWrapper::UserWrapper(UserWrapper && other) :
    IUser(std::move(other)),
    d(other.d)
{}

UserWrapper & UserWrapper::operator=(const UserWrapper & other)
{
    IUser::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

UserWrapper & UserWrapper::operator=(UserWrapper && other)
{
    IUser::operator=(std::move(other));

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

UserWrapper::~UserWrapper()
{}

const qevercloud::User & UserWrapper::GetEnUser() const
{
    return d->m_qecUser;
}

qevercloud::User & UserWrapper::GetEnUser()
{
    return d->m_qecUser;
}

} // namespace quentier
