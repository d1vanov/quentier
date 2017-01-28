if(USE_QT5)
  find_package(Qt5Keychain QUIET REQUIRED)
else()
  find_package(QtKeychain QUIET REQUIRED)
endif()

include_directories(${QTKEYCHAIN_INCLUDE_DIRS})

get_property(QTKEYCHAIN_LIBRARY_LOCATION TARGET ${QTKEYCHAIN_LIBRARIES} PROPERTY LOCATION)
message(STATUS "Found QtKeychain library: ${QTKEYCHAIN_LIBRARY_LOCATION}")

get_filename_component(QTKEYCHAIN_LIB_DIR "${QTKEYCHAIN_LIBRARY_LOCATION}" PATH)
