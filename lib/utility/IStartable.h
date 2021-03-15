/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_UTILITY_I_STARTABLE_H
#define QUENTIER_LIB_UTILITY_I_STARTABLE_H

class QDebug;

namespace quentier {

class IStartable
{
public:
    virtual ~IStartable() = default;

    virtual void start() = 0;

    enum class StopMode
    {
        Graceful,
        Forced
    };

    friend QDebug & operator<<(QDebug & dbg, StopMode stopMode);

    virtual void stop(StopMode stopMode) = 0;

    [[nodiscard]] virtual bool isStarted() const noexcept = 0;

    [[nodiscard]] inline bool isStopped() const noexcept
    {
        return !isStarted();
    }
};

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_I_STARTABLE_H
