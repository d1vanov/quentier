cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_account)

set(HEADERS
    ../../src/AccountManager.h)

set(SOURCES
    ../../src/AccountManager.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
