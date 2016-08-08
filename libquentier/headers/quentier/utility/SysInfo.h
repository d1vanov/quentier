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
#include <quentier/utility/Qt4Helper.h>
#include <QtGlobal>

namespace quentier {

class QUENTIER_EXPORT SysInfo
{
public:
    static SysInfo & GetSingleton();

    qint64 GetPageSize();
    qint64 GetTotalMemoryBytes();
    qint64 GetFreeMemoryBytes();

    QString GetStackTrace();

private:
    SysInfo();
    SysInfo(const SysInfo & other) Q_DECL_EQ_DELETE;
    SysInfo(SysInfo && other) Q_DECL_EQ_DELETE;
    SysInfo & operator=(const SysInfo & other) Q_DECL_EQ_DELETE;
    SysInfo & operator=(SysInfo && other) Q_DECL_EQ_DELETE;
    ~SysInfo();
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_SYS_INFO_H
