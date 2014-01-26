cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)

add_subdirectory(thrift)
get_directory_property(THRIFT_LIB DIRECTORY thrift DEFINITION PROJECT_NAME)
get_directory_property(THRIFT_INCLUDE_DIRS DIRECTORY thrift DEFINITION ${THRIFT_LIB}_INCLUDE_DIRS)

add_subdirectory(kqoauth)
get_directory_property(KQOAUTH_LIB DIRECTORY kqoauth DEFINITION PROJECT_NAME)
get_directory_property(KQOAUTH_SOURCES DIRECTORY kqoauth DEFINITION ${KQOAUTH_LIB}_SOURCES)
get_directory_property(KQOAUTH_INCLUDE_DIRS DIRECTORY kqoauth DEFINITION ${KQOAUTH_LIB}_INCLUDE_DIRS)

add_subdirectory(simplecrypt)
get_directory_property(SIMPLECRYPT_LIB DIRECTORY simplecrypt DEFINITION PROJECT_NAME)
get_directory_property(SIMPLECRYPT_SOURCES DIRECTORY simplecrypt DEFINITION ${SIMPLECRYPT_LIB}_SOURCES)
get_directory_property(SIMPLECRYPT_INCLUDE_DIRS DIRECTORY simplecrypt DEFINITION ${SIMPLECRYPT_LIB}_INCLUDE_DIRS)

add_subdirectory(evernote)
get_directory_property(EVERNOTE_SDK_LIB DIRECTORY evernote DEFINITION PROJECT_NAME)
get_directory_property(EVERNOTE_SDK_INCLUDE_DIRS DIRECTORY evernote DEFINITION ${EVERNOTE_SDK_LIB}_INCLUDE_DIRS)

set(THIRDPARTY_INCLUDE_DIRS
    thrift/src
    thrift/src/thrift
    kqoauth/src
    simplecrypt/src
    evernote/sdk/src)
   
prepend_path(THIRDPARTY_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR})
