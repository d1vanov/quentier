#include "../SysInfo.h"
#include <unistd.h>
#include <sys/sysinfo.h>

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
    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.totalram);
}

qint64 SysInfo::GetFreeMemoryBytes()
{
    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.freeram);
}

SysInfo::SysInfo() {}

SysInfo::~SysInfo() {}

} // namespace qute_note
