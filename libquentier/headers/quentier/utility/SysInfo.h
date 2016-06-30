#ifndef LIB_QUENTIER_UTILITY_SYS_INFO_H
#define LIB_QUENTIER_UTILITY_SYS_INFO_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <QtGlobal>

namespace quentier {

class QUENTIER_EXPORT SysInfo
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

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_SYS_INFO_H
