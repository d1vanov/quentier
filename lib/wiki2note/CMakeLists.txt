cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_wiki2note)

set(HEADERS
    WikiArticleToNote.h
    WikiRandomArticleUrlFetcher.h)

set(SOURCES
    WikiArticleToNote.cpp
    WikiRandomArticleUrlFetcher.cpp)

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

set(quentier_wiki2note ${PROJECT_NAME} CACHE INTERNAL "quentier_wiki2note")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
