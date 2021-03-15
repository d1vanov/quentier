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

#ifndef QUENTIER_LIB_PREFERENCES_DEFAULTS_START_AT_LOGIN_H
#define QUENTIER_LIB_PREFERENCES_DEFAULTS_START_AT_LOGIN_H

#include <lib/utility/StartAtLogin.h>

namespace quentier::preferences::defaults {

// Will set up Quentier to start automatically on logging into the system
// by default
constexpr bool startAtLogin = true;

// Will set up Quentier to start at login being minimized to system tray
// by default
constexpr auto startAtLoginOption = StartQuentierAtLoginOption::MinimizedToTray;

} // namespace quentier::preferences::defaults

#endif // QUENTIER_LIB_PREFERENCES_DEFAULTS_START_AT_LOGIN_H
