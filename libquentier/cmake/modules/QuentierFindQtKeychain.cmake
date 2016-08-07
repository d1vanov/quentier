if(USE_QT5)
  find_package(Qt5Keychain REQUIRED)
else()
  find_package(QtKeychain REQUIRED)
endif()

include_directories(${QTKEYCHAIN_INCLUDE_DIRS})
