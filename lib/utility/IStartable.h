/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#pragma once

class QDebug;
class QTextStream;

namespace quentier {

class IStartable
{
public:
    virtual ~IStartable() = default;

    virtual void start() = 0;

    enum class StopMode
    {
        Graceful = 0,
        Forced
    };

    friend QDebug & operator<<(QDebug & dbg, StopMode stopMode);
    friend QTextStream & operator<<(
        QTextStream & strm, IStartable::StopMode stopMode);

    virtual void stop(const StopMode stopMode) = 0;
    [[nodiscard]] virtual bool isStarted() const = 0;

    [[nodiscard]] inline bool isStopped() const noexcept
    {
        return !isStarted();
    }
};

} // namespace quentier
