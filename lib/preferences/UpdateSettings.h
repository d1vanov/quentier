/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_UPDATE_SETTINGS_H
#define QUENTIER_LIB_PREFERENCES_UPDATE_SETTINGS_H

#include <QString>

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

enum class UpdateProvider
{
    GITHUB = 0,
    APPIMAGE = 1
};

void readPersistentUpdateSettings(
    bool & checkForUpdatesEnabled,
    bool & shouldCheckForUpdatesOnStartup,
    bool & useContinuousUpdateChannel,
    int & checkForUpdatesIntervalOption,
    QString & updateChannel,
    UpdateProvider & updateProvider);

qint64 checkForUpdatesIntervalMsecFromOption(
    const CheckForUpdatesInterval option);

QString updateProviderName(
    const UpdateProvider updateProvider);

} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_UPDATE_SETTINGS_H
