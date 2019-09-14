cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(wiki2enex VERSION 1.0.0)
else()
  project(wiki2enex)
  set(PROJECT_VERSION_MAJOR "1")
  set(PROJECT_VERSION_MINOR "0")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(HEADERS
    src/WikiArticleFetcher.h)

set(SOURCES
    src/main.cpp
    src/WikiArticleFetcher.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

if(BUILD_WITH_QT4)
  target_link_libraries(${PROJECT_NAME} ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY})
else()
  target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network)
endif()

target_link_libraries(${PROJECT_NAME} ${quentier_network})
target_link_libraries(${PROJECT_NAME} ${quentier_wiki2note})
target_link_libraries(${PROJECT_NAME} ${LIBQUENTIER_LIBRARIES})

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
if(NOT BUILD_WITH_QT4)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()
