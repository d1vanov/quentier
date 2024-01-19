cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_crash_handler VERSION 1.0.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(HEADERS
    src/MainWindow.h
    src/Utility.h
    src/SymbolsUnpacker.h)

set(SOURCES
    src/MainWindow.cpp
    src/Utility.cpp
    src/SymbolsUnpacker.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui)

if(BUILD_WITH_QT6)
  qt6_wrap_ui(FORMS_HEADERS ${FORMS})
  qt6_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})
else()
  qt5_wrap_ui(FORMS_HEADERS ${FORMS})
  qt5_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})
endif()

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

if(APPLE)
  set(APPLICATION_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../../resource/icons/app/quentier_icon.icns)
  set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
  set(APPLICATION_ICON "")
endif()

list(APPEND ${PROJECT_NAME}_HEADERS ${QUENTIER_VERSION_INFO_HEADER})

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${HEADERS}
               ${FORMS_HEADERS}
               ${SOURCES}
               ${RESOURCES_RCC})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_VERSION_INFO_HEADER_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    quentier_utility)

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt6::Core
      Qt6::Widgets
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt5::Core
      Qt5::Widgets
      Libquentier::libqt5quentier)
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
