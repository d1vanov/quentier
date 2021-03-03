/*
 * Copyright 2018-2021 Dmitry Ivanov
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

#include <lib/preferences/defaults/StartAtLogin.h>
#include <lib/preferences/keys/StartAtLogin.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QDebug>

namespace quentier {

std::pair<bool, StartQuentierAtLoginOption> isQuentierSetToStartAtLogin()
{
    QNDEBUG("utility", "isQuentierSetToStartAtLogin");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup);

    bool shouldStartAutomaticallyAtLogin =
        appSettings.value(preferences::keys::shouldStartAtLogin).toBool();

    auto startAutomaticallyAtLoginOptionData =
        appSettings.value(preferences::keys::startAtLoginOption);

    appSettings.endGroup();

    if (!shouldStartAutomaticallyAtLogin) {
        QNDEBUG("utility", "Starting automatically at login is switched off");
        return std::make_pair(
            false, StartQuentierAtLoginOption::MinimizedToTray);
    }

    auto option = preferences::defaults::startAtLoginOption;
    bool conversionResult = false;

    const int startAutomaticallyAtLoginOptionInt =
        startAutomaticallyAtLoginOptionData.toInt(&conversionResult);

    if (conversionResult) {
        switch (static_cast<StartQuentierAtLoginOption>(
                startAutomaticallyAtLoginOptionInt)) {
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
            QNWARNING(
                "utility",
                "Detected invalid start Quentier "
                    << "automatically at login option value: "
                    << startAutomaticallyAtLoginOptionInt
                    << ", fallback to the default option of "
                    << preferences::defaults::startAtLoginOption);
            break;
        }
        }
    }
    else {
        QNWARNING(
            "utility",
            "Failed to convert start Quentier automatically "
                << "at login option from app settings to int! "
                << "Fallback to the default option of "
                << preferences::defaults::startAtLoginOption);
    }

    QNDEBUG(
        "utility",
        "Should start Quentier automatically at login, option = " << option);

    return std::make_pair(true, option);
}

QDebug & operator<<(QDebug & dbg, const StartQuentierAtLoginOption option)
{
    switch(option)
    {
    case StartQuentierAtLoginOption::MinimizedToTray:
        dbg << "Minimized to tray";
        break;
    case StartQuentierAtLoginOption::Minimized:
        dbg << "Minimized";
        break;
    case StartQuentierAtLoginOption::Normal:
        dbg << "Normal";
        break;
    default:
        dbg << "Unknown (" << static_cast<qint64>(option) << ")";
        break;
    }

    return dbg;
}

} // namespace quentier
