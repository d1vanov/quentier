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
