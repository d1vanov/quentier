#ifndef __QUTE_NOTE__CLIENT__UTILITY_H
#define __QUTE_NOTE__CLIENT__UTILITY_H

#include <QString>
#include <QEverCloud.h>

namespace qute_note {

template <class T>
bool CheckGuid(const T & guid)
{
    size_t guidSize = guid.size();

    if (guidSize < qevercloud::EDAM_GUID_LEN_MIN) {
        return false;
    }

    if (guidSize > qevercloud::EDAM_GUID_LEN_MAX) {
        return false;
    }

    return true;
}

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber);

const QString PrintableDateTimeFromTimestamp(const quint64 timestamp);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__UTILITY_H
