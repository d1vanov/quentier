cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

if(BREAKPAD_FOUND)
  add_subdirectory(crash_handler)
  add_subdirectory(symbols_compressor)
endif()

add_subdirectory(quentier)
add_subdirectory(wiki2account)
add_subdirectory(wiki2enex)
