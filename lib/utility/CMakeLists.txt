cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_utility)

set(HEADERS
    ActionsInfo.h
    AsyncFileWriter.h
    BasicXMLSyntaxHighlighter.h
    ColorCodeValidator.h
    HumanReadableVersionInfo.h
    MainWindowSideBorderOption.h
    NetworkProxySettingsHelpers.h
    QObjectThreadMover.h
    QObjectThreadMover_p.h
    StartAtLogin.h
    SuppressWarningsMacro.h)

set(SOURCES
    ActionsInfo.cpp
    AsyncFileWriter.cpp
    BasicXMLSyntaxHighlighter.cpp
    ColorCodeValidator.cpp
    HumanReadableVersionInfo.cpp
    NetworkProxySettingsHelpers.cpp
    QObjectThreadMover.cpp
    QObjectThreadMover_p.cpp
    StartAtLogin.cpp)

if(WIN32)
  list(APPEND SOURCES windows/StartAtLogin.cpp)
elseif(APPLE)
  list(APPEND SOURCES darwin/StartAtLogin.cpp)
else()
  list(APPEND SOURCES linux/StartAtLogin.cpp)
endif()

set_property(GLOBAL PROPERTY QUENTIER_VERSION_INFO_HEADER
             "${PROJECT_BINARY_DIR}/VersionInfo.h")
configure_file(VersionInfo.h.in
               ${QUENTIER_VERSION_INFO_HEADER} @ONLY)
list(APPEND HEADERS ${QUENTIER_VERSION_INFO_HEADER})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
