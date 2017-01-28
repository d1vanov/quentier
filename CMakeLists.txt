cmake_minimum_required(VERSION 2.8)

project(quentier)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libquentier/cmake/modules)

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
if(WIN32 AND USE_QT5)
  set (WINDEPLOYQT_APP ${Qt5Core_DIR}/../../../bin/windeployqt)
  message(STATUS "Windeployqt path: ${WINDEPLOYQT_APP}")
endif()

add_subdirectory(libquentier)
get_directory_property(LIB_QUENTIER DIRECTORY libquentier DEFINITION PROJECT_NAME)
get_directory_property(LIB_QUENTIER_CHECKABLE_SOURCES DIRECTORY libquentier DEFINITION ${LIB_QUENTIER}_CHECKABLE_SOURCES)
get_directory_property(THIRDPARTY_LIBS DIRECTORY libquentier DEFINITION THIRDPARTY_LIBS)
get_directory_property(THIRDPARTY_LIB_DIRS DIRECTORY libquentier DEFINITION THIRDPARTY_LIB_DIRS)

add_subdirectory(quentier)
get_directory_property(QUENTIER_APP DIRECTORY quentier DEFINITION PROJECT_NAME)
get_directory_property(QUENTIER_APP_SOURCES DIRECTORY quentier DEFINITION ${QUENTIER_APP}_SOURCES)
get_directory_property(QUENTIER_APP_INCLUDE_DIRS DIRECTORY quentier DEFINITION ${QUENTIER_APP}_INCLUDE_DIRS)

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

set(QUENTIER_CPPCHECKABLE_SOURCES "${LIB_QUENTIER_CHECKABLE_SOURCES}")
list(APPEND QUENTIER_CPPCHECKABLE_SOURCES "${QUENTIER_APP_SOURCES}")

list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${QUENTIER_APP_INCLUDE_DIRS}")

include(QuentierSetupCppCheck)
