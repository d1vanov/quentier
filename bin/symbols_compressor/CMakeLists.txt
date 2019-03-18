cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(quentier_symbols_compressor VERSION 1.0.0)
else()
  project(quentier_symbols_compressor)
  set(PROJECT_VERSION_MAJOR "1")
  set(PROJECT_VERSION_MINOR "0")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2017")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(${PROJECT_NAME}_SOURCES
    src/main.cpp)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

if(USE_QT5)
  target_link_libraries(${PROJECT_NAME} Qt5::Core)
else()
  target_link_libraries(${PROJECT_NAME} ${QT_QTCORE_LIBRARY})
endif()

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
if(USE_QT5)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()

set(symbols_compressor ${PROJECT_NAME} PARENT_SCOPE)
