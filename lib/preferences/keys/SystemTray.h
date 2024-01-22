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

#include <string_view>

namespace quentier::preferences::keys {

// Name of group within ApplicationSettings inside which preferences related
// to system tray are stored
constexpr std::string_view systemTrayGroup = "SystemTray";

// Name of preference specifying whether Quentier icon should be present
// in the system tray
constexpr std::string_view showSystemTrayIcon = "ShowSystemTrayIcon";

// Name of preference specifying whether when closing the main Quentier window
// the app should be minimized to the system tray
constexpr std::string_view closeToSystemTray = "CloseToSystemTray";

// Name of preference specifying whether when minimizing the main Quentier
// window the app should be minimized to the system tray
constexpr std::string_view minimizeToSystemTray = "MinimizeToSystemTray";

// Name of preference specifying whether Quentier should start minimized
// to system tray
constexpr std::string_view startMinimizedToSystemTray =
    "StartMinimizedToSystemTray";

// Name of preference specifying which kind of tray icon Quentier should use -
// colored one of dark monochrome or light monochrome
constexpr std::string_view systemTrayIconKind = "TrayIconKind";

// Name of preference containing the action to be taken on a single left mouse
// button click on Quentier's tray icon
constexpr std::string_view singleClickTrayAction = "SingleClickTrayAction";

// Name of preference containing the action to be taken on a single middle
// mouse button click on Quentier's tray icon
constexpr std::string_view middleClickTrayAction = "MiddleClickTrayAction";

// Name of preference containing the action to be taken on a double left mouse
// button click on Quentier's tray icon
constexpr std::string_view doubleClickTrayAction = "DoubleClickTrayAction";

// The name of environment variable allowing to override the system tray
// availability
constexpr std::string_view overrideSystemTrayAvailabilityEnvVar =
    "QUENTIER_OVERRIDE_SYSTEM_TRAY_AVAILABILITY";

} // namespace quentier::preferences::keys
