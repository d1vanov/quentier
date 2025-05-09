cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(wiki2account VERSION 1.0.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019")
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
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network)

target_link_libraries(${PROJECT_NAME} ${quentier_account})
target_link_libraries(${PROJECT_NAME} ${quentier_initialization})
target_link_libraries(${PROJECT_NAME} ${quentier_network})
target_link_libraries(${PROJECT_NAME} ${quentier_utility})
target_link_libraries(${PROJECT_NAME} ${quentier_wiki2note})
target_link_libraries(${PROJECT_NAME} ${LIBQUENTIER_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if(BREAKPAD_FOUND)
  target_link_libraries(${PROJECT_NAME} ${BREAKPAD_LIBRARIES})
endif()

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR}/src)
