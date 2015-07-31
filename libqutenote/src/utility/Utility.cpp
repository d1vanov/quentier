#include <qute_note/utility/Utility.h>
#include <limits>
#include <QDateTime>

namespace qute_note {

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber)
{
    return !( (updateSequenceNumber < 0) ||
              (updateSequenceNumber == std::numeric_limits<int32_t>::min()) ||
              (updateSequenceNumber == std::numeric_limits<int32_t>::max()) );
}

const QString PrintableDateTimeFromTimestamp(const qint64 timestamp)
{
    QString result = QString::number(timestamp);
    result += " (";
    result += QDateTime::fromMSecsSinceEpoch(timestamp).toString(Qt::ISODate);
    result += ")";

    return result;
}

} // namespace qute_note
