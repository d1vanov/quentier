if(USE_QT5)
  find_package(Qt5Keychain REQUIRED)
else()
  find_package(QtKeychain REQUIRED)
endif()

include_directories(${QTKEYCHAIN_INCLUDE_DIRS})

get_property(QTKEYCHAIN_LIBRARY_LOCATION TARGET ${QTKEYCHAIN_LIBRARIES} PROPERTY LOCATION)
message(STATUS "Found QtKeychain library: ${QTKEYCHAIN_LIBRARY_LOCATION}")
