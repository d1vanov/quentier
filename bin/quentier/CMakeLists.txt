cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

set(PROJECT_VERSION_MAJOR "${QUENTIER_VERSION_MAJOR}")
set(PROJECT_VERSION_MINOR "${QUENTIER_VERSION_MINOR}")
set(PROJECT_VERSION_PATCH "${QUENTIER_VERSION_PATCH}")
set(PROJECT_VERSION_COUNT 3)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

project(quentier VERSION ${PROJECT_VERSION})

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2019-2024")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

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
    src/MainWindow.h
    src/ParseCommandLine.h
    src/SetupStartupSettings.h)

set(SOURCES
    src/MainWindow.cpp
    src/main.cpp
    src/ParseCommandLine.cpp
    src/SetupStartupSettings.cpp)

set(FORMS
    src/MainWindow.ui)

if(BUILD_WITH_QT6)
  qt6_wrap_ui(FORMS_HEADERS ${FORMS})
else()
  qt_wrap_ui(FORMS_HEADERS ${FORMS})
endif()

if(WIN32)
  configure_file(src/quentier.rc.in
                 ${PROJECT_BINARY_DIR}/quentier.rc @ONLY)
  list(APPEND SOURCES ${PROJECT_BINARY_DIR}/quentier.rc)
endif()

if(BUILD_WITH_QT6)
  qt6_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})
else()
  qt5_add_resources(RESOURCES_RCC ${RESOURCES_FULL_PATH})
endif()

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${HEADERS}
               ${SOURCES}
               ${FORMS_HEADERS}
               ${RESOURCES_RCC})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

add_sanitizers(${PROJECT_NAME})

if(WIN32)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      wsock32
      ws2_32)

  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      -DBOOST_ALL_NO_LIB
      -DBOOST_ALL_DYN_LINK)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    quentier_account
    quentier_delegate
    quentier_dialog
    quentier_enex
    quentier_exception
    quentier_initialization
    quentier_model
    quentier_network
    quentier_preferences
    quentier_tray
    quentier_utility
    quentier_view
    quentier_widget
    Boost::boost)

if(INCLUDE_UPDATE_INFO)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      quentier_update)
endif()

if (BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
	PRIVATE
	  Qt6::Core
	  Qt6::Gui
	  Qt6::Widgets
	  Qt6::PrintSupport
	  Libquentier::libqt6quentier
	  QEverCloud::qt6qevercloud)
else()
  target_link_libraries(${PROJECT_NAME}
	PRIVATE
	  Qt5::Core
	  Qt5::Gui
	  Qt5::Widgets
	  Qt5::PrintSupport
	  Libquentier::libqt5quentier
	  QEverCloud::qt5qevercloud)
endif()

if(BREAKPAD_FOUND)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      ${BREAKPAD_LIBRARIES})

  target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${BREAKPAD_INCLUDE_DIRS})

  target_compile_definitions(${PROJECT_NAME}
	PRIVATE
	  -DBUILDING_WITH_BREAKPAD=1)

  # 1) Generate symbols for the executable
  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${BREAKPAD_DUMP_SYMS} $<TARGET_FILE:${PROJECT_NAME}> > ${PROJECT_NAME}.syms)

  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${symbols_compressor} ${PROJECT_NAME}.syms)

  # 2) Generate symbols for the core library unless it is specified on the command line
  if(LIBQUENTIER_BREAKPAD_SYMS)
    add_custom_command(TARGET ${PROJECT_NAME}
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy ${LIBQUENTIER_BREAKPAD_SYMS} ${PROJECT_BINARY_DIR})
  else()
    add_custom_command(TARGET ${PROJECT_NAME}
                      POST_BUILD
                      COMMAND ${BREAKPAD_DUMP_SYMS} ${LIBQUENTIER_BINARY_NAME} > ${LIBQUENTIER_FILE_NAME}.syms)
  endif()

  add_custom_command(TARGET ${PROJECT_NAME}
                     POST_BUILD
                     COMMAND ${symbols_compressor} ${LIBQUENTIER_FILE_NAME}.syms)

  if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /LTCG")
  endif()
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
    set(CRASH_HANDLER_BINARY "${PROJECT_BINARY_DIR}/../crash_handler/quentier_crash_handler.exe")
  else()
    set(CRASH_HANDLER_BINARY "${PROJECT_BINARY_DIR}/../crash_handler/quentier_crash_handler")
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

  set(PRODUCT_EDITION "Qt ${Qt5Core_VERSION}")

  if(MSVC)
    if(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^19[.]2(.+)")
      set(VC_VERSION "2019")
    elseif(${CMAKE_CXX_COMPILER_VERSION} MATCHES "^19[.]1(.+)")
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
  else()
    set(PRODUCT_EDITION "${PRODUCT_EDITION} ${CMAKE_CXX_COMPILER_ID}")
  endif()

  set(QUENTIER_LAUNCHABLE_FILE "quentier.exe")
  set(PRODUCT_EDITION "${PRODUCT_EDITION} ${QUENTIER_TARGET_PLATFORM}")

  if(NOT LIBQUENTIER_USE_QT_WEB_ENGINE AND MSVC)
    set(PRODUCT_EDITION "${PRODUCT_EDITION} webkit")
  endif()

  configure_file(src/installer/windows/wininstaller.nsi.in
                 ${PROJECT_BINARY_DIR}/wininstaller.nsi @ONLY)
endif()

if(CREATE_BUNDLE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
  include(QuentierFindLinuxdeployqt)
endif()

include(QuentierCreateBundle)
CreateQuentierBundle()

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)
