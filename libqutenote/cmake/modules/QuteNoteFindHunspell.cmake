find_path(HUNSPELL_INCLUDE_DIR hunspell/hunspell.hxx)
if (${HUNSPELL_INCLUDE_DIR} STREQUAL "HUNSPELL_INCLUDE_DIR-NOTFOUND")
  message(FATAL "Can't find development headers for hunspell library")
else()
  include_directories(${HUNSPELL_INCLUDE_DIR})
endif()

find_library(HUNSPELL_LIBRARIES NAMES hunspell hunspell-1.3 hunspell-1.2 libhunspell libhunspell-1.3 libhunspell-1.2)
if (${HUNSPELL_LIBRARIES} STREQUAL "HUNSPELL_LIBRARIES-NOTFOUND")
  message(FATAL "Can't find hunspell library")
else()
  message(STATUS "Found hunspell library: ${HUNSPELL_LIBRARIES}")
endif()
    
