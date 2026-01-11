/*
 * Copyright 2020-2025 Dmitry Ivanov
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

// Name of group within utility::ApplicationSettings inside which preferences
// related to updates are stored
constexpr std::string_view checkForUpdatesGroup = "CheckForUpdates";

// Name of preference specifying whether Quentier should check for updates
// on its own or not
constexpr std::string_view checkForUpdates = "CheckForUpdates";

// Name of preference specifyiing whether Quentier should check for updates
// on startup
constexpr std::string_view checkForUpdatesOnStartup =
    "CheckForUpdatesOnStartup";

// Name of preference specifyiing whether Quentier should check for updates
// using continuous update channel in addition to tagged releases
constexpr std::string_view useContinuousUpdateChannel =
    "UseContinuousUpdateChannel";

// Name of preference containing the interval between checks for updates
// in milliseconds
constexpr std::string_view checkForUpdatesInterval = "CheckForUpdatesInterval";

// Name of preference corresponding to the used updates provider - GitHub
// releases, AppImage updates of other
constexpr std::string_view checkForUpdatesProvider = "CheckForUpdatesProvider";

// Name of preference corresponding to the used updates channel - stable,
// unstable, etc.
constexpr std::string_view checkForUpdatesChannel = "CheckForUpdatesChannel";

} // namespace quentier::preferences::keys
