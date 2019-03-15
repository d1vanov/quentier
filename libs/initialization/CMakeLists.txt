cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_initialization)

set(HEADERS
    ../../src/initialization/CommandLineParser.h
    ../../src/initialization/DefaultAccountFirstNotebookAndNoteCreator.h
    ../../src/initialization/Initialize.h
    ../../src/initialization/LoadDependencies.h
    ../../src/initialization/ParseStartupAccount.h
    ../../src/initialization/SetupApplicationIcon.h
    ../../src/initialization/SetupTranslations.h
    ../../src/initialization/SetupStartAtLogin.h)

set(SOURCES
    ../../src/initialization/CommandLineParser.cpp
    ../../src/initialization/DefaultAccountFirstNotebookAndNoteCreator.cpp
    ../../src/initialization/Initialize.cpp
    ../../src/initialization/LoadDependencies.cpp
    ../../src/initialization/ParseStartupAccount.cpp
    ../../src/initialization/SetupApplicationIcon.cpp
    ../../src/initialization/SetupTranslations.cpp
    ../../src/initialization/SetupStartAtLogin.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
