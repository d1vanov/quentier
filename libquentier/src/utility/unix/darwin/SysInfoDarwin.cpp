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
#include "../../SysInfo_p.h"
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include "../StackTrace.h"
#include <QString>
#include <QMutexLocker>

namespace quentier {

qint64 SysInfo::totalMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    int mib[2];
    int64_t physical_memory;
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    size_t length = sizeof(int64_t);
    int rc = sysctl(mib, 2, &physical_memory, &length, NULL, 0);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(physical_memory);
}

qint64 SysInfo::freeMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics_data_t vm_stats;

    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics(mach_port, HOST_VM_INFO,
                                        (host_info_t)&vm_stats, &count))
    {
        return static_cast<qint64>(vm_stats.free_count * static_cast<qint64>(page_size));
    }
    else
    {
        return -1;
    }
}

} // namespace quentier
