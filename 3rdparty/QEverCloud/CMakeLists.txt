cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(QEverCloud)

set(CMAKE_AUTOMOC ON)

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")

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
    EventLoopFinisher.h
    EverCloudException.h
    qt4helper.h
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
    EventLoopFinisher.cpp
    generated/constants.cpp 
    generated/services.cpp
    generated/types.cpp)

add_library(${PROJECT_NAME} STATIC
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core Widgets Webkit WebKitWidgets Network)
endif()

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
