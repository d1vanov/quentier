cmake_minimum_required(VERSION 2.8)
project(QuteNote)

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

include_directories(${QuteNoteProject_SOURCE_DIR}/thrift/src
                    ${QuteNoteProject_SOURCE_DIR}/thrift/src/thrift
                    ${QuteNoteProject_SOURCE_DIR}/kqoauth/src
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
                    ${CMAKE_CURRENT_BINARY_DIR})

set(${PROJECT_NAME}_QT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/MainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskConsumerKeyAndSecret.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskUserNameAndPassword.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EvernoteOAuthBrowser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/HorizontalLineExtraData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/ToDoCheckboxTextObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/ResourceTextObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/MediaResourceTextObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/QuteNoteTextEdit.h)

set(${PROJECT_NAME}_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/EvernoteServiceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/EvernoteServiceOAuthHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/CredentialsModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/EvernoteServiceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/UserStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/User.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/NoteStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Note.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Guid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Notebook.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Tag.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/SynchronizedDataElement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/enml/ENMLConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/EvernoteServiceManagerImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/UserStoreImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/UserImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/NoteStoreImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/NoteImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/ResourceImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/TagImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/Printable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/TypeWithError.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/IQuteNoteException.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/QuteNoteNullPtrException.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/QuteNoteCheckPtr.h)

set(${PROJECT_NAME}_EVERNOTE_SDK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore_constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Types_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Types_constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore_constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Limits_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Limits_constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Errors_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Errors_constants.h)

set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskConsumerKeyAndSecret.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskUserNameAndPassword.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/EvernoteOAuthBrowser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/ResourceTextObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/ToDoCheckboxTextObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/note_editor/QuteNoteTextEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/EvernoteServiceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/EvernoteServiceOAuthHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/CredentialsModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/EvernoteServiceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/UserStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/NoteStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/User.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Note.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Guid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Notebook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Resource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/Tag.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/SynchronizedDataElement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/enml/ENMLConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/EvernoteServiceManagerImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/ResourceImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/TagImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client_private/NoteImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/Printable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/TypeWithError.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/IQuteNoteException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/QuteNoteNullPtrException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

set(${PROJECT_NAME}_EVERNOTE_SDK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore_constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/UserStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Types_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Types_constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore_constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/NoteStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Limits_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Limits_constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Errors_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_sdk/src/Errors_constants.cpp)

set(FORMS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/MainWindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskConsumerKeyAndSecret.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/AskUserNameAndPassword.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

set(RESOURCES
    resource/icons.qrc
    resource/enc_data.qrc
    resource/enml.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

find_package(OpenSSL REQUIRED)

find_package(LibXml2 REQUIRED)
if(LIBXML2_FOUND)
  include_directories(${LIBXML2_INCLUDE_DIR})
endif()

add_executable(${PROJECT_NAME}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_EVERNOTE_SDK_HEADERS}
               ${${PROJECT_NAME}_EVERNOTE_SDK_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core)
  qt5_use_modules(${PROJECT_NAME} Gui)
  qt5_use_modules(${PROJECT_NAME} Widgets)
  qt5_use_modules(${PROJECT_NAME} Network)
  qt5_use_modules(${PROJECT_NAME} Webkit)
  qt5_use_modules(${PROJECT_NAME} WebKitWidgets)
else()
  target_link_libraries(${PROJECT_NAME}
                        ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${SIMPLECRYPT_LIB}
                      ${OPENSSL_LIBRARIES}
                      ${LIBXML2_LIBRARIES})

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
