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

#include <lib/update/UpdateInfo.h>

namespace quentier::preferences::defaults {

// Won't check for updates periodically while the app is running by default
constexpr bool checkForUpdates = false;

// Will check for updates on the app's startup
constexpr bool checkForUpdatesOnStartup = true;

// Will use continuous update channel by default
constexpr bool useContinuousUpdateChannel = true;

// The default item index of a combo box with check for updates intervals
constexpr int checkForUpdatesIntervalOptionIndex = 0;

// Default update channel
constexpr const char * updateChannel = QUENTIER_DEFAULT_UPDATE_CHANNEL;

constexpr auto updateProvider = QUENTIER_DEFAULT_UPDATE_PROVIDER;

} // namespace quentier::preferences::defaults
