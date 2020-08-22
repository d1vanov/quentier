/*
 * Copyright 2019 Dmitry Ivanov
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

namespace quentier {

class IStartable
{
public:
    virtual void start() = 0;

    struct StopMode
    {
        enum type
        {
            Graceful = 0,
            Forced
        };
    };

    virtual void stop(const StopMode::type stopMode) = 0;

    virtual bool isStarted() const = 0;

    inline bool isStopped() const
    {
        return !isStarted();
    }

    virtual ~IStartable() {}
};

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_I_STARTABLE_H
