find_package(Breakpad)
if(NOT BREAKPAD_FOUND)
  message(STATUS "Google Breakpad was not found, will build without it")
else()
  include_directories(SYSTEM ${BREAKPAD_INCLUDE_DIRS})
endif()
