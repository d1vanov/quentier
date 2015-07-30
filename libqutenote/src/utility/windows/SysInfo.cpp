#include <qute_note/utility/SysInfo.h>
#include <QMutexLocker>
#include <QString>
#include <windows.h>

namespace qute_note {

SysInfo & SysInfo::GetSingleton()
{
    static SysInfo sysInfo;
    return sysInfo;
}

qint64 SysInfo::GetPageSize()
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo (&systemInfo);
    return static_cast<qint64>(systemInfo.dwPageSize);
}

qint64 SysInfo::GetFreeMemoryBytes()
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

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

qint64 SysInfo::GetTotalMemoryBytes()
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

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

QString SysInfo::GetStackTrace()
{
    return "<stack trace obtaining is not implemented on Windows, patches are welcome>";
}

SysInfo::SysInfo() {}

SysInfo::~SysInfo() {}

} // namespace qute_note
