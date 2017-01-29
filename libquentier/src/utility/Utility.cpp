/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/Utility.h>
#include <quentier/types/RegisterMetatypes.h>
#include <limits>
#include <QDateTime>

namespace quentier {

bool checkUpdateSequenceNumber(const int32_t updateSequenceNumber)
{
    return !( (updateSequenceNumber < 0) ||
              (updateSequenceNumber == std::numeric_limits<int32_t>::min()) ||
              (updateSequenceNumber == std::numeric_limits<int32_t>::max()) );
}

const QString printableDateTimeFromTimestamp(const qint64 timestamp)
{
    QString result = QString::number(timestamp);
    result += QStringLiteral(" (");
    result += QDateTime::fromMSecsSinceEpoch(timestamp).toString(Qt::ISODate);
    result += QStringLiteral(")");

    return result;
}

void initializeLibquentier()
{
    registerMetatypes();
}

} // namespace quentier
