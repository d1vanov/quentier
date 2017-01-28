if(NOT HUNSPELL_INCLUDE_DIR AND NOT HUNSPELL_LIBRARIES)
  find_path(HUNSPELL_INCLUDE_DIR NAMES hunspell/hunspell.hxx PATHS ${CMAKE_INCLUDE_PATH})
  if(NOT HUNSPELL_INCLUDE_DIR)
    message(FATAL_ERROR "Can't find development headers for hunspell library")
  else()
    include_directories(${HUNSPELL_INCLUDE_DIR})
  endif()

  find_library(HUNSPELL_LIBRARIES
               NAMES
               hunspell hunspell-1.6 hunspell-1.5 hunspell-1.4 hunspell-1.3 hunspell-1.2
               libhunspell libhunspell-1.6 libhunspell-1.5 libhunspell-1.4 libhunspell-1.3 libhunspell-1.2)
  if(NOT HUNSPELL_LIBRARIES)
    message(FATAL_ERROR "Can't find hunspell library")
  else()
    message(STATUS "Found hunspell library: ${HUNSPELL_LIBRARIES}")
  endif()
endif()

get_filename_component(HUNSPELL_LIB_DIR "${HUNSPELL_LIBRARIES}" PATH)
