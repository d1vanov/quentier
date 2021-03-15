/*
 * Copyright 2020-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_DEFAULTS_SYSTEM_TRAY_H
#define QUENTIER_LIB_PREFERENCES_DEFAULTS_SYSTEM_TRAY_H

namespace quentier::preferences::defaults {

// Will display Quentier's icon in system tray by default
constexpr bool showSystemTrayIcon = true;

// Will close Quentier to system tray instead of quitting the app by default
constexpr bool closeToSystemTray = true;

// Won't minimize Quentier to system tray instead of task bar by default
constexpr bool minimizeToSystemTray = false;

// Won't start Quentier minimized to system tray by default
constexpr bool startMinimizedToSystemTray = false;

// Default system tray icon kind
constexpr const char * trayIconKind =
#ifdef Q_WS_MAC
    "dark"
#else
    "colored"
#endif
    ;

} // namespace quentier::preferences::defaults

#endif // QUENTIER_LIB_PREFERENCES_DEFAULTS_SYSTEM_TRAY_H
