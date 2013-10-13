cmake_minimum_required(VERSION 2.8)
project(simplecrypt)

add_definitions("-DBUILD_LIBSIMPLECRYPT")

set(${PROJECT_NAME}_HEADERS
    src/Simplecrypt.h
    src/Linkage.h)

set(${PROJECT_NAME}_SOURCES
    src/Simplecrypt.cpp)

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
else()
  qt5_use_modules(${PROJECT_NAME} Core)
endif()

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
