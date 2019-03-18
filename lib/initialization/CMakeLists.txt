cmake_minimum_required(VERSION 2.8)

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

  # workaround: cppcheck fails to parse some macros used within Breakpad integration sources so need to exclude them
  list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/breakpad/BreakpadIntegration.cpp)
  if(WIN32)
    list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/breakpad/BreakpadIntegrationWindows.cpp)
  else()
    list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/breakpad/BreakpadIntegrationLinux.cpp)
  endif()
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

add_dependencies(${PROJECT_NAME} quentier_utility quentier_account)

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(${SOURCES})

set(DIRS . breakpad)
COLLECT_INCLUDE_DIRS(DIRS)
