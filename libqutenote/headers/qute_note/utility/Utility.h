#ifndef LIB_QUTE_NOTE_UTILITY_UTILITY_H
#define LIB_QUTE_NOTE_UTILITY_UTILITY_H

#include <qute_note/utility/Linkage.h>
#include <QEverCloud.h>
#include <QString>
#include <QFlags>
#include <cstdint>

#define SEC_TO_MSEC(sec) (sec * 100)
#define SIX_HOURS_IN_MSEC 2160000

namespace qute_note {

template <class T>
bool checkGuid(const T & guid)
{
    qint32 guidSize = static_cast<qint32>(guid.size());

    if (guidSize < qevercloud::EDAM_GUID_LEN_MIN) {
        return false;
    }

    if (guidSize > qevercloud::EDAM_GUID_LEN_MAX) {
        return false;
    }

    return true;
}

bool QUTE_NOTE_EXPORT checkUpdateSequenceNumber(const int32_t updateSequenceNumber);

const QString QUTE_NOTE_EXPORT printableDateTimeFromTimestamp(const qint64 timestamp);

} // namespace qute_note

#endif // LIB_QUTE_NOTE_UTILITY_UTILITY_H
