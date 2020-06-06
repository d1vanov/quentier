/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifdef WITH_UPDATE_MANAGER
#include "UpdateSettings.h"

#include <lib/update/UpdateInfo.h>
#endif

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

#ifdef WITH_UPDATE_MANAGER
#define DEFAULT_CHECK_FOR_UPDATES (false)
#define DEFAULT_CHECK_FOR_UPDATES_ON_STARTUP (true)
#define DEFAULT_UPDATE_CHANNEL QUENTIER_DEFAULT_UPDATE_CHANNEL
#define DEFAULT_UPDATE_PROVIDER QUENTIER_DEFAULT_UPDATE_PROVIDER
#define DEFAULT_USE_CONTINUOUS_UPDATE_CHANNEL (true)
#define DEFAULT_CHECK_FOR_UPDATES_INTERVAL_OPTION_INDEX (0)
#endif

#define DEFAULT_FILTER_BY_SELECTED_NOTEBOOK (true)
#define DEFAULT_FILTER_BY_SELECTED_TAG (true)

#endif // QUENTIER_LIB_PREFERENCES_DEFAULT_SETTINGS_H
