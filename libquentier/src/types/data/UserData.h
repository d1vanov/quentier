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

#ifndef LIB_QUENTIER_TYPES_DATA_USER_DATA_H
#define LIB_QUENTIER_TYPES_DATA_USER_DATA_H

#include <quentier/utility/Macros.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QSharedData>

namespace quentier {

class UserData: public QSharedData
{
public:
    UserData();
    UserData(const UserData & other);
    UserData(UserData && other);
    UserData(const qevercloud::User & user);
    virtual ~UserData();

    qevercloud::User    m_qecUser;
    bool                m_isLocal;
    bool                m_isDirty;

private:
    UserData & operator=(const UserData & other) Q_DECL_EQ_DELETE;
    UserData & operator=(UserData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_USER_DATA_H
