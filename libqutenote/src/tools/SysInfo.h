#ifndef __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H
#define __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H

#include "Linkage.h"
#include "qt4helper.h"
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
    SysInfo(const SysInfo & other) Q_DECL_DELETE;
    SysInfo(SysInfo && other) Q_DECL_DELETE;
    SysInfo & operator=(const SysInfo & other) Q_DECL_DELETE;
    SysInfo & operator=(SysInfo && other) Q_DECL_DELETE;
    ~SysInfo();

    qint64  m_pageSize;
    qint64  m_totalMemoryBytes;
    qint64  m_freeMemoryBytes;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__SYS_INFO_H
