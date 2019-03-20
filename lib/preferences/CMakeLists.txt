cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_preferences)

set(HEADERS
    DefaultSettings.h
    SettingsNames.h
    PreferencesDialog.h
    shortcut_settings/ShortcutButton.h
    shortcut_settings/ShortcutSettingsWidget.h)

set(SOURCES
    PreferencesDialog.cpp
    shortcut_settings/ShortcutButton.cpp
    shortcut_settings/ShortcutSettingsWidget.cpp)

set(FORMS
    PreferencesDialog.ui
    shortcut_settings/ShortcutSettingsWidget.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set(quentier_preferences ${PROJECT_NAME} CACHE INTERNAL "quentier_preferences")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, ${FORMS})

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)

set(DIRS
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/shortcut_settings)
COLLECT_INCLUDE_DIRS(DIRS)
