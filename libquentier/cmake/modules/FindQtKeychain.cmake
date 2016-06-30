message("Looking for system wide installation of Qt Keychain")
find_path(QTKEYCHAIN_INCLUDE_PATH keychain.h SILENT REQUIRED)
if(NOT QTKEYCHAIN_INCLUDE_PATH)
  message(FATAL_ERROR "Can't find development header keychain.h")
else()
  message(STATUS "Found development header keychain.h in folder ${QTKEYCHAIN_INCLUDE_PATH}")
  find_library(QTKEYCHAIN_LIB
               NAMES libqtkeychain.so libqtkeychain.a libqtkeychain.dylib libqtkeychain.dll qtkeychain.dll libqtkeychain.lib qtkeychain.lib
               SILENT REQUIRED)
  if(NOT QTKEYCHAIN_LIB)
    message(FATAL_ERROR "Can't find Qt Keychain library");
  else()
    message(STATUS "Found Qt Keychain library: " ${QTKEYCHAIN_LIB})
  endif()
endif()
