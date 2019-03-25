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

#ifndef QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
#define QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H

// The names of different .ini files containing the settings
#define QUENTIER_UI_SETTINGS QStringLiteral("UserInterface")
#define QUENTIER_AUXILIARY_SETTINGS QStringLiteral("Auxiliary")
#define QUENTIER_SYNC_SETTINGS QStringLiteral("Synchronization")

// Start automatically at login settings keys
#define START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME \
    QStringLiteral("StartAutomaticallyAtLogin")

#define SHOULD_START_AUTOMATICALLY_AT_LOGIN \
    QStringLiteral("ShouldStartAutomaticallyAtLogin")
#define START_AUTOMATICALLY_AT_LOGIN_OPTION \
    QStringLiteral("StartAutomaticallyAtLoginOption")

// System tray related settings keys
#define SYSTEM_TRAY_SETTINGS_GROUP_NAME QStringLiteral("SystemTray")

#define SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY QStringLiteral("ShowSystemTrayIcon")
#define CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("CloseToSystemTray")
#define MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("MinimizeToSystemTray")
#define START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY \
    QStringLiteral("StartMinimizedToSystemTray")

// The name of the environment variable allowing to override the system tray availability
#define OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR \
    "QUENTIER_OVERRIDE_SYSTEM_TRAY_AVAILABILITY"

#define SYSTEM_TRAY_ICON_KIND_KEY QStringLiteral("TrayIconKind")

#define SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("SingleClickTrayAction")
#define MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("MiddleClickTrayAction")
#define DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("DoubleClickTrayAction")

// Note editor related settings keys
#define NOTE_EDITOR_SETTINGS_GROUP_NAME QStringLiteral("NoteEditor")

#define USE_LIMITED_SET_OF_FONTS QStringLiteral("UseLimitedSetOfFonts")
#define REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY \
    QStringLiteral("RemoveEmptyUneditedNotes")
#define LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY \
    QStringLiteral("LastExportNoteToPdfPath")
#define CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ConvertToNoteTimeout")
#define EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ExpungeNoteTimeout")

// Other UI related settings keys
#define LOOK_AND_FEEL_SETTINGS_GROUP_NAME QStringLiteral("LookAndFeel")
#define ICON_THEME_SETTINGS_KEY QStringLiteral("IconTheme")
#define PANELS_STYLE_SETTINGS_KEY QStringLiteral("PanelStyle")
#define SHOW_NOTE_THUMBNAILS_SETTINGS_KEY QStringLiteral("ShowNoteThumbnails")
#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY QStringLiteral("HideNoteThumbnailsFor")
// max.allowed count of keys in HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY
#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY_MAX_COUNT 100

#define DARKER_PANEL_STYLE_NAME QStringLiteral("Darker")
#define LIGHTER_PANEL_STYLE_NAME QStringLiteral("Lighter")

// Left/right main window borders settings

#define SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY \
    QStringLiteral("ShowLeftMainWindowBorderOption")
#define LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY \
    QStringLiteral("LeftMainWindowBorderWidth")
#define LEFT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR \
    QStringLiteral("LeftMainWindowBorderOverrideColor")

#define SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY \
    QStringLiteral("ShowRightMainWindowBorderOption")
#define RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY \
    QStringLiteral("RightMainWindowBorderWidth")
#define RIGHT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR \
    QStringLiteral("RightMainWindowBorderOverrideColor")

// ENEX export/import related settings keys
#define ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME QStringLiteral("EnexExportImport")

#define LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY \
    QStringLiteral("LastExportNotesToEnexPath")
#define LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY \
    QStringLiteral("LastExportNotesToEnexExportTags")
#define LAST_IMPORT_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastImportEnexPath")
#define LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY \
    QStringLiteral("LastImportEnexNotebookName")

// Account-related settings keys
#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")

#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID QStringLiteral("LastUsedAccountId")
#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE \
    QStringLiteral("LastUsedAccountEvernoteAccountType")
#define LAST_USED_ACCOUNT_EVERNOTE_HOST QStringLiteral("LastUsedAccountEvernoteHost")

#define ONCE_DISPLAYED_GREETER_SCREEN QStringLiteral("OnceDisplayedGreeterScreen")

// Environment variables that can be used to specify the account to use on startup
#define ACCOUNT_NAME_ENV_VAR "QUENTIER_ACCOUNT_NAME"
#define ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_TYPE"
#define ACCOUNT_ID_ENV_VAR "QUENTIER_ACCOUNT_ID"
#define ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE"
#define ACCOUNT_EVERNOTE_HOST_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_HOST"

// Log level settings
#define LOGGING_SETTINGS_GROUP QStringLiteral("LoggingSettings")
#define CURRENT_MIN_LOG_LEVEL QStringLiteral("MinLogLevel")
#define ENABLE_LOG_VIEWER_INTERNAL_LOGS QStringLiteral("EnableLogViewerInternalLogs")

// Translations setting
#define TRANSLATION_SETTINGS_GROUP_NAME QStringLiteral("TranslationSettings")

#define LIBQUENTIER_TRANSLATIONS_SEARCH_PATH \
    QStringLiteral("LibquentierTranslationsSearchPath")
#define QUENTIER_TRANSLATIONS_SEARCH_PATH \
    QStringLiteral("QuentierTranslationsSearchPath")

// Synchronization settings
#define SYNCHRONIZATION_SETTINGS_GROUP_NAME QStringLiteral("SynchronizationSettings")
#define SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS QStringLiteral("DownloadNoteThumbnails")
#define SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES QStringLiteral("DownloadInkNoteImages")
#define SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES QStringLiteral("RunSyncEachNumMinutes")

#define SYNCHRONIZATION_NETWORK_PROXY_SETTINGS \
    QStringLiteral("SynchronizationNetworkProxySettings")
#define SYNCHRONIZATION_NETWORK_PROXY_TYPE \
    QStringLiteral("SynchronizationNetworkProxyType")
#define SYNCHRONIZATION_NETWORK_PROXY_HOST \
    QStringLiteral("SynchronizationNetworkProxyHost")
#define SYNCHRONIZATION_NETWORK_PROXY_PORT \
    QStringLiteral("SynchronizationNetworkProxyPort")
#define SYNCHRONIZATION_NETWORK_PROXY_USER \
    QStringLiteral("SynchronizationNetworkProxyUser")
#define SYNCHRONIZATION_NETWORK_PROXY_PASSWORD \
    QStringLiteral("SynchronizationNetworkProxyPassword")

#endif // QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
