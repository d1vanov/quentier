cmake_minimum_required(VERSION 2.8)

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(quentier VERSION 0.5.0)
else()
  project(quentier)
  set(PROJECT_VERSION_MAJOR "0")
  set(PROJECT_VERSION_MINOR "5")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2018")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(QUENTIER_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(QUENTIER_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(QUENTIER_VERSION_PATCH ${PROJECT_VERSION_PATCH})

if(APPLE)
  set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_NAME} ${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "${PROJECT_COPYRIGHT_YEAR} ${PROJECT_VENDOR}")
  set(MACOSX_BUNDLE_ICON_FILE "quentier_icon.icns")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_DOMAIN_SECOND}.${PROJECT_DOMAIN_FIRST}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "Quentier")

  set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources")
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})
endif()

set(HEADERS
    MainWindow.h
    MainWindowSideBordersController.h)

set(SOURCES
    MainWindow.cpp
    MainWindowSideBordersController.cpp
    main.cpp)

set(FORMS
    MainWindow.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

if(WIN32)
  configure_file(quentier.rc.in
                 ${PROJECT_BINARY_DIR}/quentier.rc @ONLY)
  list(APPEND SOURCES ${PROJECT_BINARY_DIR}/quentier.rc)
endif()

qt_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${HEADERS}
               ${SOURCES}
               ${FORMS_HEADERS}
               ${RESOURCES_RCC})

add_sanitizers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
                      quentier_account
                      quentier_delegate
                      quentier_dialog
                      quentier_enex
                      quentier_exception
                      quentier_initialization
                      quentier_model
                      quentier_preferences
                      quentier_tray
                      quentier_utility
                      quentier_view
                      quentier_widget
                      ${THIRDPARTY_LIBS})

if(BREAKPAD_FOUND)
  # 1) Generate symbols for the executable
  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${BREAKPAD_DUMP_SYMS} $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.syms)

  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${symbols_compressor} ${PROJECT_NAME}.syms)

  # 2) Generate symbols for the core library
  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${BREAKPAD_DUMP_SYMS} ${LIBQUENTIER_LIBRARY_LOCATION} > ${LIBQUENTIER_FILE_NAME}.syms)

  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${symbols_compressor} ${LIBQUENTIER_FILE_NAME}.syms)
endif()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        BUNDLE DESTINATION .)

if(BREAKPAD_FOUND)
  install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.syms.compressed
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

  install(FILES ${PROJECT_BINARY_DIR}/${LIBQUENTIER_FILE_NAME}.syms.compressed
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

  if(WIN32)
    set(CRASH_HANDLER_BINARY "${PROJECT_BINARY_DIR}/crash_handler/quentier_crash_handler.exe")
  else()
    set(CRASH_HANDLER_BINARY "${PROJECT_BINARY_DIR}/crash_handler/quentier_crash_handler")
  endif()

  install(FILES ${CRASH_HANDLER_BINARY}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE)

  if(WIN32)
    set(STACKWALK_BINARY_NAME "quentier_minidump_stackwalk.exe")
  else()
    set(STACKWALK_BINARY_NAME "quentier_minidump_stackwalk")
  endif()

  install(FILES ${BREAKPAD_STACKWALKER}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE
    RENAME ${STACKWALK_BINARY_NAME})
endif()

if(NSIS_FOUND)
  include(QuentierDetectArchitecture)
  set(ARCH "")
  target_architecture(ARCH)
  message(STATUS "Target architecture: ${ARCH}")
  if(ARCH MATCHES "x86_64")
    set(QUENTIER_TARGET_PLATFORM x64)
  else()
    set(QUENTIER_TARGET_PLATFORM Win32)
  endif()

  if(USE_QT5)
    set(PRODUCT_EDITION "Qt ${Qt5Core_VERSION}")
  else()
    set(PRODUCT_EDITION "Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
  endif()

  if(MSVC)
    if(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^19[.]1(.+)")
      set(VC_VERSION "2017")
    elseif(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^19[.]0(.+)")
      set(VC_VERSION "2015")
    elseif(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^18[.](.+)")
      set(VC_VERSION "2013")
    elseif(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^17[.](.+)")
      set(VC_VERSION "2012")
    elseif(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^16[.](.+)")
      set(VC_VERSION "2010")
    endif()
    # No need to check for older versions since Quentier isn't buildable with them anyway

    set(PRODUCT_EDITION "${PRODUCT_EDITION} MSVC${VC_VERSION}")
    set(QUENTIER_LAUNCHABLE_FILE "quentier.exe")
  else()
    set(MINGW_VERSION "${GCC_VERSION}")
    string(REGEX REPLACE "\n$" "" MINGW_VERSION "${MINGW_VERSION}")
    set(PRODUCT_EDITION "${PRODUCT_EDITION} MinGW ${MINGW_VERSION}")
    set(QUENTIER_LAUNCHABLE_FILE "quentier.bat")
  endif()

  set(PRODUCT_EDITION "${PRODUCT_EDITION} ${QUENTIER_TARGET_PLATFORM}")

  if(NOT LIBQUENTIER_USE_QT_WEB_ENGINE AND MSVC)
    set(PRODUCT_EDITION "${PRODUCT_EDITION} webkit")
  endif()

  configure_file(installer/windows/wininstaller.nsi.in
                 ${PROJECT_BINARY_DIR}/wininstaller.nsi @ONLY)
endif()

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, ${FORMS})

COLLECT_SOURCES_FOR_CPPCHECK(${SOURCES})
COLLECT_INCLUDE_DIRS(.)
