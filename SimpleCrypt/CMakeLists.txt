cmake_minimum_required(VERSION 2.8)
project(simplecrypt)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-uninitialized")
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    find_library(LIBCPP NAMES libc++.so libc++.so.1.0 libc++.dylib OPTIONAL)
    if(LIBCPP)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
  endif()
endif()

find_package(Qt4 COMPONENTS QtCore REQUIRED)
include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
add_definitions("-DKQOAUTH")

set(CMAKE_AUTOMOC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_HEADERS
    src/simplecrypt.h)

set(${PROJECT_NAME}_SOURCES
    src/simplecrypt.cpp)

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
