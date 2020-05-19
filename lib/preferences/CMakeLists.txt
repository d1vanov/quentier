cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_preferences)

set(HEADERS
    DefaultDisableNativeMenuBar.h
    DefaultSettings.h
    SettingsNames.h
    PreferencesDialog.h
    panel_colors/PanelColorsHandlerWidget.h
    shortcut_settings/ShortcutButton.h
    shortcut_settings/ShortcutSettingsWidget.h)

set(SOURCES
    DefaultDisableNativeMenuBar.cpp
    PreferencesDialog.cpp
    panel_colors/PanelColorsHandlerWidget.cpp
    shortcut_settings/ShortcutButton.cpp
    shortcut_settings/ShortcutSettingsWidget.cpp)

if(INCLUDE_UPDATE_INFO)
  list(APPEND HEADERS UpdateSettings.h)
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
    ${PROJECT_SOURCE_DIR}/shortcut_settings)
COLLECT_INCLUDE_DIRS(DIRS)
