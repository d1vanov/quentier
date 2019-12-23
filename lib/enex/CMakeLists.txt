cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_enex)

set(HEADERS
    EnexExporter.h
    EnexExportDialog.h
    EnexImporter.h
    EnexImportDialog.h)

set(SOURCES
    EnexExporter.cpp
    EnexExportDialog.cpp
    EnexImporter.cpp
    EnexImportDialog.cpp)

set(FORMS
    EnexExportDialog.ui
    EnexImportDialog.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_enex ${PROJECT_NAME} CACHE INTERNAL "quentier_enex")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
