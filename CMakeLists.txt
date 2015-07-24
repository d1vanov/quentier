cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(QuteNote)

include("cmake/compiler.cmake")
include("cmake/qt.cmake")
include("cmake/libxml2.cmake")
include("cmake/openssl.cmake")
include("cmake/prepend.cmake")
include("cmake/boost.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_subdirectory(libqutenote)
get_directory_property(LIB_QUTE_NOTE DIRECTORY libqutenote DEFINITION PROJECT_NAME)
get_directory_property(LIB_QUTE_NOTE_SOURCES DIRECTORY libqutenote DEFINITION ${LIB_QUTE_NOTE}_SOURCES)
get_directory_property(LIB_QUTE_NOTE_INCLUDE_DIRS DIRECTORY libqutenote DEFINITION ${LIB_QUTE_NOTE}_INCLUDE_DIRS)

get_directory_property(SIMPLECRYPT_LIB DIRECTORY libqutenote DEFINITION SIMPLECRYPT_LIB)
get_directory_property(QEVERCLOUD_LIB DIRECTORY libqutenote DEFINITION QEVERCLOUD_LIB)
get_directory_property(QTKEYCHAIN_LIB DIRECTORY libqutenote DEFINITION QTKEYCHAIN_LIB)
get_directory_property(STACKTRACE_LIB DIRECTORY libqutenote DEFINITION STACKTRACE_LIB)
get_directory_property(QSLOG_LIB DIRECTORY libqutenote DEFINITION QSLOG_LIB)
get_directory_property(QT4_MIMETYPES_LIB DIRECTORY libqutenote DEFINITION QT4_MIMETYPES_LIB)
get_directory_property(TIDY_HTML5_LIB DIRECTORY libqutenote DEFINITION TIDY_HTML5_LIB)

get_directory_property(THIRDPARTY_INCLUDE_DIRS DIRECTORY libqutenote DEFINITION THIRDPARTY_INCLUDE_DIRS)
get_directory_property(THIRDPARTY_SOURCES DIRECTORY libqutenote DEFINITION THIRDPARTY_SOURCES)

add_subdirectory(gui)
get_directory_property(QUTE_NOTE_APP DIRECTORY gui DEFINITION PROJECT_NAME)
get_directory_property(QUTE_NOTE_APP_SOURCES DIRECTORY gui DEFINITION ${QUTE_NOTE_APP}_SOURCES)
get_directory_property(QUTE_NOTE_APP_INCLUDE_DIRS DIRECTORY gui DEFINITION ${QUTE_NOTE_APP}_INCLUDE_DIRS)

set(QUTE_NOTE_CPPCHECKABLE_SOURCES "${THIRDPARTY_SOURCES}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_SOURCES "${LIB_QUTE_NOTE_SOURCES}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_SOURCES "${QUTE_NOTE_APP_SOURCES}")

set(QUTE_NOTE_CPPCHECKABLE_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_INCLUDE_DIRS "${LIB_QUTE_NOTE_INCLUDE_DIRS}")
list(APPEND QUTE_NOTE_CPPCHECKABLE_INCLUDE_DIRS "${QUTE_NOTE_APP_INCLUDE_DIRS}")

include("cmake/cppcheck.cmake")
