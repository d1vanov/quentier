function(CreateQuentierBundle)
  if(NOT CREATE_BUNDLE)
    return()
  endif()

  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(NOT USE_QT5)
      message(STATUS "Deployment on Linux is only supported with Qt5")
      return()
    endif()
    set(ARCH "")
    execute_process(COMMAND /bin/uname -m
                    OUTPUT_VARIABLE ARCH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
  elseif((NOT WIN32) AND (NOT APPLE))
    message(STATUS "The application bundle creation is not supported on this platform")
    return()
  endif()

  # Need several libraries for packaging. Most of them should have been found along with libquentier:
  # 1) OpenSLL
  # 2) QEverCloud (Qt4 or Qt5 version)
  # 3) QtKeychain (Qt4 or Qt5 version)
  # 4) libxml2
  # 5) libtidy5
  # 6) libhunspell
  # 7) qt4-mimetypes (in Qt4 version only)
  # 
  # However, there are a few more dependencies. One group is represented by the dependencies of libquentier's dependencie:
  # 8) iconv (for libxml2)
  # 9) zlib (for libxml2 / iconv)
  # 
  # And, of course, need to deploy libquentier itself
  # 10) libquentier
  # 
  # Also, quentier has one or more additional dependencies of its own:
  # 11) boost program options
  # 12) Google breakpad, if used

  message(STATUS "Searching for additional dependencies for deployment")

  find_package(Iconv REQUIRED)
  find_package(ZLIB REQUIRED)
  
  # Need to collect the dirs of all thirdparty libraries
  set(THIRDPARTY_LIB_DIRS "")

  # 1) OpenSSL
  set(OPENSSL_LIB_DIRS "")
  foreach(OPENSSL_LIB ${OPENSSL_LIBRARIES})
    get_filename_component(_CURRENT_OPENSSL_LIB_DIR "${OPENSSL_LIB}" PATH)
    if(${_CURRENT_OPENSSL_LIB_DIR} MATCHES "[/]")
      list(APPEND OPENSSL_LIB_DIRS "${_CURRENT_OPENSSL_LIB_DIR}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES OPENSSL_LIB_DIRS)
  list(APPEND THIRDPARTY_LIB_DIRS ${OPENSSL_LIB_DIRS})

  # 2) QEverCloud 
  get_property(QEVERCLOUD_LIBRARY_LOCATION TARGET ${QEVERCLOUD_LIBRARIES} PROPERTY LOCATION)
  get_filename_component(QEVERCLOUD_LIB_DIR "${QEVERCLOUD_LIBRARY_LOCATION}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${QEVERCLOUD_LIB_DIR}")
  
  # 3) QtKeychain
  get_property(QTKEYCHAIN_LIBRARY_LOCATION TARGET ${QTKEYCHAIN_LIBRARIES} PROPERTY LOCATION)
  get_filename_component(QTKEYCHAIN_LIB_DIR "${QTKEYCHAIN_LIBRARY_LOCATION}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${QTKEYCHAIN_LIB_DIR}")

  # 4) libxml2
  get_filename_component(LIBXML2_LIB_DIR "${LIBXML2_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${LIBXML2_LIB_DIR}")

  # 5) libtidy5
  get_filename_component(TIDY_LIB_DIR "${LIB_TIDY_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${TIDY_LIB_DIR}")

  # 6) libhunspell
  get_filename_component(HUNSPELL_LIB_DIR "${HUNSPELL_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${HUNSPELL_LIB_DIR}")

  # 7) qt4-mimetypes
  if(NOT USE_QT5)
    get_property(QT4-MIMETYPES_LIBRARY_LOCATION TARGET ${QT4-MIMETYPES_LIBRARIES} PROPERTY LOCATION)
    get_filename_component(QT4-MIMETYPES_LIB_DIR "${QT4-MIMETYPES_LIBRARY_LOCATION}" PATH)
    list(APPEND THIRDPARTY_LIB_DIRS "${QT4-MIMETYPES_LIB_DIR}")
  endif()

  # 8) iconv
  get_filename_component(ICONV_LIB_DIR "${ICONV_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${ICONV_LIB_DIR}")

  # 9) zlib
  get_filename_component(ZLIB_LIB_DIR "${ZLIB_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${ZLIB_LIB_DIR}")

  # 10) libquentier
  get_property(LIBQUENTIER_LIBRARY_LOCATION TARGET ${LIBQUENTIER_LIBRARIES} PROPERTY LOCATION)
  get_filename_component(LIBQUENTIER_LIB_DIR "${LIBQUENTIER_LIBRARY_LOCATION}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${LIBQUENTIER_LIB_DIR}")

  # 11) Boost program options
  get_filename_component(BOOST_LIB_DIR "${Boost_LIBRARIES}" PATH)
  list(APPEND THIRDPARTY_LIB_DIRS "${BOOST_LIB_DIR}")

  # 12) Google breakpad - include only on non-Windows platforms due to the following:
  # dump_syms.exe doesn't have any runtime dependencies and minidump_stackwalk.exe
  # requires some Cygwin dlls but the latter ones cannot be properly deployed
  # by CMake's fixup_bundle because it assumes all binaries built with MSVC
  # and freaks out from Cygwin dlls' binary format
  if(NOT WIN32 AND BREAKPAD_FOUND)
    list(APPEND THIRDPARTY_LIB_DIRS "${BREAKPAD_LIBRARY_DIRS}")
  endif()

  # Finally, remove the duplicates from the lib dirs list
  list(REMOVE_DUPLICATES THIRDPARTY_LIB_DIRS)

  if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      set(DEBUG_SUFFIX "d")
    else()
      set(DEBUG_SUFFIX "")
    endif()
    set(APPS "${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}.exe")
  elseif(APPLE)
    set(APPS "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
  else()
    set(APPS "${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}")
  endif()

  if(WIN32 AND USE_QT5)
    set(DEPLOYQT_TOOL "${Qt5Core_DIR}/../../../bin/windeployqt")
    message(STATUS "Windeployqt path: ${DEPLOYQT_TOOL}")
  elseif(APPLE)
    if(USE_QT5)
      set(DEPLOYQT_TOOL "${Qt5Core_DIR}/../../../bin/macdeployqt")
    else()
      get_filename_component(QT_BIN_DIR ${QT_QMAKE_EXECUTABLE} PATH)
      set(DEPLOYQT_TOOL "${QT_BIN_DIR}/macdeployqt")
    endif()
    message(STATUS "Macdeployqt path: ${DEPLOYQT_TOOL}")
  else()
    set(DEPLOYQT_TOOL ${LINUXDEPLOYQT})
    message(STATUS "Linuxdeployqt path: ${DEPLOYQT_TOOL}")
  endif()

  set(DIRS ${THIRDPARTY_LIB_DIRS})

  # We know the locations of thirdparty libraries but on Windows some of them are .lib files
  # and here we need .dll ones. Usually they are stored in sibling "bin" directories
  # so using this hint
  foreach(THIRDPARTY_LIB_DIR ${THIRDPARTY_LIB_DIRS})
    list(APPEND DIRS "${THIRDPARTY_LIB_DIR}/../bin")
  endforeach()

  if(WIN32)
    if(USE_QT5)
      set(WINDEPLOYQT_OPTIONS "--no-compiler-runtime")
      if(MINGW AND ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        # Without this windeployqt thinks the binary is the debug one and deploys a ton of debug Qt libraries of huge weight
        set(WINDEPLOYQT_OPTIONS "${WINDEPLOYQT_OPTIONS} --release")
      endif()
      install(CODE "
              message(STATUS \"Running deploy Qt tool: ${DEPLOYQT_TOOL}\")
              execute_process(COMMAND ${DEPLOYQT_TOOL} ${WINDEPLOYQT_OPTIONS} ${APPS})
              " COMPONENT Runtime)

      if(LIBQUENTIER_USE_QT_WEB_ENGINE)
        set(QTWEBENGINEPROCESS "${Qt5Core_DIR}/../../../bin/QtWebEngineProcess${DEBUG_SUFFIX}.exe")
        install(FILES "${QTWEBENGINEPROCESS}" DESTINATION "${CMAKE_INSTALL_BINDIR}")
        install(DIRECTORY "${Qt5Core_DIR}/../../../resources" DESTINATION "${CMAKE_INSTALL_BINDIR}")
        install(DIRECTORY "${Qt5Core_DIR}/../../../translations/qtwebengine_locales" DESTINATION "${CMAKE_INSTALL_BINDIR}/translations")
      endif()

      # deploying the SQLite driver which windeployqt/macdeployqt misses for some reason
      install(FILES "${Qt5Core_DIR}/../../../plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll" DESTINATION ${CMAKE_INSTALL_BINDIR}/sqldrivers)

      # fixup other dependencies not taken care of by windeployqt/macdeployqt
      install(CODE "
              include(CMakeParseArguments)
              include(${PROJECT_SOURCE_DIR}/cmake/modules/BundleUtilities.cmake)
              include(InstallRequiredSystemLibraries)
              fixup_bundle(${APPS}   \"\"   \"${DIRS}\" IGNORE_ITEM \"quentier_minidump_stackwalk.exe;\")
              " COMPONENT Runtime)
    else()
      install(CODE "
              include(DeployQt4)
              include(InstallRequiredSystemLibraries)
              fixup_qt4_executable(${APPS} \"qsqlite\" \"\" \"${DIRS}\" IGNORE_ITEM \"quentier_minidump_stackwalk.exe;\")
              " COMPONENT Runtime)
    endif(USE_QT5)

    # Forcefully installing OpenSSL dlls, they don't seem to be included for unknown reason
    install(CODE "
            set(OPENSSL_POTENTIAL_ROOT_DIRS \"\")
            foreach(OPENSSL_LIB_DIR \"${OPENSSL_LIB_DIRS}\")
              list(APPEND OPENSSL_POTENTIAL_ROOT_DIRS \"\${OPENSSL_LIB_DIR}/..\")
              list(APPEND OPENSSL_POTENTIAL_ROOT_DIRS \"\${OPENSSL_LIB_DIR}/../..\")
            endforeach()
            list(REMOVE_DUPLICATES OPENSSL_POTENTIAL_ROOT_DIRS)
            foreach(OPENSSL_ROOT_DIR \${OPENSSL_POTENTIAL_ROOT_DIRS})
              message(STATUS \"Checking whether file exists: \${OPENSSL_ROOT_DIR}/bin/libeay32.dll\")
              if(EXISTS \"\${OPENSSL_ROOT_DIR}/bin/libeay32.dll\")
                message(STATUS \"Exists, copying\")
                file(COPY \"\${OPENSSL_ROOT_DIR}/bin/libeay32.dll\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              elseif(EXISTS \"\${OPENSSL_ROOT_DIR}/libeay32.dll\")
                message(STATUS \"Another file exists: \${OPENSSL_ROOT_DIR}/libeay32.dll, copying\")
                file(COPY \"\${OPENSSL_ROOT_DIR}/libeay32.dll\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              endif()
              message(STATUS \"Checking whether file exists: \${OPENSSL_ROOT_DIR}/bin/ssleay32.dll\")
              if(EXISTS \"\${OPENSSL_ROOT_DIR}/bin/ssleay32.dll\")
                message(STATUS \"Exists, copying\")
                file(COPY \"\${OPENSSL_ROOT_DIR}/bin/ssleay32.dll\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              elseif(EXISTS \"\${OPENSSL_ROOT_DIR}/ssleay32.dll\")
                message(STATUS \"Another file exists: \${OPENSSL_ROOT_DIR}/ssleay32.dll, copying\")
                file(COPY \"\${OPENSSL_ROOT_DIR}/ssleay32.dll\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              endif()
              message(STATUS \"Checking whether file exists: \${OPENSSL_ROOT_DIR}/libssl32.dll\")
              if(EXISTS \"\${OPENSSL_ROOT_DIR}/libssl32.dll\")
                message(STATUS \"Exists, copying\")
                file(COPY \"\${OPENSSL_ROOT_DIR}/libssl32.dll\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              endif()
            endforeach()
            " COMPONENT Runtime)

    # Removing redundant SQL drivers deployed by windeployqt and/or fixup_bundle/fixup_qt4_executable
    install(CODE "
            file(GLOB sqlDrivers \"${CMAKE_INSTALL_BINDIR}/sqldrivers/*.dll\")
            foreach(sqlDriver \${sqlDrivers})
              get_filename_component(sqlDriverBaseName \${sqlDriver} NAME)
              if(NOT \${sqlDriverBaseName} MATCHES \"qsqlite.dll\")
                if(NOT \${sqlDriverBaseName} MATCHES \"qsqlited.dll\")
                  message(STATUS \"Removing redundant SQL driver: \${sqlDriverBaseName}\")
                  file(REMOVE \${sqlDriver})
                endif()
              endif()
            endforeach()
            " COMPONENT Runtime)

    if(MSVC)
      # Removing VC runtime deployed by fixup_bundle/fixup_qt4_executable
      install(CODE "
              file(GLOB runtimes \"${CMAKE_INSTALL_BINDIR}/vcruntime*.dll\")
              foreach(runtime \${runtimes})
                get_filename_component(runtimeBaseName \${runtime} NAME)
                message(STATUS \"Removing automatically deployed VC runtime: \${runtimeBaseName}\")
                file(REMOVE \${runtime})
              endforeach()
              " COMPONENT Runtime)
    elseif(MINGW)
      # MinGW built binary for some reason searches for dlls first on PATH and only then at its folder
      # Working around this: installing custom bat file for launching of quentier.exe with empty PATH
      install(CODE "
              file(COPY \"${PROJECT_SOURCE_DIR}/src/installer/windows/quentier.bat\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              file(COPY \"${PROJECT_SOURCE_DIR}/src/installer/windows/quentier_launcher.vbs\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")
              " COMPONENT Runtime)
    endif()

    if (BREAKPAD_FOUND)
      # need to do some manual steps for deploying the dependencies of quentier_minidump_stackwalk.exe - Cygwin dlls
      get_filename_component(BREAKPAD_STACKWALKER_DIR ${BREAKPAD_STACKWALKER} DIRECTORY)
      file(GLOB cygwin_dlls "${BREAKPAD_STACKWALKER_DIR}/cy*.dll")
      foreach(cygwin_dll ${cygwin_dlls})
        install(CODE "file(COPY \"${cygwin_dll}\" DESTINATION \"${CMAKE_INSTALL_BINDIR}\")")
      endforeach()
    endif()

    if(NSIS_FOUND AND CREATE_INSTALLER)
      install(CODE "
              execute_process(COMMAND \"${NSIS_MAKE}\" \"${PROJECT_BINARY_DIR}/wininstaller.nsi\")
	            " COMPONENT Runtime)
    endif()
  elseif(APPLE)
    install(CODE "
            message(STATUS \"Running deploy Qt tool: ${DEPLOYQT_TOOL}\")
            execute_process(COMMAND ${DEPLOYQT_TOOL} ${APPS} ERROR_QUIET)
            execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -add_rpath @executable_path/../Frameworks ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME})
            " COMPONENT Runtime)
  else()
    install(CODE "
            message(STATUS \"Running deploy Qt tool: ${DEPLOYQT_TOOL}\")
            set(ENV{PATH} ${Qt5Core_DIR}/../../../bin:${CMAKE_INSTALL_PREFIX}/bin:$ENV{PATH})
            set(ENV{LD_LIBRARY_PATH} \"${Qt5Core_DIR}/../../../lib:${DIRS}\")
            set(ENV{ARCH} \"${ARCH}\")
            execute_process(WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/..\"
                            COMMAND \"${DEPLOYQT_TOOL}\" ${CMAKE_INSTALL_PREFIX}/share/applications/org.quentier.Quentier.desktop -verbose=0 -bundle-non-qt-libs)
            execute_process(WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/..\"
                            COMMAND \"${DEPLOYQT_TOOL}\" ${CMAKE_INSTALL_PREFIX}/share/applications/org.quentier.Quentier.desktop -appimage)
            " COMPONENT Runtime)
  endif()
endfunction()
