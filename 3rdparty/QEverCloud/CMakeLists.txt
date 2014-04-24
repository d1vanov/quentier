cmake_minimum_required(VERSION 2.8)
project(QEverCloud)

set(CMAKE_AUTOMOC ON)

set(${PROJECT_NAME}_HEADERS
    include/QEverCloud.h
    include/QEverCloudOAuth.h
    oauth.h
    thrift.h
    http.h
    public.h
    exceptions.h
    globals.h
    impl.h
    thumbnail.h
    AsyncResult.h
    Optional.h
    EverCloudException.h
    generated/constants.h
    generated/services.h
    generated/types.h
    generated/types_impl.h
    generated/EDAMErrorCode.h)

set(${PROJECT_NAME}_SOURCES
    exceptions.cpp
    oauth.cpp
    http.cpp
    services_nongenerated.cpp
    AsyncResult.cpp
    generated/constants.cpp 
    generated/services.cpp
    generated/types.cpp)

add_library(${PROJECT_NAME} STATIC
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/generated)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/generated/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
