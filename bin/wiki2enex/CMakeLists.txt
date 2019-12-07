cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(wiki2enex VERSION 1.0.0)

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

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network)

target_link_libraries(${PROJECT_NAME} ${quentier_network})
target_link_libraries(${PROJECT_NAME} ${quentier_wiki2note})
target_link_libraries(${PROJECT_NAME} ${LIBQUENTIER_LIBRARIES})

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
