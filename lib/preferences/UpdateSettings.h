/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include <QString>

class QDebug;
class QTextStream;

namespace quentier {

enum class CheckForUpdatesInterval
{
    FIFTEEN_MINUTES = 0,
    HALF_AN_HOUR = 1,
    HOUR = 2,
    TWO_HOURS = 3,
    FOUR_HOURS = 4,
    DAILY = 5,
    WEEKLY = 6,
    MONTHLY = 7
};

QTextStream & operator<<(
    QTextStream & strm, const CheckForUpdatesInterval interval);

QDebug & operator<<(QDebug & dbg, const CheckForUpdatesInterval interval);

enum class UpdateProvider
{
    GITHUB = 0,
    APPIMAGE = 1
};

QTextStream & operator<<(QTextStream & strm, const UpdateProvider provider);
QDebug & operator<<(QDebug & dbg, const UpdateProvider provider);

void readPersistentUpdateSettings(
    bool & checkForUpdatesEnabled, bool & shouldCheckForUpdatesOnStartup,
    bool & useContinuousUpdateChannel, int & checkForUpdatesIntervalOption,
    QString & updateChannel, UpdateProvider & updateProvider);

[[nodiscard]] qint64 checkForUpdatesIntervalMsecFromOption(
    const CheckForUpdatesInterval option);

[[nodiscard]] QString updateProviderName(const UpdateProvider updateProvider);

} // namespace quentier
