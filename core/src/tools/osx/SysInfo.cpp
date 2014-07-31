#include "../SysInfo.h"
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

namespace qute_note {

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

SysInfo::SysInfo() {}

SysInfo::~SysInfo() {}

} // namespace qute_note
