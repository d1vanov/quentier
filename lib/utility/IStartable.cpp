/*
 * Copyright 2024 Dmitry Ivanov
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
#include <QTextStream>

namespace quentier {

namespace {

template <class T>
void printStopMode(const IStartable::StopMode stopMode, T & t)
{
    switch (stopMode)
    {
    case IStartable::StopMode::Graceful:
        t << "Graceful";
        break;
    case IStartable::StopMode::Forced:
        t << "Forced";
        break;
    }
}

} // namespace

QDebug & operator<<(const IStartable::StopMode stopMode, QDebug & dbg)
{
    printStopMode(stopMode, dbg);
    return dbg;
}

QTextStream & operator<<(const IStartable::StopMode stopMode, QTextStream & strm)
{
    printStopMode(stopMode, strm);
    return strm;
}

} // namespace quentier
