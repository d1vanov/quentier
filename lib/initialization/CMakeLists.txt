cmake_minimum_required(VERSION 3.16.3)

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

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Libquentier::libqt5quentier)
endif()

if(BREAKPAD_FOUND)
  target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${BREAKPAD_INCLUDE_DIRS})

  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      ${BREAKPAD_LIBRARIES})
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_initialization ${PROJECT_NAME} CACHE INTERNAL "quentier_initialization")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
