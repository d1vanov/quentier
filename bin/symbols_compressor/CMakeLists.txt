cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_symbols_compressor VERSION 1.0.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019-2024")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(SOURCES
    src/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt6::Core)
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt5::Core)
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

set(symbols_compressor ${PROJECT_NAME} CACHE INTERNAL "symbols_compressor")
