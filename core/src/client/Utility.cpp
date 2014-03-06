#include "Utility.h"
#include <QDateTime>
#include <Limits_constants.h>

namespace qute_note {

bool CheckGuid(const evernote::edam::Guid & guid)
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

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber)
{
    return ( (updateSequenceNumber < 0) ||
             (updateSequenceNumber == std::numeric_limits<int32_t>::min()) ||
             (updateSequenceNumber == std::numeric_limits<int32_t>::max()) );
}

const QString PrintableDateTimeFromTimestamp(const evernote::edam::Timestamp timestamp)
{
    return std::move(QDateTime::fromMSecsSinceEpoch(timestamp).toString(Qt::ISODate));
}

} // namespace qute_note
