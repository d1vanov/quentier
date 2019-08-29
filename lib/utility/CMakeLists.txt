cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_utility)

set(HEADERS
    ActionsInfo.h
    AsyncFileWriter.h
    BasicXMLSyntaxHighlighter.h
    ColorCodeValidator.h
    HumanReadableVersionInfo.h
    IStartable.h
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
