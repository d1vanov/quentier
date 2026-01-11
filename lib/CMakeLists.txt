cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

add_subdirectory(account)
add_subdirectory(delegate)
add_subdirectory(dialog)
add_subdirectory(enex)
add_subdirectory(exception)
add_subdirectory(initialization)
add_subdirectory(model)
add_subdirectory(network)
add_subdirectory(preferences)
add_subdirectory(synchronization)
add_subdirectory(tray)

if(INCLUDE_UPDATE_INFO)
  add_subdirectory(update)
endif()

add_subdirectory(utility)
add_subdirectory(view)
add_subdirectory(widget)

if(QUENTIER_BUILD_TEST_TOOLS)
  add_subdirectory(wiki2note)
endif()
