cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_initialization)

set(HEADERS
    CommandLineParser.h
    DefaultAccountFirstNotebookAndNoteCreator.h
    Initialize.h
    LoadDependencies.h
    ParseStartupAccount.h
    SetupApplicationIcon.h
    SetupTranslations.h
    SetupStartAtLogin.h)

set(SOURCES
    CommandLineParser.cpp
    DefaultAccountFirstNotebookAndNoteCreator.cpp
    Initialize.cpp
    LoadDependencies.cpp
    ParseStartupAccount.cpp
    SetupApplicationIcon.cpp
    SetupTranslations.cpp
    SetupStartAtLogin.cpp)

if(BREAKPAD_FOUND)
  list(APPEND HEADERS breakpad/BreakpadIntegration.h)
  list(APPEND SOURCES breakpad/BreakpadIntegration.cpp)
  if(WIN32)
    list(APPEND SOURCES breakpad/BreakpadIntegrationWindows.cpp)
  else()
    list(APPEND SOURCES breakpad/BreakpadIntegrationLinux.cpp)
  endif()

endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_initialization ${PROJECT_NAME} CACHE INTERNAL "quentier_initialization")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

if(BREAKPAD_FOUND)
  # workaround: cppcheck fails to parse some macros used within Breakpad integration sources so need to exclude them
  list(REMOVE_ITEM SOURCES breakpad/BreakpadIntegration.cpp)
  if(WIN32)
    list(REMOVE_ITEM SOURCES breakpad/BreakpadIntegrationWindows.cpp)
  else()
    list(REMOVE_ITEM SOURCES /breakpad/BreakpadIntegrationLinux.cpp)
  endif()
endif()

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)

set(DIRS
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/breakpad)
COLLECT_INCLUDE_DIRS(DIRS)

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(DIRS)
