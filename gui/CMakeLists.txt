cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(QuteNote)

include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libqutenote/src)

set(${PROJECT_NAME}_HEADERS
    client/CredentialsModel.h
    client/EvernoteServiceManager.h
    src/MainWindow.h
    src/AskConsumerKeyAndSecret.h
    src/AskUserNameAndPassword.h
    src/EvernoteOAuthBrowser.h
    src/TableSettingsDialog.h
    src/insert-table-tool-button/InsertTableToolButton.h
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.h
    src/insert-table-tool-button/TableSizeSelectorActionWidget.h
    src/insert-table-tool-button/TableSizeSelector.h)

set(${PROJECT_NAME}_SOURCES
    client/CredentialsModel.cpp
    client/EvernoteServiceManager.cpp
    src/MainWindow.cpp
    src/AskConsumerKeyAndSecret.cpp
    src/AskUserNameAndPassword.cpp
    src/EvernoteOAuthBrowser.cpp
    src/TableSettingsDialog.cpp
    src/insert-table-tool-button/InsertTableToolButton.cpp
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelector.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui
    src/AskConsumerKeyAndSecret.ui
    src/AskUserNameAndPassword.ui
    src/TableSettingsDialog.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

set(RESOURCES
    resource/icons.qrc
    resource/enc_data.qrc)

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
  qt5_use_modules(${PROJECT_NAME} Webkit)
  qt5_use_modules(${PROJECT_NAME} WebKitWidgets)
  qt5_use_modules(${PROJECT_NAME} Xml)
  qt5_use_modules(${PROJECT_NAME} Sql)
  add_definitions(-DUSE_QT5)
else()
  target_link_libraries(${PROJECT_NAME}
                        ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} ${SIMPLECRYPT_LIB} ${LIB_QUTE_NOTE}
                      ${QEVERCLOUD_LIB} ${QTKEYCHAIN_LIB} ${STACKTRACE_LIB} ${QSLOG_LIB})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT4_MIMETYPES_LIB})
endif()

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
endif()

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
