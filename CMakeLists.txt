cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/sanitizers ${CMAKE_MODULE_PATH})

include(QuentierCMakePolicies)
SET_POLICIES()

project(quentier VERSION 0.6.0)

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2016-2024")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(QUENTIER_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(QUENTIER_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(QUENTIER_VERSION_PATCH ${PROJECT_VERSION_PATCH})

option(BUILD_WITH_QT6 "Build with Qt6" OFF)
option(QUENTIER_BUILD_TEST_TOOLS "Build test tools" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")

include(QuentierCompilerSettings)
include(QuentierAdditionalCompilerWarnings)
include(QuentierFindDependencies)
include(QuentierUpdateTranslations)
include(QuentierDoxygen)
include(PrependPath)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

set(RESOURCES
    resource/icons/app/app_icons.qrc
    resource/icons/themes/tango/tango.qrc
    resource/icons/themes/oxygen/oxygen.qrc
    resource/icons/themes/breeze/breeze.qrc
    resource/icons/themes/breeze-dark/breeze-dark.qrc
    resource/icons/other/icons.qrc)

prepend_path(RESOURCES_FULL_PATH "${RESOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}")

set(THIRDPARTY_LIBS ${QT_LIBRARIES})
list(APPEND THIRDPARTY_LIBS QEverCloud::qt5qevercloud)
list(APPEND THIRDPARTY_LIBS Libquentier::libqt5quentier)
if(WIN32)
  list(APPEND THIRDPARTY_LIBS wsock32 ws2_32)
endif()

if(APPLE)
  set(APPLICATION_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon.icns)
  set_source_files_properties(${APPLICATION_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
else()
  set(APPLICATION_ICON "")
endif()

# install application bundle
if(WIN32 OR APPLE)
  set(DEFAULT_CREATE_BUNDLE ON)
  if(WIN32)
    set(DEFAULT_CREATE_INSTALLER ON)
  endif()
else()
  set(DEFAULT_CREATE_BUNDLE OFF)
endif()

set(CREATE_BUNDLE ${DEFAULT_CREATE_BUNDLE} CACHE BOOL "Create application standalone bundle/installation")

if(WIN32)
  set(CREATE_INSTALLER ${DEFAULT_CREATE_INSTALLER} CACHE BOOL "Create application installer")
endif()

include(QuentierFindNSIS)

# configure version info
if(QUENTIER_PACKAGED_AS_APP_IMAGE OR (("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux") AND CREATE_BUNDLE))
  message(STATUS "Quentier is being packaged as AppImage")
  set(QUENTIER_PACKAGED_AS_APP_IMAGE_DEFINE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 1")
else()
  set(QUENTIER_PACKAGED_AS_APP_IMAGE_DEFINE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 0")
endif()

get_filename_component(LIBQUENTIER_BINARY_NAME "${LIBQUENTIER_LIBRARY_LOCATION}" NAME)
get_filename_component(QEVERCLOUD_BINARY_NAME "${QEVERCLOUD_LIBRARY_LOCATION}" NAME)

set(QUENTIER_LIBQUENTIER_BINARY_NAME "#define QUENTIER_LIBQUENTIER_BINARY_NAME \"${LIBQUENTIER_BINARY_NAME}\"")
set(QUENTIER_QEVERCLOUD_BINARY_NAME "#define QUENTIER_QEVERCLOUD_BINARY_NAME \"${QEVERCLOUD_BINARY_NAME}\"")

if (NOT QUENTIER_BUILD_INFO)
  find_package(Git)
  if(GIT_FOUND)
    message(STATUS "Git found: ${GIT_EXECUTABLE}")

    # Get git branch
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE QUENTIER_GIT_BRANCH
      RESULT_VARIABLE QUENTIER_GIT_BRANCH_RETURN_CODE
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT "${QUENTIER_GIT_BRANCH_RETURN_CODE}" STREQUAL "0")
      message(AUTHOR_WARNING "Failed to determine the current git branch, return code ${QUENTIER_GIT_BRANCH_RETURN_CODE}")
      set(QUENTIER_GIT_BRANCH "unknown branch")
    else()
      if(${QUENTIER_GIT_BRANCH} STREQUAL "HEAD")
        # Can happen if running on detached HEAD, can happen in CI jobs; workaround: try to get the current branch from environment variables
        set(APPVEYOR_REPO_BRANCH "$ENV{APPVEYOR_REPO_BRANCH}")
        set(TRAVIS_BRANCH "$ENV{TRAVIS_BRANCH}")
        if(NOT "${APPVEYOR_REPO_BRANCH}" STREQUAL "")
          set(QUENTIER_GIT_BRANCH "${APPVEYOR_REPO_BRANCH}")
        elseif(NOT "${TRAVIS_BRANCH}" STREQUAL "")
          set(QUENTIER_GIT_BRANCH "${TRAVIS_BRANCH}")
        endif()
      endif()
      message(STATUS "Git branch: ${QUENTIER_GIT_BRANCH}")
    endif()

    # Get last commit short hash
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      OUTPUT_VARIABLE QUENTIER_GIT_REVISION
      RESULT_VARIABLE QUENTIER_GIT_REVISION_RETURN_CODE
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT "${QUENTIER_GIT_REVISION_RETURN_CODE}" STREQUAL "0")
      message(AUTHOR_WARNING "Failed to determine the current git revision")
      set(QUENTIER_GIT_REVISION "unknown")
    else()
      message(STATUS "Last commit short hash: ${QUENTIER_GIT_REVISION}")
    endif()

    # Check for uncommitted changes
    set(QUENTIER_GIT_REVISION_SUFFIX "")
    execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE QUENTIER_GIT_DIRTY_STATE)
    if(NOT "${QUENTIER_GIT_DIRTY_STATE}" STREQUAL "0")
      set(QUENTIER_GIT_REVISION_SUFFIX ", with uncommitted changes")
    endif()

    set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"${QUENTIER_GIT_BRANCH}, ${QUENTIER_GIT_REVISION}${QUENTIER_GIT_REVISION_SUFFIX}\"")
  else()
    set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"unknown\"")
  endif()
