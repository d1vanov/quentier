cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

project(quentier)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libquentier/cmake/modules)

include(QuentierCompilerSettings)
include(QuentierLinkerSettings)
include(QuentierSetupQt)
include(QuentierFindLibXml2)
include(QuentierFindOpenSSL)
include(QuentierFindBoost)
include(QuentierFindHunspell)
include(QuentierFindQtKeychain)
include(QuentierFindQEverCloud)
include(QuentierDoxygen)
include(PrependPath)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_subdirectory(libquentier)
get_directory_property(LIB_QUENTIER DIRECTORY libquentier DEFINITION PROJECT_NAME)
get_directory_property(LIB_QUENTIER_CHECKABLE_SOURCES DIRECTORY libquentier DEFINITION ${LIB_QUENTIER}_CHECKABLE_SOURCES)

get_directory_property(THIRDPARTY_LIBS DIRECTORY libquentier DEFINITION THIRDPARTY_LIBS)
get_directory_property(THIRDPARTY_INCLUDE_DIRS DIRECTORY libquentier DEFINITION THIRDPARTY_INCLUDE_DIRS)
get_directory_property(THIRDPARTY_SOURCES DIRECTORY libquentier DEFINITION THIRDPARTY_SOURCES)

add_subdirectory(quentier)
get_directory_property(QUENTIER_APP DIRECTORY quentier DEFINITION PROJECT_NAME)
get_directory_property(QUENTIER_APP_SOURCES DIRECTORY quentier DEFINITION ${QUENTIER_APP}_SOURCES)
get_directory_property(QUENTIER_APP_INCLUDE_DIRS DIRECTORY quentier DEFINITION ${QUENTIER_APP}_INCLUDE_DIRS)

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

set(QUENTIER_CPPCHECKABLE_SOURCES "${LIB_QUENTIER_CHECKABLE_SOURCES}")
list(APPEND QUENTIER_CPPCHECKABLE_SOURCES "${QUENTIER_APP_SOURCES}")

set(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}")
list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${QUENTIER_APP_INCLUDE_DIRS}")

include(QuentierSetupCppCheck)
