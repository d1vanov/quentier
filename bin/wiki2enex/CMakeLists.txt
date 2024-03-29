cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(wiki2enex VERSION 1.0.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019-2024")
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

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    quentier_network
    quentier_wiki2note)

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt6::Core
      Qt6::Network
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt5::Core
      Qt5::Network
      Libquentier::libqt5quentier)
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
