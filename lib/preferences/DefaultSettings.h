/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_DEFAULT_SETTINGS_H
#define QUENTIER_LIB_PREFERENCES_DEFAULT_SETTINGS_H

#include <lib/utility/StartAtLogin.h>
#include <lib/utility/VersionInfo.h>

#include <QtGlobal>

#define DEFAULT_SHOW_SYSTEM_TRAY_ICON (true)
#define DEFAULT_CLOSE_TO_SYSTEM_TRAY (true)
#define DEFAULT_MINIMIZE_TO_SYSTEM_TRAY (false)

#define DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY (false)

#define DEFAULT_SHOW_NOTE_THUMBNAILS (true)

#define DEFAULT_SHOULD_START_AUTOMATICALLY_AT_LOGIN_OPTION (true)

#define DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION                            \
    (StartQuentierAtLoginOption::MinimizedToTray)                              \
// DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION

#ifdef Q_WS_MAC
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("dark")
#else
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("colored")
#endif

#define DEFAULT_REMOVE_EMPTY_UNEDITED_NOTES (true)
#define DEFAULT_EDITOR_CONVERT_TO_NOTE_TIMEOUT (500)
#define DEFAULT_EXPUNGE_NOTE_TIMEOUT (500)

#define DEFAULT_DOWNLOAD_NOTE_THUMBNAILS (true)
#define DEFAULT_DOWNLOAD_INK_NOTE_IMAGES (true)

#define DEFAULT_RUN_SYNC_EACH_NUM_MINUTES (15)

#define DEFAULT_CHECK_FOR_UPDATES (true)
#define DEFAULT_CHECK_FOR_UPDATES_ON_STARTUP (true)
#define DEFAULT_UPDATES_CHANNEL (QStringLiteral("stable"))
#define DEFAULT_USE_CONTINUOUS_UPDATE_CHANNEL (true)
#define DEFAULT_CHECK_FOR_UPDATES_INTERVAL_OPTION_INDEX (0)

#if QUENTIER_PACKAGED_AS_APP_IMAGE
#define DEFAULT_UPDATES_PROVIDER (QStringLiteral("AppImage"))
#else
#define DEFAULT_UPDATES_PROVIDER (QStringLiteral("GitHub releases"))
#endif

#endif // QUENTIER_LIB_PREFERENCES_DEFAULT_SETTINGS_H
