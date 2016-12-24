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

#include <quentier/utility/SysInfo.h>
#include "../SysInfo_p.h"
#include <QMutexLocker>
#include <QString>
#include <windows.h>

namespace quentier {

qint64 SysInfo::pageSize()
{
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo (&systemInfo);
    return static_cast<qint64>(systemInfo.dwPageSize);
}

qint64 SysInfo::freeMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    MEMORYSTATUSEX memory_status;
    ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
    memory_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memory_status)) {
        return static_cast<qint64>(memory_status.ullAvailPhys);
    }
    else {
        return -1;
    }
}

qint64 SysInfo::totalMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    MEMORYSTATUSEX memory_status;
    ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
    memory_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memory_status)) {
        return static_cast<qint64>(memory_status.ullTotalPhys);
    }
    else {
        return -1;
    }
}

QString SysInfo::stackTrace()
{
    return QStringLiteral("Stack trace obtaining is not implemented on Windows, patches are welcome");
}

} // namespace quentier
