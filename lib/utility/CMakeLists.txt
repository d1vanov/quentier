cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_utility)

set(HEADERS
    ActionsInfo.h
    AsyncFileWriter.h
    BasicXMLSyntaxHighlighter.h
    ColorCodeValidator.h
    ExitCodes.h
    HumanReadableVersionInfo.h
    IStartable.h
    Keychain.h
    Log.h
    QObjectThreadMover.h
    QObjectThreadMover_p.h
    RestartApp.h
    StartAtLogin.h)

set(SOURCES
    ActionsInfo.cpp
    AsyncFileWriter.cpp
    BasicXMLSyntaxHighlighter.cpp
    ColorCodeValidator.cpp
    HumanReadableVersionInfo.cpp
    Keychain.cpp
    Log.cpp
    QObjectThreadMover.cpp
    QObjectThreadMover_p.cpp
    RestartApp.cpp
    StartAtLogin.cpp)

if(WIN32)
  list(APPEND SOURCES windows/StartAtLogin.cpp)
elseif(APPLE)
  list(APPEND SOURCES darwin/StartAtLogin.cpp)
else()
  list(APPEND SOURCES linux/StartAtLogin.cpp)
endif()

set(QUENTIER_MAJOR_VERSION "#define QUENTIER_MAJOR_VERSION \"${QUENTIER_VERSION_MAJOR}\"")
set(QUENTIER_MINOR_VERSION "#define QUENTIER_MINOR_VERSION \"${QUENTIER_VERSION_MINOR}\"")
set(QUENTIER_PATCH_VERSION "#define QUENTIER_PATCH_VERSION \"${QUENTIER_VERSION_PATCH}\"")

set(QUENTIER_VERSION_INFO_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/VersionInfo.h"
    CACHE INTERNAL "QUENTIER_VERSION_INFO_HEADER")
configure_file(VersionInfo.h.in
               ${QUENTIER_VERSION_INFO_HEADER} @ONLY)
list(APPEND HEADERS ${QUENTIER_VERSION_INFO_HEADER})
set(QUENTIER_VERSION_INFO_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "VersionInfo.h dir")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_utility ${PROJECT_NAME} CACHE INTERNAL "quentier_utility")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)

set(DIRS ${PROJECT_SOURCE_DIR})

if(WIN32)
  list(APPEND DIRS ${PROJECT_SOURCE_DIR}/windows)
elseif(APPLE)
  list(APPEND DIRS ${PROJECT_SOURCE_DIR}/darwin)
else()
  list(APPEND DIRS ${PROJECT_SOURCE_DIR}/linux)
endif()

COLLECT_INCLUDE_DIRS(DIRS)

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(DIRS)
