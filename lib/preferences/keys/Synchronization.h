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
// related to synchronization are stored
constexpr std::string_view synchronizationGroup = "SynchronizationSettings";

// Name of preference specifying whether Quentier should download note
// thumbnails during the synchronization
constexpr std::string_view downloadNoteThumbnails = "DownloadNoteThumbnails";

// Name of preference specifying whether Quentier should download ink note
// images during the synchronization
constexpr std::string_view downloadInkNoteImages = "DownloadInkNoteImages";

// Name of preference specifying the period in minutes after which Quentier
// should repeatedly run sync
constexpr std::string_view runSyncPeriodMinutes = "RunSyncEachNumMinutes";

// Name of group within utility::ApplicationSettings inside which preferences
// related to synchronization network proxy settings are stored
constexpr std::string_view syncNetworkProxyGroup =
    "SynchronizationNetworkProxySettings";

// Name of preference specifying the type of network proxy which Quentier should
// use for synchronization
constexpr std::string_view syncNetworkProxyType =
    "SynchronizationNetworkProxyType";

// Name of preference specifying the host of network proxy which Quentier should
// use for synchronization
constexpr std::string_view syncNetworkProxyHost =
    "SynchronizationNetworkProxyHost";

// Name of preference specifying the port of network proxy which Quentier should
// use for synchronization
constexpr std::string_view syncNetworkProxyPort =
    "SynchronizationNetworkProxyPort";

// Name of preference specifying the username of network proxy which Quentier
// should use for synchronization
constexpr std::string_view syncNetworkProxyUser =
    "SynchronizationNetworkProxyUser";

// Name of preference specifying the password of network proxy which Quentier
// should use for synchronization
constexpr std::string_view syncNetworkProxyPassword =
    "SynchronizationNetworkProxyPassword";

// Name of preference specifying whether the app should run synchronization
// on startup
constexpr std::string_view runSyncOnStartup = "SynchronizationRunSyncOnStartup";

} // namespace quentier::preferences::keys
