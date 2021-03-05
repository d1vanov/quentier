/*
 * Copyright 2021 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IStartable.h"

#include <QDebug>

namespace quentier {

QDebug & operator<<(QDebug & dbg, const IStartable::StopMode stopMode)
{
    switch (stopMode)
    {
    case IStartable::StopMode::Graceful:
        dbg << "Graceful";
        break;
    case IStartable::StopMode::Forced:
        dbg << "Forced";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(stopMode) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