else()
  set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"${QUENTIER_BUILD_INFO}\"")
endif()

if(INCLUDE_UPDATE_INFO)
  if(NOT DEFAULT_UPDATE_CHANNEL)
    message(FATAL_ERROR "No default update channel is set")
  endif()

  if(NOT DEFAULT_UPDATE_PROVIDER)
    message(FATAL_ERROR "No default update provider is set")
  endif()

  if(NOT GIT_FOUND)
    message(FATAL_ERROR "Build from git repository is required to include update info")
  endif()

  if(QUENTIER_PACKAGED_AS_APP_IMAGE OR (("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux") AND CREATE_BUNDLE))
    set(USE_APP_IMAGE_UPDATER_BRIDGE TRUE)
    find_package(AppImageUpdaterBridge REQUIRED)
    get_property(APP_IMAGE_UPDATER_BRIDGE_LIBRARY_LOCATION TARGET AppImageUpdaterBridge PROPERTY LOCATION)
    get_property(APP_IMAGE_UPDATER_BRIDGE_INCLUDE_DIR TARGET AppImageUpdaterBridge PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${APP_IMAGE_UPDATER_BRIDGE_INCLUDE_DIR})
    list(APPEND THIRDPARTY_LIBS AppImageUpdaterBridge)
  endif()

  add_definitions(-DWITH_UPDATE_MANAGER=1)
endif()

set(QUENTIER_TRANSLATABLE_SOURCES "")
set(QUENTIER_TRANSLATABLE_FORMS "")

set_property(GLOBAL PROPERTY QUENTIER_TRANSLATABLE_SOURCES ${QUENTIER_TRANSLATABLE_SOURCES})
set_property(GLOBAL PROPERTY QUENTIER_TRANSLATABLE_FORMS ${QUENTIER_TRANSLATABLE_FORMS})

include(QuentierCollectSourcesForTranslation)

set(QUENTIER_CPPCHECKABLE_SOURCES "")
set(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "")

set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_SOURCES ${QUENTIER_CPPCHECKABLE_SOURCES})
set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS ${QUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

set(QUENTIER_ALL_HEADERS "")
set(QUENTIER_ALL_SOURCES "")
set(QUENTIER_ALL_INCLUDE_DIRS "")

set_property(GLOBAL PROPERTY QUENTIER_ALL_HEADERS ${QUENTIER_ALL_HEADERS})
set_property(GLOBAL PROPERTY QUENTIER_ALL_SOURCES ${QUENTIER_ALL_SOURCES})
set_property(GLOBAL PROPERTY QUENTIER_ALL_INCLUDE_DIRS ${QUENTIER_ALL_INCLUDE_DIRS})

# installation settings
if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") AND CREATE_BUNDLE)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/usr")
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
  set(CMAKE_INSTALL_TRANSLATIONSDIR "${CMAKE_INSTALL_PREFIX}/share/quentier/translations")
else()
  if(NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
  if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
  endif()
  if(NOT CMAKE_INSTALL_TRANSLATIONSDIR)
    set(CMAKE_INSTALL_TRANSLATIONSDIR "${CMAKE_INSTALL_PREFIX}/share/quentier/translations")
  endif()
endif()

get_filename_component(LIBQUENTIER_FILE_NAME "${LIBQUENTIER_LIBRARIES}" NAME)

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")

if(NOT WIN32 AND NOT APPLE)
  # install .desktop file
  install(FILES ${CMAKE_SOURCE_DIR}/resource/Quentier.desktop
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
          RENAME org.quentier.Quentier.desktop)
  # install application icons
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_16.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_32.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_48.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_64.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_128.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_256.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_512.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/512x512/apps"
          RENAME quentier.png)
  install(FILES ${CMAKE_SOURCE_DIR}/resource/icons/app/quentier_icon_1024.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/1024x1024/apps"
          RENAME quentier.png)
  # install metainfo file
  install(FILES ${CMAKE_SOURCE_DIR}/resource/quentier-metainfo.appdata.xml
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo"
          RENAME org.quentier.Quentier.appdata.xml)
