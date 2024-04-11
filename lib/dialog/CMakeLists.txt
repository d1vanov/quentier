cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_dialog)

set(HEADERS
    AddOrEditNotebookDialog.h
    AddOrEditTagDialog.h
    AddOrEditSavedSearchDialog.h
    EditNoteDialog.h
    EditNoteDialogsManager.h
    FirstShutdownDialog.h
    LocalStorageVersionTooHighDialog.h
    LocalStorageUpgradeDialog.h
    WelcomeToQuentierDialog.h)

set(SOURCES
    AddOrEditNotebookDialog.cpp
    AddOrEditTagDialog.cpp
    AddOrEditSavedSearchDialog.cpp
    EditNoteDialog.cpp
    EditNoteDialogsManager.cpp
    FirstShutdownDialog.cpp
    LocalStorageVersionTooHighDialog.cpp
    LocalStorageUpgradeDialog.cpp
    WelcomeToQuentierDialog.cpp)

set(FORMS
    AddOrEditNotebookDialog.ui
    AddOrEditSavedSearchDialog.ui
    AddOrEditTagDialog.ui
    EditNoteDialog.ui
    FirstShutdownDialog.ui
    LocalStorageUpgradeDialog.ui
    LocalStorageVersionTooHighDialog.ui
    WelcomeToQuentierDialog.ui)

if(BUILD_WITH_QT6)
  qt_wrap_ui(FORMS_HEADERS ${FORMS})
else()
  qt5_wrap_ui(FORMS_HEADERS ${FORMS})
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Qt6::Widgets
      Libquentier::libqt6quentier
    PRIVATE
      quentier_exception)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Qt5::Widgets
      Libquentier::libqt5quentier
    PRIVATE
      quentier_exception)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_dialog ${PROJECT_NAME} CACHE INTERNAL "quentier_dialog")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)
