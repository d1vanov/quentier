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
#define START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME                       \
    QStringLiteral("StartAutomaticallyAtLogin")                                \
// START_AUTOMATICALLY_AT_LOGIN_SETTINGS_GROUP_NAME

#define SHOULD_START_AUTOMATICALLY_AT_LOGIN                                    \
    QStringLiteral("ShouldStartAutomaticallyAtLogin")                          \
// SHOULD_START_AUTOMATICALLY_AT_LOGIN

#define START_AUTOMATICALLY_AT_LOGIN_OPTION                                    \
    QStringLiteral("StartAutomaticallyAtLoginOption")                          \
// START_AUTOMATICALLY_AT_LOGIN_OPTION

// System tray related settings keys
#define SYSTEM_TRAY_SETTINGS_GROUP_NAME QStringLiteral("SystemTray")

#define SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY QStringLiteral("ShowSystemTrayIcon")
#define CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("CloseToSystemTray")

#define MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY                                   \
    QStringLiteral("MinimizeToSystemTray")                                     \
// MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY

#define START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY                            \
    QStringLiteral("StartMinimizedToSystemTray")                               \
// START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY

// The name of the environment variable allowing to override the system tray
// availability
#define OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR                              \
    "QUENTIER_OVERRIDE_SYSTEM_TRAY_AVAILABILITY"                               \
// OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR

#define SYSTEM_TRAY_ICON_KIND_KEY QStringLiteral("TrayIconKind")

#define SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY                                  \
    QStringLiteral("SingleClickTrayAction")                                    \
// SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY

#define MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY                                  \
    QStringLiteral("MiddleClickTrayAction")                                    \
// MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY

#define DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY                                  \
    QStringLiteral("DoubleClickTrayAction")                                    \
// DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY

// Note editor related settings keys
#define NOTE_EDITOR_SETTINGS_GROUP_NAME QStringLiteral("NoteEditor")

#define USE_LIMITED_SET_OF_FONTS QStringLiteral("UseLimitedSetOfFonts")

#define REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY                               \
    QStringLiteral("RemoveEmptyUneditedNotes")                                 \
// REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY

#define LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY                              \
    QStringLiteral("LastExportNoteToPdfPath")                                  \
// LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY

#define CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY                                   \
    QStringLiteral("ConvertToNoteTimeout")                                     \
// CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY

#define EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY                                      \
    QStringLiteral("ExpungeNoteTimeout")                                       \
// EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY

#define NOTE_EDITOR_FONT_COLOR_SETTINGS_KEY QStringLiteral("FontColor")

#define NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY                              \
    QStringLiteral("BackgroundColor")                                          \
// NOTE_EDITOR_BACKGROUND_COLOR_SETTINGS_KEY

#define NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY                               \
    QStringLiteral("HighlightColor")                                           \
// NOTE_EDITOR_HIGHLIGHT_COLOR_SETTINGS_KEY

#define NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY                              \
    QStringLiteral("HighlightedTextColor")                                     \
// NOTE_EDITOR_HIGHLIGHTED_TEXT_SETTINGS_KEY

// Other UI related settings keys
#define LOOK_AND_FEEL_SETTINGS_GROUP_NAME QStringLiteral("LookAndFeel")
#define ICON_THEME_SETTINGS_KEY QStringLiteral("IconTheme")
#define PANELS_STYLE_SETTINGS_KEY QStringLiteral("PanelStyle")
#define SHOW_NOTE_THUMBNAILS_SETTINGS_KEY QStringLiteral("ShowNoteThumbnails")

#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY                                  \
    QStringLiteral("HideNoteThumbnailsFor")                                    \
// HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY

// Max allowed count of keys in HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY
#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY_MAX_COUNT 100

#define DARKER_PANEL_STYLE_NAME QStringLiteral("Darker")
#define LIGHTER_PANEL_STYLE_NAME QStringLiteral("Lighter")

#define DISABLE_NATIVE_MENU_BAR_SETTINGS_KEY                                   \
    QStringLiteral("DisableNativeMenuBar")                                     \
// DISABLE_NATIVE_MENU_BAR_SETTINGS_KEY

// ENEX export/import related settings keys
#define ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME                                 \
    QStringLiteral("EnexExportImport")                                         \
// ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME

#define LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY                             \
    QStringLiteral("LastExportNotesToEnexPath")                                \
// LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY

#define LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY                      \
    QStringLiteral("LastExportNotesToEnexExportTags")                          \
// LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY

#define LAST_IMPORT_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastImportEnexPath")

#define LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY                            \
    QStringLiteral("LastImportEnexNotebookName")                               \
// LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY

// Account-related settings keys
#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")

#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID QStringLiteral("LastUsedAccountId")

#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE                                \
    QStringLiteral("LastUsedAccountEvernoteAccountType")                       \
// LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE

#define LAST_USED_ACCOUNT_EVERNOTE_HOST                                        \
    QStringLiteral("LastUsedAccountEvernoteHost")                              \
// LAST_USED_ACCOUNT_EVERNOTE_HOST

#define ONCE_DISPLAYED_GREETER_SCREEN                                          \
    QStringLiteral("OnceDisplayedGreeterScreen")                               \
// ONCE_DISPLAYED_GREETER_SCREEN

// Environment variables that can be used to specify the account to use on
// startup
#define ACCOUNT_NAME_ENV_VAR "QUENTIER_ACCOUNT_NAME"
#define ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_TYPE"
#define ACCOUNT_ID_ENV_VAR "QUENTIER_ACCOUNT_ID"

#define ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR                                  \
    "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE"                                   \
// ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR

#define ACCOUNT_EVERNOTE_HOST_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_HOST"

// Log level settings
#define LOGGING_SETTINGS_GROUP QStringLiteral("LoggingSettings")
#define CURRENT_MIN_LOG_LEVEL QStringLiteral("MinLogLevel")

#define ENABLE_LOG_VIEWER_INTERNAL_LOGS                                        \
    QStringLiteral("EnableLogViewerInternalLogs")                              \
// ENABLE_LOG_VIEWER_INTERNAL_LOGS

// Translations setting
#define TRANSLATION_SETTINGS_GROUP_NAME QStringLiteral("TranslationSettings")

#define LIBQUENTIER_TRANSLATIONS_SEARCH_PATH                                   \
    QStringLiteral("LibquentierTranslationsSearchPath")                        \
// LIBQUENTIER_TRANSLATIONS_SEARCH_PATH

#define QUENTIER_TRANSLATIONS_SEARCH_PATH                                      \
    QStringLiteral("QuentierTranslationsSearchPath")                           \
// QUENTIER_TRANSLATIONS_SEARCH_PATH

// Synchronization settings
#define SYNCHRONIZATION_SETTINGS_GROUP_NAME                                    \
    QStringLiteral("SynchronizationSettings")                                  \
// SYNCHRONIZATION_SETTINGS_GROUP_NAME

#define SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS                               \
    QStringLiteral("DownloadNoteThumbnails")                                   \
// SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS

#define SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES                               \
    QStringLiteral("DownloadInkNoteImages")                                    \
// SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES

#define SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES                              \
    QStringLiteral("RunSyncEachNumMinutes")                                    \
// SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES

#define SYNCHRONIZATION_NETWORK_PROXY_SETTINGS                                 \
    QStringLiteral("SynchronizationNetworkProxySettings")                      \
// SYNCHRONIZATION_NETWORK_PROXY_SETTINGS

#define SYNCHRONIZATION_NETWORK_PROXY_TYPE                                     \
    QStringLiteral("SynchronizationNetworkProxyType")                          \
// SYNCHRONIZATION_NETWORK_PROXY_TYPE

#define SYNCHRONIZATION_NETWORK_PROXY_HOST                                     \
    QStringLiteral("SynchronizationNetworkProxyHost")                          \
// SYNCHRONIZATION_NETWORK_PROXY_HOST

#define SYNCHRONIZATION_NETWORK_PROXY_PORT                                     \
    QStringLiteral("SynchronizationNetworkProxyPort")                          \
// SYNCHRONIZATION_NETWORK_PROXY_PORT

#define SYNCHRONIZATION_NETWORK_PROXY_USER                                     \
    QStringLiteral("SynchronizationNetworkProxyUser")                          \
// SYNCHRONIZATION_NETWORK_PROXY_USER

#define SYNCHRONIZATION_NETWORK_PROXY_PASSWORD                                 \
    QStringLiteral("SynchronizationNetworkProxyPassword")                      \
// SYNCHRONIZATION_NETWORK_PROXY_PASSWORD

#endif // QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
