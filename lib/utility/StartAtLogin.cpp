/*
 * Copyright 2018-2024 Dmitry Ivanov
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
#include <QTextStream>

namespace quentier {

namespace {

template <class T>
void printStartQuentierAtLoginOption(
    const StartQuentierAtLoginOption option, T & t)
{
    switch (option) {
    case StartQuentierAtLoginOption::MinimizedToTray:
        t << "minimized to tray";
        break;
    case StartQuentierAtLoginOption::Minimized:
        t << "minimized";
        break;
    case StartQuentierAtLoginOption::Normal:
        t << "normal";
        break;
    }
}

} // namespace

std::pair<bool, StartQuentierAtLoginOption> isQuentierSetToStartAtLogin()
{
    QNDEBUG("utility", "isQuentierSetToStartAtLogin");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup.data());
    ApplicationSettings::GroupCloser groupCloser{appSettings};

    bool shouldStartAutomaticallyAtLogin =
        appSettings.value(preferences::keys::shouldStartAtLogin.data())
            .toBool();

    auto startAutomaticallyAtLoginOptionData =
        appSettings.value(preferences::keys::startAtLoginOption.data());

    if (!shouldStartAutomaticallyAtLogin) {
        QNDEBUG("utility", "Starting automatically at login is switched off");
        return std::make_pair(
            false, StartQuentierAtLoginOption::MinimizedToTray);
    }

    StartQuentierAtLoginOption option =
        preferences::defaults::startAtLoginOption;

    bool conversionResult = false;

    int startAutomaticallyAtLoginOptionInt =
        startAutomaticallyAtLoginOptionData.toInt(&conversionResult);

    if (conversionResult) {
        switch (startAutomaticallyAtLoginOptionInt) {
        case static_cast<int>(StartQuentierAtLoginOption::MinimizedToTray):
            option = StartQuentierAtLoginOption::MinimizedToTray;
            break;
        case static_cast<int>(StartQuentierAtLoginOption::Minimized):
            option = StartQuentierAtLoginOption::Minimized;
            break;
        case static_cast<int>(StartQuentierAtLoginOption::Normal):
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

    return std::pair{true, option};
}

QDebug & operator<<(QDebug & dbg, const StartQuentierAtLoginOption option)
{
    printStartQuentierAtLoginOption(option, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const StartQuentierAtLoginOption option)
{
    printStartQuentierAtLoginOption(option, strm);
    return strm;
}

} // namespace quentier
