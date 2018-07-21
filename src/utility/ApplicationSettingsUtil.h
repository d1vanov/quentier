/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_UTILITY_APPLICATION_SETTINGS_UTIL_H
#define QUENTIER_UTILITY_APPLICATION_SETTINGS_UTIL_H

#include <quentier/utility/ApplicationSettings.h>


namespace quentier {


// helpers to avoid duplicating code when accessing settings
// could be later moved directly to ApplicationSettings

/**
 * Get application setting.
 *
 * @param account Current account.
 * @param settingsName Settings name.
 * @param settingsGroup Group name.
 * @param settingsKey Key name.
 * @param defaultValue Default value.
 * @return Settings value (or default if it was not set before).
 */
QVariant getApplicationSetting(
    const Account & account,
    const QString & settingsName,
    const QString & settingsGroup,
    const QString & settingsKey,
    const QVariant & defaultValue);

/**
 * Get application setting.
 *
 * @param appSettings Setting instance.
 * @param settingsGroup Group name.
 * @param settingsKey Key name.
 * @param defaultValue Default value.
 * @return Settings value (or default if it was not set before).
 */
QVariant getApplicationSetting(
    ApplicationSettings & appSettings,
    const QString & settingsGroup,
    const QString & settingsKey,
    const QVariant & defaultValue);



/**
 * Set application setting.
 *
 * @param account Current account.
 * @param settingsName Settings name.
 * @param settingsGroup Group name.
 * @param settingsKey Key name.
 * @param value Value to set.
 */
void setApplicationSetting(
    const Account &account,
    const QString &settingsName,
    const QString &settingsGroup,
    const QString &settingsKey,
    const QVariant &value);

} // namespace quentier

#endif // QUENTIER_UTILITY_APPLICATION_SETTINGS_UTIL_H
