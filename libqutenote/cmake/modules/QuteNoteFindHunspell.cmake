find_path(HUNSPELL_INCLUDE_DIR hunspell/hunspell.hxx)
if (HUNSPELL_INCLUDE_DIR-NOTFOUND)
  message(FATAL "Can't find development headers for hunsell library")
else()
  include_directories(${HUNSPELL_INCLUDE_DIR})
endif()

find_library(HUNSPELL_LIBRARIES NAMES hunspell-1.3 hunspell-1.2)
if (HUNSPELL_LIBRARIES-NOTFOUND)
  message(FATAL "Can't find hunspell library")
else()
  message(STATUS "Found hunspell library: ${HUNSPELL_LIBRARIES}")
endif()
    
