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

#ifndef LIB_QUENTIER_UTILITY_SYS_INFO_H
#define LIB_QUENTIER_UTILITY_SYS_INFO_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Macros.h>
#include <QScopedPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SysInfoPrivate)

class QUENTIER_EXPORT SysInfo
{
public:
    SysInfo();
    ~SysInfo();

    qint64 pageSize();
    qint64 totalMemory();
    qint64 freeMemory();

    QString stackTrace();

private:
    Q_DISABLE_COPY(SysInfo)

private:
    QScopedPointer<SysInfoPrivate> d_ptr;
    Q_DECLARE_PRIVATE(SysInfo)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_SYS_INFO_H
