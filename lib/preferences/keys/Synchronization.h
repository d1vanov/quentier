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

#ifndef QUENTIER_LIB_PREFERENCES_KEYS_SYNCHRONIZATION_H
#define QUENTIER_LIB_PREFERENCES_KEYS_SYNCHRONIZATION_H

namespace quentier::preferences::keys {

// Name of group within ApplicationSettings inside which preferences related
// to synchronization are stored
constexpr const char * synchronizationGroup = "SynchronizationSettings";

// Name of preference specifying whether Quentier should download note
// thumbnails during the synchronization
constexpr const char * downloadNoteThumbnails = "DownloadNoteThumbnails";

// Name of preference specifying whether Quentier should download ink note
// images during the synchronization
constexpr const char * downloadInkNoteImages = "DownloadInkNoteImages";

// Name of preference specifying the period in minutes after which Quentier
// should repeatedly run sync
constexpr const char * runSyncPeriodMinutes = "RunSyncEachNumMinutes";

// Name of group within ApplicationSettings inside which preferences related
// to synchronization network proxy settings are stored
constexpr const char * syncNetworkProxyGroup =
    "SynchronizationNetworkProxySettings";

// Name of preference specifying the type of network proxy which Quentier should
// use for synchronization
constexpr const char * syncNetworkProxyType = "SynchronizationNetworkProxyType";

// Name of preference specifying the host of network proxy which Quentier should
// use for synchronization
constexpr const char * syncNetworkProxyHost = "SynchronizationNetworkProxyHost";

// Name of preference specifying the port of network proxy which Quentier should
// use for synchronization
constexpr const char * syncNetworkProxyPort = "SynchronizationNetworkProxyPort";

// Name of preference specifying the username of network proxy which Quentier
// should use for synchronization
constexpr const char * syncNetworkProxyUser = "SynchronizationNetworkProxyUser";

// Name of preference specifying the password of network proxy which Quentier
// should use for synchronization
constexpr const char * syncNetworkProxyPassword =
    "SynchronizationNetworkProxyPassword";

} // namespace quentier::preferences::keys

#endif // QUENTIER_LIB_PREFERENCES_KEYS_SYNCHRONIZATION_H
