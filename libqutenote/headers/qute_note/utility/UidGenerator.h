#ifndef LIB_QUTE_NOTE_UTILITY_UID_GENERATOR_H
#define LIB_QUTE_NOTE_UTILITY_UID_GENERATOR_H

#include <qute_note/utility/Linkage.h>
#include <QString>
#include <QUuid>

namespace qute_note {

class QUTE_NOTE_EXPORT UidGenerator
{
public:
    static QString Generate();
    static QString UidToString(const QUuid & uid);
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_UTILITY_UID_GENERATOR_H
