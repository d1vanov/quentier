cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

project(QuteNote)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libqutenote/cmake/modules)

include(QuteNoteCompilerSettings)
include(QuteNoteLinkerSettings)
include(QuteNoteSetupQt)
include(QuteNoteFindLibXml2)
include(QuteNoteFindOpenSSL)
include(QuteNoteFindBoost)
include(QuteNoteFindHunspell)
include(PrependPath)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_subdirectory(libqutenote)
get_directory_property(LIB_QUTE_NOTE DIRECTORY libqutenote DEFINITION PROJECT_NAME)
get_directory_property(LIB_QUTE_NOTE_CHECKABLE_SOURCES DIRECTORY libqutenote DEFINITION ${LIB_QUTE_NOTE}_CHECKABLE_SOURCES)

get_directory_property(THIRDPARTY_LIBS DIRECTORY libqutenote DEFINITION THIRDPARTY_LIBS)
get_directory_property(THIRDPARTY_INCLUDE_DIRS DIRECTORY libqutenote DEFINITION THIRDPARTY_INCLUDE_DIRS)
get_directory_property(THIRDPARTY_SOURCES DIRECTORY libqutenote DEFINITION THIRDPARTY_SOURCES)

add_subdirectory(qutenote)
get_directory_property(QUTE_NOTE_APP DIRECTORY qutenote DEFINITION PROJECT_NAME)
get_directory_property(QUTE_NOTE_APP_SOURCES DIRECTORY qutenote DEFINITION ${QUTE_NOTE_APP}_SOURCES)
get_directory_property(QUTE_NOTE_APP_INCLUDE_DIRS DIRECTORY qutenote DEFINITION ${QUTE_NOTE_APP}_INCLUDE_DIRS)

set(QUTE_NOTE_CPPCHECKABLE_SOURCES "${LIB_QUTE_NOTE_CHECKABLE_SOURCES}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_SOURCES "${QUTE_NOTE_APP_SOURCES}")

set(QUTE_NOTE_CPPCHECKABLE_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_INCLUDE_DIRS "${QUTE_NOTE_APP_INCLUDE_DIRS}")

include(QuteNoteSetupCppCheck)
