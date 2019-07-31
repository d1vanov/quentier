/*
 * Copyright 2018-2019 Dmitry Ivanov
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

#include "StartAtLogin.h"

#include <lib/preferences/SettingsNames.h>
#include <lib/preferences/DefaultSettings.h>

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

std::pair<bool, StartQuentierAtLoginOption::type> isQuentierSetToStartAtLogin()
{
    QNDEBUG("isQuentierSetToStartAtLogin");

    ApplicationSettings appSettings;
    appSettings.beginGroup(START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME);
    bool shouldStartAutomaticallyAtLogin =
        appSettings.value(SHOULD_START_AUTOMATICALLY_AT_LOGIN).toBool();
    QVariant startAutomaticallyAtLoginOptionData =
        appSettings.value(START_AUTOMATICALLY_AT_LOGIN_OPTION);
    appSettings.endGroup();

    if (!shouldStartAutomaticallyAtLogin) {
        QNDEBUG("Starting automatically at login is switched off");
        return std::pair<bool, StartQuentierAtLoginOption::type>(
            false,
            StartQuentierAtLoginOption::MinimizedToTray);
    }

    StartQuentierAtLoginOption::type option =
        DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION;

    bool conversionResult = false;
    int startAutomaticallyAtLoginOptionInt =
        startAutomaticallyAtLoginOptionData.toInt(&conversionResult);
    if (conversionResult)
    {
        switch(startAutomaticallyAtLoginOptionInt)
        {
        case StartQuentierAtLoginOption::MinimizedToTray:
            option = StartQuentierAtLoginOption::MinimizedToTray;
            break;
        case StartQuentierAtLoginOption::Minimized:
            option = StartQuentierAtLoginOption::Minimized;
            break;
        case StartQuentierAtLoginOption::Normal:
            option = StartQuentierAtLoginOption::Normal;
            break;
        default:
            {
                QNWARNING("Detected invalid start Quentier "
                          << "automatically at login option value: "
                          << startAutomaticallyAtLoginOptionInt
                          << ", fallback to the default option of "
                          << DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION);
                break;
            }
        }
    }
    else
    {
        QNWARNING("Failed to convert start Quentier automatically "
                  << "at login option from app settings to int! "
                  << "Fallback to the default option of "
                  << DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION);
    }

    QNDEBUG("Should start Quentier automatically at login, option = " << option);
    return std::pair<bool, StartQuentierAtLoginOption::type>(true, option);
}

} // namespace quentier
