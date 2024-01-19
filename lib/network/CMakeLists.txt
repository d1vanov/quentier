cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_network)

set(HEADERS
    NetworkReplyFetcher.h
    NetworkProxySettingsHelpers.h)

set(SOURCES
    NetworkReplyFetcher.cpp
    NetworkProxySettingsHelpers.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Qt6::Network)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Qt5::Network)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_network ${PROJECT_NAME} CACHE INTERNAL "quentier_network")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
