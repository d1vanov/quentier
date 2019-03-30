cmake_minimum_required(VERSION 2.8)

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

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set(quentier_dialog ${PROJECT_NAME} CACHE INTERNAL "quentier_dialog")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
