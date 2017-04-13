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

#define REMOVE_EMPTY_UNEDITED_NOTES_SETTINGS_KEY QStringLiteral("RemoveEmptyUneditedNotes")
#define LAST_EXPORT_NOTE_TO_PDF_PATH_SETTINGS_KEY QStringLiteral("LastExportNoteToPdfPath")
#define CONVERT_TO_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ConvertToNoteTimeout")
#define EXPUNGE_NOTE_TIMEOUT_SETTINGS_KEY QStringLiteral("ExpungeNoteTimeout")

// ENEX export/import related settings keys
#define ENEX_EXPORT_IMPORT_SETTINGS_GROUP_NAME QStringLiteral("EnexExportImport")

#define LAST_EXPORT_NOTE_TO_ENEX_PATH_SETTINGS_KEY QStringLiteral("LastExportNotesToEnexPath")
#define LAST_EXPORT_NOTE_TO_ENEX_EXPORT_TAGS_SETTINGS_KEY QStringLiteral("LastExportNotesToEnexExportTags")

#endif // QUENTIER_SETTINGS_NAMES_H
