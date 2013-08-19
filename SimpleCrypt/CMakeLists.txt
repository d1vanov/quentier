cmake_minimum_required(VERSION 2.8)
project(simplecrypt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_HEADERS
    src/simplecrypt.h)

set(${PROJECT_NAME}_SOURCES
    src/simplecrypt.cpp)

add_library(${PROJECT_NAME} STATIC
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
