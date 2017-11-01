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

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/Utility.h
    src/SymbolsUnpacker.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/Utility.cpp
    src/SymbolsUnpacker.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui)

qt_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})

set(RESOURCES
    ../resource/icons/app/app_icons.qrc)

qt_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

set(${PROJECT_NAME}_TR_SOURCES
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES})

set(${PROJECT_NAME}_TR_FILES
    ../translations/quentier_crash_handler_ru.ts)

set(THIRDPARTY_LIBS ${QT_LIBRARIES})

if(APPLE)
  set(APPLICATION_ICON ${CMAKE_CURRENT_SOURCE_DIR}/../resource/icons/app/quentier_icon.icns)
  set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
  set(APPLICATION_ICON "")
endif()

# configure packaging info
if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") AND CREATE_BUNDLE AND USE_QT5)
  set(QUENTIER_PACKAGED_AS_APP_IMAGE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 1")
  set(LIBQUENTIER_BINARY_NAME "libqt5quentier.so.0")
else()
  set(QUENTIER_PACKAGED_AS_APP_IMAGE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 0")
  get_filename_component(LIBQUENTIER_BINARY_NAME "${LIBQUENTIER_LIBRARY_LOCATION}" NAME)
endif()

set(QUENTIER_LIBQUENTIER_BINARY_NAME "#define QUENTIER_LIBQUENTIER_BINARY_NAME \"${LIBQUENTIER_BINARY_NAME}\"")

find_package(Git)
if(GIT_FOUND)
  message(STATUS "Git found: ${GIT_EXECUTABLE}")

  # Get git branch
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  OUTPUT_VARIABLE QUENTIER_GIT_BRANCH
                  RESULT_VARIABLE QUENTIER_GIT_BRANCH_RETURN_CODE
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT "${QUENTIER_GIT_BRANCH_RETURN_CODE}" STREQUAL "0")
    message(AUTHOR_WARNING "Failed to determine the current git branch, return code ${QUENTIER_GIT_BRANCH_RETURN_CODE}")
    set(QUENTIER_GIT_BRANCH "unknown branch")
  else()
    message(STATUS "Git branch: ${QUENTIER_GIT_BRANCH}")
  endif()

  # Get last commit short hash
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  OUTPUT_VARIABLE QUENTIER_GIT_REVISION
                  RESULT_VARIABLE QUENTIER_GIT_REVISION_RETURN_CODE
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT "${QUENTIER_GIT_REVISION_RETURN_CODE}" STREQUAL "0")
    message(AUTHOR_WARNING "Failed to determine the current git revision")
    set(QUENTIER_GIT_REVISION "unknown revision")
  else()
    message(STATUS "Last commit short hash: ${QUENTIER_GIT_REVISION}")
  endif()

  # Check for uncommitted changes
  execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  RESULT_VARIABLE QUENTIER_GIT_DIRTY_STATE)
  if(NOT "${QUENTIER_GIT_DIRTY_STATE}" STREQUAL "0")
    string(CONCAT QUENTIER_GIT_REVISION "${QUENTIER_GIT_REVISION}" ", with uncommitted changes")
  endif()

  set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"${QUENTIER_GIT_BRANCH}, ${QUENTIER_GIT_REVISION}\"")
else()
  set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"unknown\"")
endif()

configure_file(src/PackagingInfo.h.in
               ${PROJECT_BINARY_DIR}/PackagingInfo.h @ONLY)
list(APPEND ${PROJECT_NAME}_HEADERS ${PROJECT_BINARY_DIR}/PackagingInfo.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

target_link_libraries(${PROJECT_NAME} ${THIRDPARTY_LIBS})

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
if(USE_QT5)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()
