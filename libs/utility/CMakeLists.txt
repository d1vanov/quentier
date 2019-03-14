cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_utility)

set(HEADERS
    ../../src/utility/ColorCodeValidator.h
    ../../src/utility/HumanReadableVersionInfo.h
    ../../src/utility/QObjectThreadMover.h
    ../../src/utility/QObjectThreadMover_p.h
    ../../src/utility/StartAtLogin.h)

set(SOURCES
    ../../src/utility/ColorCodeValidator.cpp
    ../../src/utility/HumanReadableVersionInfo.cpp
    ../../src/utility/QObjectThreadMover.cpp
    ../../src/utility/QObjectThreadMover_p.cpp
    ../../src/utility/StartAtLogin.cpp)

if(WIN32)
  list(APPEND SOURCES ../../src/utility/windows/StartAtLogin.cpp)
elseif(APPLE)
  list(APPEND SOURCES ../../src/utility/darwin/StartAtLogin.cpp)
else()
  list(APPEND SOURCES ../../src/utility/linux/StartAtLogin.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
