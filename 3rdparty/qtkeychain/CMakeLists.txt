cmake_minimum_required(VERSION 2.8)
project(qtkeychain)

set(QTKEYCHAIN_VERSION 0.4.90)
set(QTKEYCHAIN_SOVERSION 0)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake/Modules")

if(USE_QT5)
  set(QTKEYCHAIN_VERSION_INFIX 5)
  if(UNIX AND NOT APPLE)
    macro(qt_add_dbus_interface)
      qt5_add_dbus_interface(${ARGN})
    endmacro()
  endif()
  macro(qt_add_translation)
    qt5_add_translation(${ARGN})
  endmacro(qt_add_translation)
  macro(qt_create_translation)
    qt5_create_translation(${ARGN})
  endmacro(qt_create_translation)
else()
  set(QTKEYCHAIN_VERSION_INFIX "")
  if(UNIX AND NOT APPLE)
    macro(qt_add_dbus_interface)
      qt4_add_dbus_interface(${ARGN})
    endmacro()
  endif()
  macro(qt_add_translation)
    qt4_add_translation(${ARGN})
  endmacro(qt_add_translation)
  macro(qt_create_translation)
    qt4_create_translation(${ARGN})
  endmacro(qt_create_translation)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

list(APPEND qtkeychain_LIBRARIES ${QT_LIBRARIES})
set(qtkeychain_SOURCES
    keychain.cpp
)

if(WIN32)
    list(APPEND qtkeychain_SOURCES keychain_win.cpp)
    list(APPEND qtkeychain_LIBRARIES crypt32)
    #FIXME: mingw bug; otherwise getting undefined refs to RtlSecureZeroMemory there
    if(MINGW)
        add_definitions( -O2 )
    endif()
endif()

if(APPLE)
    list(APPEND qtkeychain_SOURCES keychain_mac.cpp)

    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    list(APPEND qtkeychain_LIBRARIES ${COREFOUNDATION_LIBRARY})

    find_library(SECURITY_LIBRARY Security)
    list(APPEND qtkeychain_LIBRARIES ${SECURITY_LIBRARY})
endif()

if(UNIX AND NOT APPLE)
    list(APPEND qtkeychain_SOURCES keychain_unix.cpp gnomekeyring.cpp)
    qt_add_dbus_interface(qtkeychain_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.KWallet.xml kwallet_interface KWalletInterface)
endif()

set(qtkeychain_TR_FILES
      translations/qtkeychain_de.ts
      translations/qtkeychain_ro.ts
)

file(GLOB qtkeychain_TR_SOURCES *.cpp *.h *.ui)
qt_create_translation(qtkeychain_MESSAGES ${qtkeychain_TR_SOURCES} ${qtkeychain_TR_FILES})
qt_add_translation(qtkeychain_QM_FILES ${qtkeychain_TR_FILES})
add_custom_target(messages DEPENDS ${qtkeychain_MESSAGES})
add_custom_target(translations DEPENDS ${qtkeychain_QM_FILES})

install(FILES ${qtkeychain_QM_FILES}
        DESTINATION translations)

set(QTKEYCHAIN_TARGET_NAME qt${QTKEYCHAIN_VERSION_INFIX}keychain)
if(NOT QTKEYCHAIN_STATIC)
    add_library(${QTKEYCHAIN_TARGET_NAME} SHARED ${qtkeychain_SOURCES} ${qtkeychain_QM_FILES})
    set_target_properties(${QTKEYCHAIN_TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS QKEYCHAIN_BUILD_QKEYCHAIN_LIB)
    target_link_libraries(${QTKEYCHAIN_TARGET_NAME} ${qtkeychain_LIBRARIES})
else()
    add_library(${QTKEYCHAIN_TARGET_NAME} STATIC ${qtkeychain_SOURCES} ${qtkeychain_QM_FILES})
    set_target_properties(${QTKEYCHAIN_TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS QKEYCHAIN_STATICLIB)
endif()

set_target_properties(${QTKEYCHAIN_TARGET_NAME} PROPERTIES
    VERSION ${QTKEYCHAIN_VERSION}
    SOVERSION ${QTKEYCHAIN_SOVERSION}
    MACOSX_RPATH 1
    INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
)

install(FILES keychain.h qkeychain_export.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/qt${QTKEYCHAIN_VERSION_INFIX}keychain/
)

install(TARGETS ${QTKEYCHAIN_TARGET_NAME}
  EXPORT Qt${QTKEYCHAIN_VERSION_INFIX}KeychainLibraryDepends
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib 
  ARCHIVE DESTINATION lib 
)

add_executable( testclient testclient.cpp )

if(LIBCPP)
  target_link_libraries( testclient ${QTKEYCHAIN_TARGET_NAME} ${LIBCPP})
else()
  target_link_libraries( testclient ${QTKEYCHAIN_TARGET_NAME})
endif()


###
### CMake config file
###

export(TARGETS ${QTKEYCHAIN_TARGET_NAME} FILE "${PROJECT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainLibraryDepends.cmake")
export(PACKAGE Qt${QTKEYCHAIN_VERSION_INFIX}Keychain)

configure_file(QtKeychainBuildTreeSettings.cmake.in
  "${PROJECT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainBuildTreeSettings.cmake" @ONLY)
configure_file(QtKeychainConfig.cmake.in
  "${PROJECT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfig.cmake" @ONLY)
configure_file(QtKeychainConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfigVersion.cmake" @ONLY)

install(EXPORT Qt${QTKEYCHAIN_VERSION_INFIX}KeychainLibraryDepends
        DESTINATION lib/cmake/Qt${QTKEYCHAIN_VERSION_INFIX}Keychain
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/Qt${QTKEYCHAIN_VERSION_INFIX}KeychainConfigVersion.cmake
        DESTINATION lib/cmake/Qt${QTKEYCHAIN_VERSION_INFIX}Keychain
)

