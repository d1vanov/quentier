cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_exception)

set(HEADERS
    LocalStorageVersionTooHighException.h)

set(SOURCES
    LocalStorageVersionTooHighException.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(quentier_exception ${PROJECT_NAME} CACHE INTERNAL "quentier_exception")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
