cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(quentier_crash_handler VERSION 1.0.0)
else()
  project(quentier_crash_handler)
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

qt_wrap_ui(FORMS_HEADERS ${FORMS})

qt_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})

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

include_directories(${QUENTIER_VERSION_INFO_HEADER_DIR})

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${HEADERS}
               ${FORMS_HEADERS}
               ${SOURCES}
               ${RESOURCES_RCC})

target_link_libraries(${PROJECT_NAME} quentier_utility ${LIBQUENTIER_LIBRARIES} ${QT_LIBRARIES})

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
if(USE_QT5)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()
