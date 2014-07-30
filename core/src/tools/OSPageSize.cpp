#include "OSPageSize.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace qute_note {

qint64 GetOSPageSize()
{
#ifdef Q_OS_WIN
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo (&systemInfo);
    return static_cast<qint64>(systemInfo.dwPageSize);
#else
    return static_cast<qint64>(sysconf(_SC_PAGESIZE));
#endif
}

} // namespace qute_note