endif()

set(QUENTIER_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(lib)
add_subdirectory(bin)

get_property(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS)
list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${LIBQUENTIER_INCLUDE_DIRS}")
set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS ${QUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

get_property(QUENTIER_ALL_INCLUDE_DIRS GLOBAL PROPERTY QUENTIER_ALL_INCLUDE_DIRS)
list(APPEND QUENTIER_ALL_INCLUDE_DIRS "${LIBQUENTIER_INCLUDE_DIRS}")
set_property(GLOBAL PROPERTY QUENTIER_ALL_INCLUDE_DIRS ${QUENTIER_ALL_INCLUDE_DIRS})

get_property(QUENTIER_ALL_HEADERS GLOBAL PROPERTY QUENTIER_ALL_HEADERS)
get_property(QUENTIER_ALL_SOURCES GLOBAL PROPERTY QUENTIER_ALL_SOURCES)

include(SetupClangFormat)
include(SetupClangTidy)

set(QUENTIER_TR_FILES
    translations/quentier_ru_RU.ts)

set(QUENTIER_QM_FILES "")
get_property(QUENTIER_TRANSLATABLE_SOURCES GLOBAL PROPERTY QUENTIER_TRANSLATABLE_SOURCES)
get_property(QUENTIER_TRANSLATABLE_FORMS GLOBAL PROPERTY QUENTIER_TRANSLATABLE_FORMS)
update_translation("${QUENTIER_TRANSLATABLE_SOURCES}" "${QUENTIER_TRANSLATABLE_FORMS}" "${QUENTIER_TR_FILES}")

# install translations
foreach(QM_FILE ${QUENTIER_QM_FILES})
  install(CODE "
          set(QM_FILE \"${QM_FILE}\")
          if(EXISTS \"${QM_FILE}\")
            get_filename_component(QM_FILE_BASE_NAME \"${QM_FILE}\" NAME)
            if(APPLE)
              set(DESTDIR \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/quentier.app/Contents/Resources/translations/quentier\")
            else()
              set(DESTDIR \"\$ENV{DESTDIR}${CMAKE_INSTALL_TRANSLATIONSDIR}\")
            endif()
            message(STATUS \"Installing: \${DESTDIR}/\${QM_FILE_BASE_NAME}\")
            file(COPY \"${QM_FILE}\" DESTINATION \"\${DESTDIR}\")
          endif()
          " COMPONENT Runtime)
endforeach()

if(APPLE)
  # install libquentier's translations into the app bundle
  install(CODE "
          set(LIBQUENTIER_TRANSLATIONS_SEARCH_PATH \"${LIBQUENTIER_INCLUDE_DIRS}/../share/libquentier/translations\")
          file(GLOB LIBQUENTIER_TRANSLATION_FILES \"\${LIBQUENTIER_TRANSLATIONS_SEARCH_PATH}/libquentier_*.qm\")
          foreach(LIBQUENTIER_TRANSLATION_FILE \${LIBQUENTIER_TRANSLATION_FILES})
            get_filename_component(LIBQUENTIER_TRANSLATION_FILE_BASE_NAME \"\${LIBQUENTIER_TRANSLATION_FILE}\" NAME)
            message(STATUS \"Installing: \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/quentier.app/Contents/Resources/translations/libquentier/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
            file(COPY \"\${LIBQUENTIER_TRANSLATION_FILE}\" DESTINATION \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/quentier.app/Contents/Resources/translations/libquentier/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
          endforeach()
          " COMPONENT Runtime)
elseif(WIN32)
  # install libquentier's translations into the app's installation prefix
  install(CODE "
          set(LIBQUENTIER_TRANSLATIONS_SEARCH_PATH \"${LIBQUENTIER_INCLUDE_DIRS}/../share/libquentier/translations\")
          file(GLOB LIBQUENTIER_TRANSLATION_FILES \"\${LIBQUENTIER_TRANSLATIONS_SEARCH_PATH}/libquentier_*.qm\")
          foreach(LIBQUENTIER_TRANSLATION_FILE \${LIBQUENTIER_TRANSLATION_FILES})
            get_filename_component(LIBQUENTIER_TRANSLATION_FILE_BASE_NAME \"\${LIBQUENTIER_TRANSLATION_FILE}\" NAME)
            message(STATUS \"Installing: \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/libquentier/translations/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
            file(COPY \"\${LIBQUENTIER_TRANSLATION_FILE}\" DESTINATION \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/libquentier/translations/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
          endforeach()
          " COMPONENT Runtime)
endif()
