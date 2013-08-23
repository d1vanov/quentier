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
    src/client/EvernoteServiceManager.h
    src/client/EvernoteServiceOAuthHandler.h
    src/client/CredentialsModel.h
    src/evernote_client/EvernoteServiceManager.h
    src/evernote_client/EvernoteServiceManagerImpl.h
    src/evernote_client/UserStore.h
    src/evernote_client/UserStoreImpl.h
    src/evernote_client/User.h
    src/evernote_client/UserImpl.h
    src/evernote_client/NoteStore.h
    src/evernote_client/NoteStoreImpl.h
    src/evernote_client/Note.h
    src/evernote_client/NoteImpl.h
    src/evernote_client/Guid.h
    src/evernote_client/Notebook.h)

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
    src/client/CredentialsModel.cpp
    src/evernote_client/EvernoteServiceManager.cpp
    src/evernote_client/EvernoteServiceManagerImpl.cpp
    src/evernote_client/UserStore.cpp
    src/evernote_client/NoteStore.cpp
    src/evernote_client/User.cpp
    src/evernote_client/Note.cpp
    src/evernote_client/Guid.cpp
    src/evernote_client/Notebook.cpp
    main.cpp)

set(FORMS
    src/gui/MainWindow.ui
    src/gui/NoteEditorWidget.ui
    src/gui/AskConsumerKeyAndSecret.ui
    src/gui/AskUserNameAndPassword.ui)

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

add_executable(${PROJECT_NAME}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

find_package(OpenSSL REQUIRED)

target_link_libraries(${PROJECT_NAME}
                      ${THRIFT_LIB}
                      ${KQOAUTH_LIB}
                      ${SIMPLECRYPT_LIB}
                      ${QT_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
                      ${XERCESC_LIBRARY})

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
endif()
