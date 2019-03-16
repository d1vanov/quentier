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

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

add_dependencies(${PROJECT_NAME} quentier_utility quentier_account)

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
