cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_tray)

set(HEADERS
    SystemTrayIconManager.h)

set(SOURCES
    SystemTrayIconManager.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

add_dependencies(${PROJECT_NAME} quentier_account)

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
