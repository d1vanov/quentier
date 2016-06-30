#ifndef LIB_QUENTIER_UTILITY_UID_GENERATOR_H
#define LIB_QUENTIER_UTILITY_UID_GENERATOR_H

#include <quentier/utility/Linkage.h>
#include <QString>
#include <QUuid>

namespace quentier {

class QUENTIER_EXPORT UidGenerator
{
public:
    static QString Generate();
    static QString UidToString(const QUuid & uid);
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_UID_GENERATOR_H
