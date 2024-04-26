cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_update)

set(HEADERS
    Fwd.h
    GitHubUpdateChecker.h
    IUpdateChecker.h
    IUpdateProvider.h
    UpdateManager.h)

set(SOURCES
    GitHubUpdateChecker.cpp
    IUpdateChecker.cpp
    IUpdateProvider.cpp
    UpdateManager.cpp)

if(USE_APP_IMAGE_UPDATER_BRIDGE)
  list(APPEND HEADERS AppImageUpdateChecker.h)
  list(APPEND HEADERS AppImageUpdateProvider.h)
  list(APPEND SOURCES AppImageUpdateChecker.cpp)
  list(APPEND SOURCES AppImageUpdateProvider.cpp)
endif()

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ")

if(NOT GIT_FOUND)
  set(QUENTIER_GIT_BRANCH "")
  set(QUENTIER_GIT_REVISION "")
endif()

set(QUENTIER_DEFAULT_UPDATE_CHANNEL  "#define QUENTIER_DEFAULT_UPDATE_CHANNEL \"${DEFAULT_UPDATE_CHANNEL}\"")
set(QUENTIER_DEFAULT_UPDATE_PROVIDER "#define QUENTIER_DEFAULT_UPDATE_PROVIDER UpdateProvider::${DEFAULT_UPDATE_PROVIDER}")
set(QUENTIER_BUILD_TIMESTAMP         "#define QUENTIER_BUILD_TIMESTAMP QStringLiteral(\"${BUILD_TIMESTAMP}\")")
set(QUENTIER_BUILD_GIT_BRANCH        "#define QUENTIER_BUILD_GIT_BRANCH QStringLiteral(\"${QUENTIER_GIT_BRANCH}\")")
set(QUENTIER_BUILD_GIT_COMMIT        "#define QUENTIER_BUILD_GIT_COMMIT QStringLiteral(\"${QUENTIER_GIT_REVISION}\")")

set(QUENTIER_UPDATE_INFO_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/UpdateInfo.h"
    CACHE INTERNAL "QUENTIER_UPDATE_INFO_HEADER")

configure_file(UpdateInfo.h.in
               ${QUENTIER_UPDATE_INFO_HEADER} @ONLY)

list(APPEND HEADERS ${QUENTIER_UPDATE_INFO_HEADER})
set(QUENTIER_UPDATE_INFO_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "UpdateInfo.h dir")

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${QUENTIER_ROOT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    ${quentier_network})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Libquentier::libqt5quentier)
endif()

if(USE_APP_IMAGE_UPDATER_BRIDGE)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      AppImageUpdaterBridge)

  target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${APP_IMAGE_UPDATER_BRIDGE_INCLUDE_DIR})
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_update ${PROJECT_NAME} CACHE INTERNAL "quentier_update")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
