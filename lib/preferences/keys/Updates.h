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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_UPDATES_H
#define QUENTIER_LIB_PREFERENCES_KEYS_UPDATES_H

namespace quentier {
namespace preferences {
namespace keys {

// Name of group within ApplicationSettings inside which preferences related
// to updates are stored
constexpr const char * checkForUpdatesGroup = "CheckForUpdates";

// Name of preference specifying whether Quentier should check for updates
// on its own or not
constexpr const char * checkForUpdates = "CheckForUpdates";

// Name of preference specifyiing whether Quentier should check for updates
// on startup
constexpr const char * checkForUpdatesOnStartup = "CheckForUpdatesOnStartup";

// Name of preference specifyiing whether Quentier should check for updates
// using continuous update channel in addition to tagged releases
constexpr const char * useContinuousUpdateChannel =
    "UseContinuousUpdateChannel";

// Name of preference containing the interval between checks for updates
// in milliseconds
constexpr const char * checkForUpdatesInterval = "CheckForUpdatesInterval";

// Name of preference corresponding to the used updates provider - GitHub
// releases, AppImage updates of other
constexpr const char * checkForUpdatesProvider = "CheckForUpdatesProvider";

// Name of preference corresponding to the used updates channel - stable,
// unstable, etc.
constexpr const char * checkForUpdatesChannel = "CheckForUpdatesChannel";

// Name of technical preference (not really a preference but a value persisted
// alongside preferences) containing the timestamp of the last check for updates
constexpr const char * lastCheckForUpdatesTimestamp =
    "LastCheckForUpdatesTimestamp";

} // keys
} // preferences
} // namespace quentier

#endif // QUENTIER_LIB_PREFERENCES_KEYS_UPDATES_H
