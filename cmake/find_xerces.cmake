# - Try to find Xerces-C
# Once done this will define
#
#  XERCESC_FOUND - system has Xerces-C
#  XERCESC_INCLUDE - the Xerces-C include directory
#  XERCESC_LIBRARY - Link these to use Xerces-C
#  XERCESC_VERSION - Xerces-C found version

if(XERCESC_INCLUDE AND XERCESC_LIBRARY)
  # in cache already
  set(XERCESC_FIND_QUIETLY TRUE)
endif(XERCESC_INCLUDE AND XERCESC_LIBRARY)

find_path(XERCESC_INCLUDE
NAMES xercesc/util/XercesVersion.hpp
PATHS ${CMAKE_INCLUDE_PATH} ${C_INCLUDE_PATH} ${CPLUS_INCLUDE_PATH} ${INCLUDE} ${XERCES_INCLUDE_DIR}
PATH_SUFFIXES xercesc)

find_library(
  XERCESC_LIBRARY
  NAMES xerces-c xerces-c31 xerces-c_31
  PATHS ${XERCES_LIBRARY_DIR})

if(XERCESC_INCLUDE AND XERCESC_LIBRARY)
  set(XERCESC_FOUND TRUE)
else()
  set(XERCESC_FOUND FALSE)
endif()

if(XERCESC_FOUND)
  find_path(XVERHPPPATH 
  NAMES XercesVersion.hpp 
  PATHS ${XERCESC_INCLUDE}/xercesc/util ${C_INCLUDE_PATH}/xercesc/util ${CPLUS_INCLUDE_PATH}/xercesc/util
  ${CMAKE_INCLUDE_PATH}/xercesc/util NO_DEFAULT_PATH)

  if(${XVERHPPPATH} STREQUAL "XVERHPPPATH-NOTFOUND")
    set(XERCES_VERSION "0")
  else()
    file(READ ${XVERHPPPATH}/XercesVersion.hpp XVERHPP)
    string(REGEX MATCHALL "\n *#define XERCES_VERSION_MAJOR +[0-9]+" XVERMAJ ${XVERHPP})
    string(REGEX MATCH "\n *#define XERCES_VERSION_MINOR +[0-9]+" XVERMIN ${XVERHPP})
    string(REGEX MATCH "\n *#define XERCES_VERSION_REVISION +[0-9]+" XVERREV ${XVERHPP})

    string(REGEX REPLACE "\n *#define XERCES_VERSION_MAJOR +" "" XVERMAJ ${XVERMAJ})
    string(REGEX REPLACE "\n *#define XERCES_VERSION_MINOR +" "" XVERMIN ${XVERMIN})
    string(REGEX REPLACE "\n *#define XERCES_VERSION_REVISION +" "" XVERREV ${XVERREV})

    set(XERCESC_VERSION ${XVERMAJ}.${XVERMIN}.${XVERREV})
  endif()

  if(NOT XERCESC_FIND_QUIETLY)
    message(STATUS "Found Xerces-C: ${XERCESC_LIBRARY}")
    message(STATUS "              : ${XERCESC_INCLUDE}")
    message(STATUS "       Version: ${XERCESC_VERSION}")
  endif()
else()
  message(FATAL_ERROR "Could not find Xerces-C!")
endif()

mark_as_advanced(XERCESC_INCLUDE XERCESC_LIBRARY)