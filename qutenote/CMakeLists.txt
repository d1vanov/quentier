cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(qutenote)

include(QuteNoteAdditionalCompilerWarnings)

include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libqutenote/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libqutenote/headers)

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/TableSettingsDialog.h
    src/BasicXMLSyntaxHighlighter.h
    src/insert-table-tool-button/InsertTableToolButton.h
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.h
    src/insert-table-tool-button/TableSizeSelectorActionWidget.h
    src/insert-table-tool-button/TableSizeSelector.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/TableSettingsDialog.cpp
    src/BasicXMLSyntaxHighlighter.cpp
    src/insert-table-tool-button/InsertTableToolButton.cpp
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelector.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui
    src/TableSettingsDialog.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

set(RESOURCES
    resource/icons.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

add_executable(${PROJECT_NAME}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core)
  qt5_use_modules(${PROJECT_NAME} Gui)
  qt5_use_modules(${PROJECT_NAME} Widgets)
  qt5_use_modules(${PROJECT_NAME} Network)
  qt5_use_modules(${PROJECT_NAME} Xml)
  qt5_use_modules(${PROJECT_NAME} Sql)
  if(USE_QT_WEB_ENGINE)
    qt5_use_modules(${PROJECT_NAME} WebEngine)
    qt5_use_modules(${PROJECT_NAME} WebEngineWidgets)
  else()
    qt5_use_modules(${PROJECT_NAME} Webkit)
    qt5_use_modules(${PROJECT_NAME} WebKitWidgets)
  endif()
else()
  target_link_libraries(${PROJECT_NAME}
                        ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} ${LIB_QUTE_NOTE} ${THIRDPARTY_LIBS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()
            
# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/client")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/enml")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/types")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/evernote_sdk/src")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/gui")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/note_editor")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/tools")

# modify the sources list for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# install executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
