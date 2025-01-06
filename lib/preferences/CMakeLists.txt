cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_preferences)

set(HEADERS
    PreferencesDialog.h
    defaults/Appearance.h
    defaults/NoteEditor.h
    defaults/SidePanelsFiltering.h
    defaults/StartAtLogin.h
    defaults/Synchronization.h
    defaults/SystemTray.h
    keys/Account.h
    keys/Appearance.h
    keys/Enex.h
    keys/Files.h
    keys/Logging.h
    keys/NoteEditor.h
    keys/PanelColors.h
    keys/SidePanelsFiltering.h
    keys/StartAtLogin.h
    keys/Synchronization.h
    keys/SystemTray.h
    keys/Translations.h
    panel_colors/PanelColorsHandlerWidget.h
    shortcut_settings/ShortcutButton.h
    shortcut_settings/ShortcutSettingsWidget.h)

set(SOURCES
    PreferencesDialog.cpp
    defaults/Appearance.cpp
    panel_colors/PanelColorsHandlerWidget.cpp
    shortcut_settings/ShortcutButton.cpp
    shortcut_settings/ShortcutSettingsWidget.cpp)

if(INCLUDE_UPDATE_INFO)
  list(APPEND HEADERS UpdateSettings.h)
  list(APPEND HEADERS defaults/Updates.h)
  list(APPEND HEADERS keys/Updates.h)
  list(APPEND SOURCES UpdateSettings.cpp)
endif()

set(FORMS
    PreferencesDialog.ui
    panel_colors/PanelColorsHandlerWidget.ui
    shortcut_settings/ShortcutSettingsWidget.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_preferences ${PROJECT_NAME} CACHE INTERNAL "quentier_preferences")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)

set(DIRS
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/panel_colors
    ${PROJECT_SOURCE_DIR}/shortcut_settings)
COLLECT_INCLUDE_DIRS(DIRS)

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(DIRS)
