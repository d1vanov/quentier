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

#ifndef QUENTIER_DEFAULT_SETTINGS_H
#define QUENTIER_DEFAULT_SETTINGS_H

#include "SystemTrayIconManager.h"
#include "utility/StartAtLogin.h"
#include <QtGlobal>

#define DEFAULT_SHOW_SYSTEM_TRAY_ICON (true)
#define DEFAULT_CLOSE_TO_SYSTEM_TRAY (true)
#define DEFAULT_MINIMIZE_TO_SYSTEM_TRAY (false)

#define DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY (false)

#define DEFAULT_SHOW_NOTE_THUMBNAILS (true)

#define DEFAULT_SHOULD_START_AUTOMATICALLY_AT_LOGIN_OPTION (true)
#define DEFAULT_START_AUTOMATICALLY_AT_LOGIN_OPTION \
    (StartQuentierAtLoginOption::MinimizedToTray)

#ifdef Q_WS_MAC
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("dark")
#else
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("colored")
#endif

#ifdef Q_WS_MAC
#define DEFAULT_SINGLE_CLICK_TRAY_ACTION \
    (SystemTrayIconManager::TrayActionDoNothing)
#else
#define DEFAULT_SINGLE_CLICK_TRAY_ACTION \
    (SystemTrayIconManager::TrayActionShowContextMenu)
#endif

#define DEFAULT_MIDDLE_CLICK_TRAY_ACTION \
    (SystemTrayIconManager::TrayActionShowHide)
#define DEFAULT_DOUBLE_CLICK_TRAY_ACTION \
    (SystemTrayIconManager::TrayActionDoNothing)

#define DEFAULT_REMOVE_EMPTY_UNEDITED_NOTES (true)
#define DEFAULT_EDITOR_CONVERT_TO_NOTE_TIMEOUT (500)
#define DEFAULT_EXPUNGE_NOTE_TIMEOUT (500)

#define DEFAULT_DOWNLOAD_NOTE_THUMBNAILS (true)
#define DEFAULT_DOWNLOAD_INK_NOTE_IMAGES (true)

#define DEFAULT_RUN_SYNC_EACH_NUM_MINUTES (15)

#define DEFAULT_MAIN_WINDOW_BORDER_COLOR QStringLiteral("#626262")
#define DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION \
    (MainWindowSideBorderOption::ShowOnlyWhenMaximized)
#define DEFAULT_MAIN_WINDOW_BORDER_SIZE (4)
#define MAX_MAIN_WINDOW_BORDER_SIZE (20)

#endif // QUENTIER_DEFAULT_SETTINGS_H
