cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_wiki2note)

set(HEADERS
    WikiArticleToNote.h)

set(SOURCES
    WikiArticleToNote.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(quentier_wiki2note ${PROJECT_NAME} CACHE INTERNAL "quentier_wiki2note")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
