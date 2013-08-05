cmake_minimum_required(VERSION 2.8)
project(QuteNote)

if(CMAKE_COMPILER_IS_GNUCXX)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  message(STATUS "Using GNU C++ compiler, version ${GCC_VERSION}.")
  if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    message(STATUS "Your compiler supports C++11 standard.")
    add_definitions("-std=gnu++11")
  elseif(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
    message(WARNING "Your compiler is known to support C++11 standard only partially. If you get any errors update to a compiler which fully supports C++11.")
    add_definitions("-std=gnu++0x")
  else()
    message(FATAL_ERROR "Your compiler does not support C++11 standard. You need a GNU C++ compiler with version higher than 4.3.")
  endif()
  add_definitions("-Wno-uninitialized")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Using LLVM/Clang C++ compiler, version info: ${CLANG_VERSION}")
  message(STATUS "You may have issues with compilation if your compiler is too old to support the C++11 features used in the project. In that case update your compiler. ")
  add_definitions("-std=c++11 -Wno-uninitialized")
  find_library(LIBCPP NAMES libc++.so libc++.so.1.0 libc++.dylib OPTIONAL)
  if(LIBCPP)
    message(STATUS "Using native clang C++ standard library: ${LIBCPP}")
    add_definitions("-stdlib=libc++ -DHAVELIBCPP")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC11")
  message(STATUS "Using MSVC C++ compiler, version 17")
  if(${CMAKE_MINOR_VERSION} EQUAL 8)
     if(${CMAKE_PATCH_VERSION} EQUAL 11 OR
        ${CMAKE_PATCH_VERSION} GREATER 11)
       cmake_policy(SET CMP0020 NEW)
     endif()
  endif()
  add_definitions("-DNOMINMAX")
  set(USE_MSVC17 "Yes")
else()
  message(FATAL_ERROR "Unsupported C++ compiler")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  # Include files required by thrift
  find_path(NETINET_IN_INCLUDE_DIR netinet/in.h REQUIRED)
  message(STATUS "Include file netinet/in.h found in ${NETINET_IN_INCLUDE_DIR}")
  include_directories(${NETINET_IN_INCLUDE_DIR})
  add_definitions(-DHAVE_NETINET_IN_H)
else()
  find_path(WINSOCK2_INCLUDE_DIR Winsock2.h REQUIRED)
  message(STATUS "Include file Winsock2.h found in ${WINSOCK2_INCLUDE_DIR}")
  include_directories(${WINSOCK2_INCLUDE_DIR})
endif()

# TODO: add an option to use Qt5

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
    src/gui/AskConsumerKeyAndSecret.h
    src/gui/AskUserNameAndPassword.h
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
    src/gui/AskConsumerKeyAndSecret.cpp
    src/gui/AskUserNameAndPassword.cpp
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
    src/gui/NoteEditorWidget.ui
    src/gui/AskConsumerKeyAndSecret.ui
    src/gui/AskUserNameAndPassword.ui)

qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})

set(RESOURCES
    resource/icons.qrc
    resource/enc_data.qrc)

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
  set(SIMPLECRYPT_LIB "${QuteNoteProject_BINARY_DIR}/SimpleCrypt/libsimplecrypt.so")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(THRIFT_LIB "${QuteNoteProject_BINARY_DIR}/thrift/libthrift.dylib")
  set(KQOAUTH_LIB "${QuteNoteProject_BINARY_DIR}/kqoauth/libkqoauth.dylib")
  set(SIMPLECRYPT_LIB "${QuteNoteProject_BINARY_DIR}/SimpleCrypt/libsimplecrypt.dylib")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(THRIFT_LIB "${QuteNoteProject_BINARY_DIR}/thrift/thrift.dll")
  set(KQOAUTH_LIB "${QuteNoteProject_BINARY_DIR}/kqoauth/kqoauth.dll")
  set(SIMPLECRYPT_LIB "${QuteNoteProject_BINARY_DIR}/SimpleCrypt/simplecrypt.dll")
else()
  message(FATAL_ERROR "Unsupported platform. Only Windows, Mac OS X and Linux are supported at the moment")
endif()

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${SIMPLECRYPT_LIB}
                      ${QT_LIBRARIES}
                      ${OPENSSL_LIBRARIES})

target_link_libraries("${PROJECT_NAME}_Test_EvernoteOAuthBrowser"
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${SIMPLECRYPT_LIB}
                      ${QT_LIBRARIES}
                      ${OPENSSL_LIBRARIES})

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
  target_link_libraries("${PROJECT_NAME}_Test_EvernoteOAuthBrowser" ${LIBCPP})
endif()
