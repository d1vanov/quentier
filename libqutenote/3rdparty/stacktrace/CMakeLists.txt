cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

project(StackTrace)

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS StackTrace.h)

set(SOURCES StackTrace.cpp)
prepend_path(SOURCES "${SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
foreach(ITEM ${DEPLOY_INCLUDE_FILES})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
