#ifndef __QUTE_NOTE__CLIENT__UTILITY_H
#define __QUTE_NOTE__CLIENT__UTILITY_H

#include <Types_types.h>
#include <Limits_constants.h>
#include <QString>

namespace qute_note {

template <class T>
bool CheckGuid(const T & guid)
{
    size_t guidSize = guid.size();

    if (guidSize < evernote::limits::g_Limits_constants.EDAM_GUID_LEN_MIN) {
        return false;
    }

    if (guidSize > evernote::limits::g_Limits_constants.EDAM_GUID_LEN_MAX) {
        return false;
    }

    return true;
}

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber);

const QString PrintableDateTimeFromTimestamp(const quint64 timestamp);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__UTILITY_H
