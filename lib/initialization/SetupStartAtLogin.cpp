/*
 * Copyright 2018-2025 Dmitry Ivanov
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

    utility::ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::startAtLoginGroup);
    auto groupCloser =
        std::optional{utility::ApplicationSettings::GroupCloser{appSettings}};

    if (appSettings.contains(preferences::keys::shouldStartAtLogin)) {
        QNDEBUG(
            "initialization",
            "Start automatically at login setting is present within settings, "
                << "nothing to do");
        return;
    }

    QNDEBUG(
        "initialization",
        "Start automatically at login setting is not present, will set it to "
            << "the default value");

    const bool shouldStartAutomaticallyAtLogin = preferences::defaults::startAtLogin;
    if (!shouldStartAutomaticallyAtLogin) {
        QNDEBUG(
            "initialization",
            "Should not start automatically at login by default");
        return;
    }

    groupCloser.reset();

    ErrorString errorDescription;
    if (Q_UNLIKELY(!setStartQuentierAtLoginOption(
            shouldStartAutomaticallyAtLogin, errorDescription,
            preferences::defaults::startAtLoginOption)))
    {
        QNWARNING(
            "initialization",
            "Failed to set Quentier to start " << "automatically at login: "
                                               << errorDescription);
    }
}

} // namespace quentier
