cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_exception)

set(HEADERS
    LocalStorageVersionTooHighException.h)

set(SOURCES
    LocalStorageVersionTooHighException.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

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

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_exception ${PROJECT_NAME} CACHE INTERNAL "quentier_exception")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
