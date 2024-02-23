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

if(BUILD_WITH_QT6)
  qt6_wrap_ui(FORMS_HEADERS ${FORMS})
else()
  qt5_wrap_ui(FORMS_HEADERS ${FORMS})
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ../..)

if(INCLUDE_UPDATE_INFO)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      -DWITH_UPDATE_MANAGER=1)
endif()

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Qt6::Widgets
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Qt5::Widgets
      Libquentier::libqt5quentier)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_preferences ${PROJECT_NAME} CACHE INTERNAL "quentier_preferences")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)
