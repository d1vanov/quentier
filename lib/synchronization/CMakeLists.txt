cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_synchronization)

set(HEADERS
    Fwd.h
    SyncEventsTracker.h
    SyncResultsStorage.h)

set(SOURCES
    SyncEventsTracker.cpp
    SyncResultsStorage.cpp)

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

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_synchronization ${PROJECT_NAME} CACHE INTERNAL "quentier_synchronization")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
