/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "data/UserWrapperData.h"
#include <quentier/types/UserWrapper.h>

namespace quentier {

UserWrapper::UserWrapper() :
    IUser(),
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

UserWrapper::UserWrapper(const qevercloud::User & qecUser) :
    IUser(),
    d(new UserWrapperData)
{
    d->m_qecUser = qecUser;
}

UserWrapper::UserWrapper(qevercloud::User && qecUser) :
    IUser(),
    d(new UserWrapperData)
{
    d->m_qecUser = std::move(qecUser);
}

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
