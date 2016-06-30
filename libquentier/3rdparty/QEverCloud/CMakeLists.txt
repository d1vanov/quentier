cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

project(QEverCloud)

set(CMAKE_AUTOMOC ON)

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")

set(HEADERS
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
    generated/constants.h
    generated/services.h
    generated/types.h
    generated/types_impl.h
    generated/EDAMErrorCode.h)
prepend_path(HEADERS "${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
    exceptions.cpp
    oauth.cpp
    http.cpp
    services_nongenerated.cpp
    AsyncResult.cpp
    EventLoopFinisher.cpp
    generated/constants.cpp 
    generated/services.cpp
    generated/types.cpp)
prepend_path(SOURCES "${SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
list(APPEND INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/generated")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

if(USE_QT5)
  if(USE_QT_WEB_ENGINE)
    qt5_use_modules(${PROJECT_NAME} Core Widgets WebEngine WebEngineWidgets Network)
    if(NOT Qt5WebEngine_VERSION VERSION_LESS "5.6.0")
      qt5_use_modules(${PROJECT_NAME} WebEngineCore)
    endif()
  else()
    qt5_use_modules(${PROJECT_NAME} Core Widgets Webkit WebKitWidgets Network)
  endif()
endif()

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h ${CMAKE_CURRENT_SOURCE_DIR}/generated/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
