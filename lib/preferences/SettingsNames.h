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

#ifndef QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
#define QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H

////////////////////////////////////////////////////////////////////////////////

// Panel colors settings keys
#define PANEL_COLORS_SETTINGS_GROUP_NAME QStringLiteral("PanelColors")

#define PANEL_COLORS_FONT_COLOR_SETTINGS_KEY QStringLiteral("FontColor")

#define PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY                             \
    QStringLiteral("BackgroundColor")

#define PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY                      \
    QStringLiteral("UseBackgroundGradient")

#define PANEL_COLORS_BACKGROUND_GRADIENT_BASE_COLOR_SETTINGS_KEY               \
    QStringLiteral("BackgroundGradientBaseColor")

#define PANEL_COLORS_BACKGROUND_GRADIENT_LINES_SETTINGS_KEY                    \
    QStringLiteral("BackgroundGradientLines")

#define PANEL_COLORS_BACKGROUND_GRADIENT_LINE_VALUE_SETTINGS_KEY               \
    QStringLiteral("BackgroundGradientLineValue")

#define PANEL_COLORS_BACKGROUND_GRADIENT_LINE_COLOR_SETTTINGS_KEY              \
    QStringLiteral("BackgroundGradientLineColor")

////////////////////////////////////////////////////////////////////////////////

// Side panels filtering via selection settings keys
#define SIDE_PANELS_FILTER_BY_SELECTION_SETTINGS_GROUP_NAME                    \
    QStringLiteral("SidePanelsFilterBySelection")

#define FILTER_BY_SELECTED_NOTEBOOK_SETTINGS_KEY                               \
    QStringLiteral("FilterBySelectedNotebook")

#define FILTER_BY_SELECTED_TAG_SETTINGS_KEY                                    \
    QStringLiteral("FilterBySelectedTag")

#define FILTER_BY_SELECTED_SAVED_SEARCH_SETTINGS_KEY                           \
    QStringLiteral("FilterBySelectedSavedSearch")

#define FILTER_BY_SELECTED_FAVORITED_ITEM_SETTINGS_KEY                         \
    QStringLiteral("FilterBySelectedFavoritesItem")

////////////////////////////////////////////////////////////////////////////////

// Other UI related settings keys
#define LOOK_AND_FEEL_SETTINGS_GROUP_NAME QStringLiteral("LookAndFeel")
#define ICON_THEME_SETTINGS_KEY           QStringLiteral("IconTheme")
#define PANELS_STYLE_SETTINGS_KEY         QStringLiteral("PanelStyle")
#define SHOW_NOTE_THUMBNAILS_SETTINGS_KEY QStringLiteral("ShowNoteThumbnails")

#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY                                  \
    QStringLiteral("HideNoteThumbnailsFor")

// Max allowed count of keys in HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY
#define HIDE_NOTE_THUMBNAILS_FOR_SETTINGS_KEY_MAX_COUNT 100

#define DARKER_PANEL_STYLE_NAME  QStringLiteral("Darker")
#define LIGHTER_PANEL_STYLE_NAME QStringLiteral("Lighter")

#define DISABLE_NATIVE_MENU_BAR_SETTINGS_KEY                                   \
    QStringLiteral("DisableNativeMenuBar")

////////////////////////////////////////////////////////////////////////////////

// ENEX export/import related settings keys
#define ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME                                 \
    QStringLiteral("EnexExportImport")

#define LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY                             \
    QStringLiteral("LastExportNotesToEnexPath")

#define LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY                      \
    QStringLiteral("LastExportNotesToEnexExportTags")

#define LAST_IMPORT_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastImportEnexPath")

#define LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY                            \
    QStringLiteral("LastImportEnexNotebookName")

////////////////////////////////////////////////////////////////////////////////

// Account-related settings keys
#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")

#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID   QStringLiteral("LastUsedAccountId")

#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE                                \
    QStringLiteral("LastUsedAccountEvernoteAccountType")

#define LAST_USED_ACCOUNT_EVERNOTE_HOST                                        \
    QStringLiteral("LastUsedAccountEvernoteHost")

#define ONCE_DISPLAYED_GREETER_SCREEN                                          \
    QStringLiteral("OnceDisplayedGreeterScreen")

// Environment variables that can be used to specify the account to use on
// startup
#define ACCOUNT_NAME_ENV_VAR "QUENTIER_ACCOUNT_NAME"
#define ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_TYPE"
#define ACCOUNT_ID_ENV_VAR   "QUENTIER_ACCOUNT_ID"

#define ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR                                  \
    "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE"

#define ACCOUNT_EVERNOTE_HOST_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_HOST"

////////////////////////////////////////////////////////////////////////////////

// Log level settings
#define LOGGING_SETTINGS_GROUP QStringLiteral("LoggingSettings")
#define CURRENT_MIN_LOG_LEVEL  QStringLiteral("MinLogLevel")

#define CURRENT_FILTER_BY_COMPONENT_PRESET                                     \
    QStringLiteral("FilterByComponentPreset")

#define CURRENT_FILTER_BY_COMPONENT QStringLiteral("FilterByComponentRegex")

#define ENABLE_LOG_VIEWER_INTERNAL_LOGS                                        \
    QStringLiteral("EnableLogViewerInternalLogs")

////////////////////////////////////////////////////////////////////////////////

// Translations setting
#define TRANSLATION_SETTINGS_GROUP_NAME QStringLiteral("TranslationSettings")

#define LIBQUENTIER_TRANSLATIONS_SEARCH_PATH                                   \
    QStringLiteral("LibquentierTranslationsSearchPath")

#define QUENTIER_TRANSLATIONS_SEARCH_PATH                                      \
    QStringLiteral("QuentierTranslationsSearchPath")

////////////////////////////////////////////////////////////////////////////////

// Synchronization settings
#define SYNCHRONIZATION_SETTINGS_GROUP_NAME                                    \
    QStringLiteral("SynchronizationSettings")

#define SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS                               \
    QStringLiteral("DownloadNoteThumbnails")

#define SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES                               \
    QStringLiteral("DownloadInkNoteImages")

#define SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES                              \
    QStringLiteral("RunSyncEachNumMinutes")

#define SYNCHRONIZATION_NETWORK_PROXY_SETTINGS                                 \
    QStringLiteral("SynchronizationNetworkProxySettings")

#define SYNCHRONIZATION_NETWORK_PROXY_TYPE                                     \
    QStringLiteral("SynchronizationNetworkProxyType")

#define SYNCHRONIZATION_NETWORK_PROXY_HOST                                     \
    QStringLiteral("SynchronizationNetworkProxyHost")

#define SYNCHRONIZATION_NETWORK_PROXY_PORT                                     \
    QStringLiteral("SynchronizationNetworkProxyPort")

#define SYNCHRONIZATION_NETWORK_PROXY_USER                                     \
    QStringLiteral("SynchronizationNetworkProxyUser")

#define SYNCHRONIZATION_NETWORK_PROXY_PASSWORD                                 \
    QStringLiteral("SynchronizationNetworkProxyPassword")

#endif // QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
