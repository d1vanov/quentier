if(USE_QT5)
  find_package(Qt5Keychain QUIET REQUIRED)
else()
  find_package(QtKeychain QUIET REQUIRED)
endif()

include_directories(${QTKEYCHAIN_INCLUDE_DIRS})

# Not sure why but sometimes CMake fails to recognize that ${QTKEYCHAIN_LIBRARIES} is a target and issues a weird error message
if(TARGET ${QTKEYCHAIN_LIBRARIES})
  get_property(QTKEYCHAIN_LIBRARY_LOCATION TARGET ${QTKEYCHAIN_LIBRARIES} PROPERTY LOCATION)
  message(STATUS "Found QtKeychain library: ${QTKEYCHAIN_LIBRARY_LOCATION}")
else()
  message(STATUS "Found QtKeychain library")
endif()
