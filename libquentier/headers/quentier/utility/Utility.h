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

#ifndef LIB_QUENTIER_UTILITY_UTILITY_H
#define LIB_QUENTIER_UTILITY_UTILITY_H

#include <quentier/utility/Linkage.h>
#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QFlags>
#include <cstdint>

#define SEC_TO_MSEC(sec) (sec * 100)
#define SIX_HOURS_IN_MSEC 2160000

namespace quentier {

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

bool QUENTIER_EXPORT checkUpdateSequenceNumber(const int32_t updateSequenceNumber);

const QString QUENTIER_EXPORT printableDateTimeFromTimestamp(const qint64 timestamp);

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_UTILITY_H
