#ifndef __LIB_QUTE_NOTE__UTILITY__SYS_INFO_H
#define __LIB_QUTE_NOTE__UTILITY__SYS_INFO_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
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
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__SYS_INFO_H
