cmake_minimum_required(VERSION 2.8)
project(QuteNote)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-uninitialized")
endif()

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork QtWebkit REQUIRED)
include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)

include_directories(${QuteNoteProject_SOURCE_DIR}/thrift/src
                    ${QuteNoteProject_SOURCE_DIR}/thrift/src/thrift
                    ${QuteNoteProject_SOURCE_DIR}/kqoauth/src
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
                    ${CMAKE_CURRENT_BINARY_DIR})

set(${PROJECT_NAME}_QT_HEADERS
    src/gui/MainWindow.h
    src/gui/NoteEditorWidget.h
    src/gui/EvernoteOAuthBrowser.h
    src/note_editor/ToDoCheckboxTextObject.h
    src/note_editor/HorizontalLineExtraData.h
    src/note_editor/QuteNoteTextEdit.h)

set(${PROJECT_NAME}_HEADERS
    src/evernote_sdk/src/UserStore_types.h
    src/evernote_sdk/src/UserStore_constants.h
    src/evernote_sdk/src/UserStore.h
    src/evernote_sdk/src/Types_types.h
    src/evernote_sdk/src/Types_constants.h
    src/evernote_sdk/src/NoteStore_types.h
    src/evernote_sdk/src/NoteStore_constants.h
    src/evernote_sdk/src/NoteStore.h
    src/evernote_sdk/src/Limits_types.h
    src/evernote_sdk/src/Limits_constants.h
    src/evernote_sdk/src/Errors_types.h
    src/evernote_sdk/src/Errors_constants.h
    src/tools/Singleton.h
    src/client/EvernoteServiceManager.h
    src/client/EvernoteServiceOAuthHandler.h
    src/client/CredentialsModel.h)

qt4_wrap_cpp(${PROJECT_NAME}_HEADERS_MOC ${${PROJECT_NAME}_QT_HEADERS})

set(${PROJECT_NAME}_SOURCES
    src/gui/MainWindow.cpp
    src/gui/NoteEditorWidget.cpp
    src/gui/EvernoteOAuthBrowser.cpp
    src/note_editor/ToDoCheckboxTextObject.cpp
    src/note_editor/QuteNoteTextEdit.cpp
    src/evernote_sdk/src/UserStore_types.cpp
    src/evernote_sdk/src/UserStore_constants.cpp
    src/evernote_sdk/src/UserStore.cpp
    src/evernote_sdk/src/Types_types.cpp
    src/evernote_sdk/src/Types_constants.cpp
    src/evernote_sdk/src/NoteStore_types.cpp
    src/evernote_sdk/src/NoteStore_constants.cpp
    src/evernote_sdk/src/NoteStore.cpp
    src/evernote_sdk/src/Limits_types.cpp
    src/evernote_sdk/src/Limits_constants.cpp
    src/evernote_sdk/src/Errors_types.cpp
    src/evernote_sdk/src/Errors_constants.cpp
    src/client/EvernoteServiceManager.cpp
    src/client/EvernoteServiceOAuthHandler.cpp
    src/client/CredentialsModel.cpp)

set(FORMS
    src/gui/MainWindow.ui
    src/gui/NoteEditorWidget.ui)

qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})

set(RESOURCES
    resource/icons.qrc)

qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

add_executable(${PROJECT_NAME}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_QT_HEADERS_MOC}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC}
               main.cpp)

add_executable("${PROJECT_NAME}_Test_EvernoteOAuthBrowser"
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_QT_HEADERS_MOC}
               tests/Test_EvernoteOAuthBrowser.cpp)

find_package(OpenSSL REQUIRED)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(THRIFT_LIB "${QuteNoteProject_BINARY_DIR}/thrift/libthrift.so")
  set(KQOAUTH_LIB "${QuteNoteProject_BINARY_DIR}/kqoauth/libkqoauth.so")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(THRIFT_LIB "${QuteNoteProject_BINARY_DIR}/thrift/libthrift.dylib")
  set(KQOAUTH_LIB "${QuteNoteProject_BINARY_DIR}/kqoauth/libkqoauth.dylib")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(THRIFT_LIB "${QuteNoteProject_BINARY_DIR}/thrift/thrift.dll")
  set(KQOAUTH_LIB "${QuteNoteProject_BINARY_DIR}/kqoauth/kqoauth.dll")
else()
  message(FATAL_ERROR "Unsupported platform. Only Windows, Mac OS X and Linux are supported at the moment")
endif()

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${QT_LIBRARIES}
                      ${OPENSSL_LIBRARIES})

target_link_libraries("${PROJECT_NAME}_Test_EvernoteOAuthBrowser"
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${QT_LIBRARIES}
                      ${OPENSSL_LIBRARIES})
