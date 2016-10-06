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
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include "../StackTrace.h"
#include <QDir>

namespace quentier {

SysInfo & SysInfo::GetSingleton()
{
    static SysInfo sysInfo;
    return sysInfo;
}

qint64 SysInfo::GetPageSize()
{
    return static_cast<qint64>(sysconf(_SC_PAGESIZE));
}

qint64 SysInfo::GetTotalMemoryBytes()
{
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

qint64 SysInfo::GetFreeMemoryBytes()
{
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

QString SysInfo::GetStackTrace()
{
    fpos_t pos;

    QString tmpFile = QDir::tempPath();
    tmpFile += QStringLiteral("/QuentierStackTrace.txt");

    // flush existing stderr and reopen it as file
    fflush(stderr);
    fgetpos(stderr, &pos);
    int fd = dup(fileno(stderr));
    FILE * fileHandle = freopen(tmpFile.toLocal8Bit().data(), "w", stderr);
    if (!fileHandle) {
        perror("Can't reopen stderr");
        return QString();
    }

    stacktrace::displayCurrentStackTrace();

    // revert stderr
    fflush(stderr);
    dup2(fd, fileno(stderr));
    close(fd);
    clearerr(stderr);
    fsetpos(stderr, &pos);
    fclose(fileHandle);

    QFile file(tmpFile);
    bool res = file.open(QIODevice::ReadOnly);
    if (!res) {
        return "<cannot open temporary file with stacktrace>";
    }

    QByteArray rawOutput = file.readAll();
    QString output(rawOutput);
    return output;
}

SysInfo::SysInfo() {}

SysInfo::~SysInfo() {}

} // namespace quentier
