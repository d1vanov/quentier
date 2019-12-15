cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_exception)

set(HEADERS
    LocalStorageVersionTooHighException.h)

set(SOURCES
    LocalStorageVersionTooHighException.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_exception ${PROJECT_NAME} CACHE INTERNAL "quentier_exception")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
