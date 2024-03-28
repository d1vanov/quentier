cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(wiki2account VERSION 1.0.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019-2024")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(HEADERS
    src/FetchNotes.h
    src/NotebookController.h
    src/PrepareAvailableCommandLineOptions.h
    src/PrepareLocalStorageManager.h
    src/PrepareNotebooks.h
    src/PrepareTags.h
    src/ProcessStartupAccount.h
    src/ProcessNoteOptions.h
    src/ProcessNotebookOptions.h
    src/ProcessTagOptions.h
    src/TagController.h
    src/WikiArticlesFetcher.h
    src/WikiArticlesFetchingTracker.h
    src/WikiRandomArticleFetcher.h)

set(SOURCES
    src/FetchNotes.cpp
    src/NotebookController.cpp
    src/PrepareAvailableCommandLineOptions.cpp
    src/PrepareLocalStorageManager.cpp
    src/PrepareNotebooks.cpp
    src/PrepareTags.cpp
    src/ProcessStartupAccount.cpp
    src/ProcessNoteOptions.cpp
    src/ProcessNotebookOptions.cpp
    src/ProcessTagOptions.cpp
    src/TagController.cpp
    src/WikiArticlesFetcher.cpp
    src/WikiArticlesFetchingTracker.cpp
    src/WikiRandomArticleFetcher.cpp
    src/main.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    quentier_account
    quentier_initialization
    quentier_network
    quentier_utility
    quentier_wiki2note
    Boost::boost)

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Libquentier::libqt6quentier
      Qt6::Core
      Qt6::Network)
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Libquentier::libqt5quentier
      Qt5::Core
      Qt5::Network)
endif()

if(BREAKPAD_FOUND)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      ${BREAKPAD_LIBRARIES})
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
