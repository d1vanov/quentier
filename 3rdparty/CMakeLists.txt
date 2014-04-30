cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

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

add_subdirectory(QEverCloud)
get_directory_property(QEVERCLOUD_LIB DIRECTORY QEverCloud DEFINITION PROJECT_NAME)
get_directory_property(QEVERCLOUD_INCLUDE_DIRS DIRECTORY QEverCloud DEFINITION ${QEVERCLOUD_LIB}_INCLUDE_DIRS)

add_subdirectory(backward-cpp)
get_directory_property(BACKWARDCPP_LIB DIRECTORY backward-cpp DEFINITION PROJECT_NAME)
get_directory_property(BACKWARDCPP_INCLUDE_DIRS DIRECTORY backward-cpp DEFINITION ${BACKWARDCPP_LIB}_INCLUDE_DIRS)

set(THIRDPARTY_INCLUDE_DIRS
    thrift/src
    thrift/src/thrift
    kqoauth/src
    simplecrypt/src
    evernote/sdk/src
    QEverCloud/include
    backward-cpp)
   
prepend_path(THIRDPARTY_INCLUDE_DIRS "${THIRDPARTY_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR})
