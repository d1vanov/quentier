cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(StackTrace)

set(${PROJECT_NAME}_HEADERS
    StackTrace.h)

set(${PROJECT_NAME}_SOURCES
    StackTrace.cpp)

add_library(${PROJECT_NAME} STATIC
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
foreach(ITEM ${DEPLOY_INCLUDE_FILES})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
