cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/sanitizers ${CMAKE_MODULE_PATH})

include(QuentierCMakePolicies)
SET_POLICIES()

include(QuentierCompilerSettings)
include(QuentierAdditionalCompilerWarnings)
include(QuentierLinkerSettings)
include(QuentierSetupQt)
include(QuentierUpdateTranslations)
include(QuentierFindQEverCloud)
include(QuentierFindLibquentier)
include(QuentierFindBoost)
include(QuentierDoxygen)

if(NOT APPLE)
  include(QuentierFindBreakpad)
endif()

include(PrependPath)

find_package(Sanitizers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(RESOURCES
    resource/icons/app/app_icons.qrc
    resource/icons/themes/tango/tango.qrc
    resource/icons/themes/oxygen/oxygen.qrc
    resource/icons/other/icons.qrc)

prepend_path(RESOURCES_FULL_PATH "${RESOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}")

set(THIRDPARTY_LIBS ${QT_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${QEVERCLOUD_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${LIBQUENTIER_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${Boost_LIBRARIES})
if(WIN32)
  list(APPEND THIRDPARTY_LIBS wsock32 ws2_32)
endif()

if(BREAKPAD_FOUND)
  list(APPEND THIRDPARTY_LIBS ${BREAKPAD_LIBRARIES})
  add_definitions(-DBUILDING_WITH_BREAKPAD=1)
endif()

if(WIN32)
  # Disable boost auto-linking which gets in the way of CMake's dependencies resolution
  add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK)
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
if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") AND CREATE_BUNDLE AND USE_QT5)
  set(QUENTIER_PACKAGED_AS_APP_IMAGE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 1")
  set(LIBQUENTIER_BINARY_NAME "libqt5quentier.so.0")
else()
  set(QUENTIER_PACKAGED_AS_APP_IMAGE "#define QUENTIER_PACKAGED_AS_APP_IMAGE 0")
  get_filename_component(LIBQUENTIER_BINARY_NAME "${LIBQUENTIER_LIBRARY_LOCATION}" NAME)
endif()

set(QUENTIER_LIBQUENTIER_BINARY_NAME "#define QUENTIER_LIBQUENTIER_BINARY_NAME \"${LIBQUENTIER_BINARY_NAME}\"")

find_package(Git)
if(GIT_FOUND)
  message(STATUS "Git found: ${GIT_EXECUTABLE}")

  # Get git branch
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
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
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  OUTPUT_VARIABLE QUENTIER_GIT_REVISION
                  RESULT_VARIABLE QUENTIER_GIT_REVISION_RETURN_CODE
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT "${QUENTIER_GIT_REVISION_RETURN_CODE}" STREQUAL "0")
    message(AUTHOR_WARNING "Failed to determine the current git revision")
    set(QUENTIER_GIT_REVISION "unknown revision")
  else()
    message(STATUS "Last commit short hash: ${QUENTIER_GIT_REVISION}")
  endif()

  # Check for uncommitted changes
  execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  RESULT_VARIABLE QUENTIER_GIT_DIRTY_STATE)
  if(NOT "${QUENTIER_GIT_DIRTY_STATE}" STREQUAL "0")
    set(QUENTIER_GIT_REVISION "${QUENTIER_GIT_REVISION}, with uncommitted changes")
  endif()

  set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"${QUENTIER_GIT_BRANCH}, ${QUENTIER_GIT_REVISION}\"")
else()
  set(QUENTIER_BUILD_INFO "#define QUENTIER_BUILD_INFO \"unknown\"")
endif()

set(QUENTIER_MAJOR_VERSION "#define QUENTIER_MAJOR_VERSION \"${QUENTIER_VERSION_MAJOR}\"")
set(QUENTIER_MINOR_VERSION "#define QUENTIER_MINOR_VERSION \"${QUENTIER_VERSION_MINOR}\"")
set(QUENTIER_PATCH_VERSION "#define QUENTIER_PATCH_VERSION \"${QUENTIER_VERSION_PATCH}\"")

set(QUENTIER_TRANSLATABLE_SOURCES "")
set(QUENTIER_TRANSLATABLE_FORMS "")
include(QuentierCollectSourcesForTranslation)

set(QUENTIER_CPPCHECKABLE_SOURCES "")
set(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "")

set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_SOURCES ${QUENTIER_CPPCHECKABLE_SOURCES})
set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS ${QUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

include(QuentierCollectSourcesForCppcheck)

# installation settings
if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") AND CREATE_BUNDLE AND USE_QT5)
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
if(USE_QT5)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()

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

if(NOT WIN32 AND NOT APPLE)
  # install .desktop file
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/Quentier.desktop
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
          RENAME org.quentier.Quentier.desktop)
  # install application icons
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_16.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_32.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_48.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_64.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_128.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_256.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_512.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/512x512/apps"
          RENAME quentier.png)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/icons/app/quentier_icon_1024.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/1024x1024/apps"
          RENAME quentier.png)
  # install metainfo file
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/quentier-metainfo.appdata.xml
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo"
          RENAME org.quentier.Quentier.appdata.xml)
endif()

add_subdirectory(lib)
add_subdirectory(bin)

if(CREATE_BUNDLE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
  include(QuentierFindLinuxdeployqt)
endif()

include(QuentierCreateBundle)
CreateQuentierBundle()

get_property(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS)
list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${LIBQUENTIER_INCLUDE_DIRS}")
set_property(GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS ${QUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

include(QuentierSetupCppCheck)

set(QUENTIER_TR_FILES
    translations/quentier_ru_RU.ts)

set(QUENTIER_QM_FILES "")
update_translation("${QUENTIER_TRANSLATABLE_SOURCES}" "${QUENTIER_TRANSLATABLE_FORMS}" "${QUENTIER_TR_FILES}")
