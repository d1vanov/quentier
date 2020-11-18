/*
 * Copyright 2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_START_AT_LOGIN_H
#define QUENTIER_LIB_PREFERENCES_KEYS_START_AT_LOGIN_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to starting at login are stored
constexpr const char * startAtLoginGroup = "StartAutomaticallyAtLogin";

// Key for preference specifying whether Quentier should start automatically
// at login
constexpr const char * shouldStartAtLogin = "ShouldStartAutomaticallyAtLogin";

// Key for preference specifying how exactly Quentier should start at login:
// minimized, minimized to tray etc.
constexpr const char * startAtLoginOption = "StartAutomaticallyAtLoginOption";

} // namespace keys
} // namespace preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_START_AT_LOGIN_H
