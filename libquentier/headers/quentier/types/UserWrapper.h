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

#ifndef LIB_QUENTIER_TYPES_USER_WRAPPER_H
#define LIB_QUENTIER_TYPES_USER_WRAPPER_H

#include "IUser.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(UserWrapperData)

/**
 * @brief The UserWrapper class creates and manages its own instance of
 * qevercloud::User object
 */
class QUENTIER_EXPORT UserWrapper: public IUser
{
public:
    UserWrapper();
    UserWrapper(const UserWrapper & other);
    UserWrapper(UserWrapper && other);
    UserWrapper(const qevercloud::User & qecUser);
    UserWrapper(qevercloud::User && qecUser);
    UserWrapper & operator=(const UserWrapper & other);
    UserWrapper & operator=(UserWrapper && other);
    virtual ~UserWrapper();

private:
    virtual const qevercloud::User & GetEnUser() const Q_DECL_OVERRIDE;
    virtual qevercloud::User & GetEnUser() Q_DECL_OVERRIDE;

    QSharedDataPointer<UserWrapperData> d;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::UserWrapper)

#endif // LIB_QUENTIER_TYPES_USER_WRAPPER_H
