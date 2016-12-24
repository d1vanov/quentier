#ifndef LIBQUENTIER_UTILITY_SYS_INFO_PRIVATE_H
#define LIBQUENTIER_UTILITY_SYS_INFO_PRIVATE_H

#include <QMutex>

namespace quentier {

class SysInfoPrivate
{
public:
    SysInfoPrivate() :
        m_mutex()
    {}

    QMutex  m_mutex;
};

} // namespace quentier

#endif // LIBQUENTIER_UTILITY_SYS_INFO_PRIVATE_H
