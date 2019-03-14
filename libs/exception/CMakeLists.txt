cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_exception)

set(HEADERS
    src/exception/LocalStorageVersionTooHighException.h)

set(SOURCES
    src/exception/LocalStorageVersionTooHighException.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
