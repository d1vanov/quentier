cmake_minimum_required(VERSION 3.16.3)

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

if(BUILD_WITH_QT6)
  qt6_wrap_ui(FORMS_HEADERS ${FORMS})
else()
  qt5_wrap_ui(FORMS_HEADERS ${FORMS})
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
	  Qt6::Core
	  Qt6::Widgets)
else()
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
	  Qt5::Core
	  Qt5::Widgets)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_enex ${PROJECT_NAME} CACHE INTERNAL "quentier_enex")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)
