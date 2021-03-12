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

#include "SetupStartAtLogin.h"

#include <lib/preferences/defaults/StartAtLogin.h>
#include <lib/preferences/keys/StartAtLogin.h>
#include <lib/utility/StartAtLogin.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/ApplicationSettings.h>

namespace quentier {

void setupStartQuentierAtLogin()
{
    QNDEBUG("initialization", "setupStartQuentierAtLogin");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup);
    if (appSettings.contains(preferences::keys::shouldStartAtLogin)) {
        QNDEBUG(
            "initialization",
            "Start automatically at login setting is present within settings, "
                << "nothing to do");
        appSettings.endGroup();
        return;
    }

    QNDEBUG(
        "initialization",
        "Start automatically at login setting is not present, will set it to "
            << "the default value");

    const bool shouldStartAutomaticallyAtLogin =
        preferences::defaults::startAtLogin;
    if (!shouldStartAutomaticallyAtLogin) {
        QNDEBUG(
            "initialization",
            "Should not start automatically at login by default");
        appSettings.endGroup();
        return;
    }

    appSettings.endGroup();

    const auto option = preferences::defaults::startAtLoginOption;

    ErrorString errorDescription;

    const bool res = setStartQuentierAtLoginOption(
        shouldStartAutomaticallyAtLogin, errorDescription, option);

    if (Q_UNLIKELY(!res)) {
        QNWARNING(
            "initialization",
            "Failed to set Quentier to start automatically at login: "
                << errorDescription);
    }
}

} // namespace quentier
