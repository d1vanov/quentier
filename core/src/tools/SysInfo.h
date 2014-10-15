#ifndef __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H
#define __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H

#include "Linkage.h"
#include <QtGlobal>

namespace qute_note {

class QUTE_NOTE_EXPORT SysInfo
{
public:
    static SysInfo & GetSingleton();

    qint64 GetPageSize();
    qint64 GetTotalMemoryBytes();
    qint64 GetFreeMemoryBytes();

    QString GetStackTrace();

private:
    SysInfo();
    SysInfo(const SysInfo & other) = delete;
    SysInfo(SysInfo && other) = delete;
    SysInfo & operator=(const SysInfo & other) = delete;
    SysInfo & operator=(SysInfo && other) = delete;
    ~SysInfo();

    qint64  m_pageSize;
    qint64  m_totalMemoryBytes;
    qint64  m_freeMemoryBytes;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H
