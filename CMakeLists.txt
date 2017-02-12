cmake_minimum_required(VERSION 2.8)

project(quentier)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libquentier/cmake/modules ${CMAKE_MODULE_PATH})

if((WIN32 OR APPLE) AND (${CMAKE_VERSION} VERSION_LESS "3.8.0"))
  # The deployment-related module GetPrerequisites.cmake has a bug fixed in CMake only in 3.8.0
  set(QUENTIER_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/quentier/cmake/modules)
  message(STATUS "Using backported fix for GetPrerequisites.cmake: module path = ${QUENTIER_CMAKE_MODULE_PATH}")
endif()

include(QuentierCMakePolicies)
SET_POLICIES()

include(QuentierCompilerSettings)
include(QuentierLinkerSettings)
include(QuentierSetupQt)
include(QuentierFindLibXml2)
include(QuentierFindOpenSSL)
include(QuentierFindBoost)
include(QuentierFindHunspell)
include(QuentierFindQtKeychain)
include(QuentierFindQEverCloud)
include(QuentierFindQt4Mimetypes)
include(QuentierFindLibTidyHtml5)
include(QuentierDoxygen)
include(PrependPath)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

set(CREATE_BUNDLE ON CACHE BOOL "Create application bundle")

set(UPDATE_TRANSLATIONS OFF CACHE BOOL "Update translations")

if(WIN32 AND USE_QT5)
  set (DEPLOYQT_TOOL ${Qt5Core_DIR}/../../../bin/windeployqt)
  message(STATUS "Windeployqt path: ${DEPLOYQT_TOOL}")
elseif(APPLE)
  if(USE_QT5)
    set(DEPLOYQT_TOOL ${Qt5Core_DIR}/../../../bin/macdeployqt)
  else()
    get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} PATH)
    set(DEPLOYQT_TOOL ${QT_BIN_DIR}/macdeployqt)
  endif()
  message(STATUS "Macdeployqt path: ${DEPLOYQT_TOOL}")
endif()

if(NOT CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(NOT CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if(NOT CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")
endif()

if(NOT CMAKE_INSTALL_TRANSLATIONSDIR)
  set(CMAKE_INSTALL_TRANSLATIONSDIR "${CMAKE_INSTALL_PREFIX}/translations")
endif()

add_subdirectory(libquentier)
get_directory_property(LIB_QUENTIER DIRECTORY libquentier DEFINITION PROJECT_NAME)
get_directory_property(LIB_QUENTIER_CHECKABLE_SOURCES DIRECTORY libquentier DEFINITION ${LIB_QUENTIER}_CHECKABLE_SOURCES)
get_directory_property(THIRDPARTY_LIBS DIRECTORY libquentier DEFINITION THIRDPARTY_LIBS)
get_directory_property(THIRDPARTY_LIB_DIRS DIRECTORY libquentier DEFINITION THIRDPARTY_LIB_DIRS)
get_directory_property(QUENTIER_USE_QT_WEB_ENGINE DIRECTORY libquentier DEFINITION QUENTIER_USE_QT_WEB_ENGINE)

add_subdirectory(quentier)
get_directory_property(QUENTIER_APP DIRECTORY quentier DEFINITION PROJECT_NAME)
get_directory_property(QUENTIER_APP_SOURCES DIRECTORY quentier DEFINITION ${QUENTIER_APP}_SOURCES)
get_directory_property(QUENTIER_APP_INCLUDE_DIRS DIRECTORY quentier DEFINITION ${QUENTIER_APP}_INCLUDE_DIRS)

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

set(QUENTIER_CPPCHECKABLE_SOURCES "${LIB_QUENTIER_CHECKABLE_SOURCES}")
list(APPEND QUENTIER_CPPCHECKABLE_SOURCES "${QUENTIER_APP_SOURCES}")

list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${QUENTIER_APP_INCLUDE_DIRS}")

include(QuentierSetupCppCheck)
