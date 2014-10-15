cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

add_subdirectory(simplecrypt)
get_directory_property(SIMPLECRYPT_LIB DIRECTORY simplecrypt DEFINITION PROJECT_NAME)
get_directory_property(SIMPLECRYPT_SOURCES DIRECTORY simplecrypt DEFINITION ${SIMPLECRYPT_LIB}_SOURCES)

add_subdirectory(QEverCloud)
get_directory_property(QEVERCLOUD_LIB DIRECTORY QEverCloud DEFINITION PROJECT_NAME)

add_subdirectory(qtkeychain)
get_directory_property(QTKEYCHAIN_LIB DIRECTORY qtkeychain DEFINITION PROJECT_NAME)

add_subdirectory(stacktrace)
get_directory_property(STACKTRACE_LIB DIRECTORY stacktrace DEFINITION PROJECT_NAME)

set(THIRDPARTY_INCLUDE_DIRS
    simplecrypt/src
    QEverCloud/include
    qtkeychain
    stacktrace)
   
prepend_path(THIRDPARTY_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR})
