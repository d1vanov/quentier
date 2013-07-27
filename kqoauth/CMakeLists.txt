cmake_minimum_required(VERSION 2.8)
project(kqoauth)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-uninitialized")
endif()

find_package(Qt4 COMPONENTS QtCore QtNetwork QtWebkit REQUIRED)
include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
add_definitions("-DKQOAUTH")

set(CMAKE_AUTOMOC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_HEADERS
    src/kqoauthauthreplyserver.h
    src/kqoauthauthreplyserver_p.h
    src/kqoauthmanager.h
    src/kqoauthrequest.h
    src/kqoauthrequest_xauth.h
    src/kqoauthglobals.h
    src/kqoauthrequest_1.h
    src/kqoauthutils.h
    src/kqoauthrequest_xauth_p.h
    src/kqoauthrequest_p.h
    src/kqoauthmanager_p.h)

set(${PROJECT_NAME}_SOURCES
    src/kqoauthauthreplyserver.cpp
    src/kqoauthmanager.cpp
    src/kqoauthrequest.cpp
    src/kqoauthrequest_1.cpp
    src/kqoauthrequest_xauth.cpp
    src/kqoauthutils.cpp
    )

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
