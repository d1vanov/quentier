cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

if(BREAKPAD_FOUND)
  add_subdirectory(crash_handler)
  add_subdirectory(symbols_compressor)
endif()

add_subdirectory(quentier)

if(BUILD_WITH_WIKI_TOOLS)
  add_subdirectory(wiki2account)
  add_subdirectory(wiki2enex)
endif()
