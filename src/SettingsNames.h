#ifndef QUENTIER_SETTINGS_NAMES_H
#define QUENTIER_SETTINGS_NAMES_H

// The names of different .ini files containing the settings
#define QUENTIER_UI_SETTINGS QStringLiteral("UserInterface")
#define QUENTIER_AUXILIARY_SETTINGS QStringLiteral("Auxiliary")

// System tray related settings keys
#define SYSTEM_TRAY_SETTINGS_GROUP_NAME QStringLiteral("SystemTray")

#define SHOW_SYSTEM_TRAY_ICON_SETTINGS_KEY QStringLiteral("ShowSystemTrayIcon")
#define CLOSE_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("CloseToSystemTray")
#define MINIMIZE_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("MinimizeToSystemTray")
#define START_MINIMIZED_TO_SYSTEM_TRAY_SETTINGS_KEY QStringLiteral("StartMinimizedToSystemTray")

// The name of the environment variable allowing to override the system tray availability
#define OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR "QUENTIER_OVERRIDE_SYSTEM_TRAY_AVAILABILITY"

#define SYSTEM_TRAY_ICON_KIND_KEY QStringLiteral("TrayIconKind")

#define SINGLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("SingleClickTrayAction")
#define MIDDLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("MiddleClickTrayAction")
#define DOUBLE_CLICK_TRAY_ACTION_SETTINGS_KEY QStringLiteral("DoubleClickTrayAction")

// Note editor related settings keys
#define NOTE_EDITOR_SETTINGS_GROUP_NAME QStringLiteral("NoteEditor")

#define USE_LIMITED_SET_OF_FONTS QStringLiteral("UseLimitedSetOfFonts")
#define REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY QStringLiteral("RemoveEmptyUneditedNotes")
#define LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY QStringLiteral("LastExportNoteToPdfPath")
#define CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ConvertToNoteTimeout")
#define EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ExpungeNoteTimeout")

// Other UI related settings keys
#define LOOK_AND_FEEL_SETTINGS_GROUP_NAME QStringLiteral("LookAndFeel")
#define ICON_THEME_SETTINGS_KEY QStringLiteral("iconTheme")
#define PANELS_STYLE_SETTINGS_KEY QStringLiteral("panelStyle")

// ENEX export/import related settings keys
#define ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME QStringLiteral("EnexExportImport")

#define LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastExportNotesToEnexPath")
#define LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY QStringLiteral("LastExportNotesToEnexExportTags")
#define LAST_IMPORT_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastImportEnexPath")
#define LAST_IMPORT_ENEX_NOTEBOOK_NAME_SETTINGS_KEY QStringLiteral("LastImportEnexNotebookName")

// Account-related settings keys
#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")

#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID QStringLiteral("LastUsedAccountId")
#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE QStringLiteral("LastUsedAccountEvernoteAccountType")
#define LAST_USED_ACCOUNT_EVERNOTE_HOST QStringLiteral("LastUsedAccountEvernoteHost")

// Environment variables that can be used to specify the account to use on startup
#define ACCOUNT_NAME_ENV_VAR "QUENTIER_ACCOUNT_NAME"
#define ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_TYPE"
#define ACCOUNT_ID_ENV_VAR "QUENTIER_ACCOUNT_ID"
#define ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE"
#define ACCOUNT_EVERNOTE_HOST_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_HOST"

#endif // QUENTIER_SETTINGS_NAMES_H
