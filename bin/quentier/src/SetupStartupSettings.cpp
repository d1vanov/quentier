/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "SetupStartupSettings.h"

#include <lib/account/AccountManager.h>
#include <lib/preferences/DefaultDisableNativeMenuBar.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/utility/ApplicationSettings.h>

#include <QCoreApplication>

namespace quentier {

void setupStartupSettings()
{
    AccountManager accountManager;
    Account account = accountManager.lastUsedAccount();

    // Process disable native menu bar preference
    bool disableNativeMenuBar = false;
    if (!account.isEmpty()) {
        ApplicationSettings appSettings(account, QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

        disableNativeMenuBar =
            appSettings
                .value(
                    DISABLE_NATIVE_MENU_BAR_SETTINGS_KEY,
                    QVariant::fromValue(defaultDisableNativeMenuBar()))
                .toBool();

        appSettings.endGroup();
    }
    else {
        disableNativeMenuBar = defaultDisableNativeMenuBar();
    }

    QCoreApplication::setAttribute(
        Qt::AA_DontUseNativeMenuBar, disableNativeMenuBar);
}

} // namespace quentier
