#ifndef __LIB_QUTE_NOTE__UTILITY__UTILITY_H
#define __LIB_QUTE_NOTE__UTILITY__UTILITY_H

#include <QEverCloud.h>
#include <QString>
#include <QFlags>
#include <cstdint>

#define SEC_TO_MSEC(sec) (sec * 100)
#define SIX_HOURS_IN_MSEC 2160000

namespace qute_note {

template <class T>
bool CheckGuid(const T & guid)
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

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber);

const QString PrintableDateTimeFromTimestamp(const qint64 timestamp);

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__UTILITY_H
