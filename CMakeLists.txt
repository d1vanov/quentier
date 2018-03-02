cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(QuentierCMakePolicies)
SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(quentier VERSION 0.4.0)
else()
  project(quentier)
  set(PROJECT_VERSION_MAJOR "0")
  set(PROJECT_VERSION_MINOR "4")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2017")
set(PROJECT_DOMAIN_FIRST "quentier")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

set(QUENTIER_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(QUENTIER_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(QUENTIER_VERSION_PATCH ${PROJECT_VERSION_PATCH})

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

add_custom_target(check COMMAND "${CMAKE_CTEST_COMMAND}" "--verbose")

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

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/MainWindowSideBorderOption.h
    src/MainWindowSideBordersController.h
    src/AsyncFileWriter.h
    src/ActionsInfo.h
    src/DefaultSettings.h
    src/SystemTrayIconManager.h
    src/AccountManager.h
    src/BasicXMLSyntaxHighlighter.h
    src/EditNoteDialogsManager.h
    src/EnexImporter.h
    src/NoteEditorTabsAndWindowsCoordinator.h
    src/NoteFiltersManager.h
    src/EnexExporter.h
    src/NetworkProxySettingsHelpers.h
    src/SettingsNames.h
    src/color-picker-tool-button/ColorPickerActionWidget.h
    src/color-picker-tool-button/ColorPickerToolButton.h
    src/dialogs/AddAccountDialog.h
    src/dialogs/AddOrEditNotebookDialog.h
    src/dialogs/AddOrEditTagDialog.h
    src/dialogs/AddOrEditSavedSearchDialog.h
    src/dialogs/EditNoteDialog.h
    src/dialogs/EnexExportDialog.h
    src/dialogs/EnexImportDialog.h
    src/dialogs/ManageAccountsDialog.h
    src/dialogs/PreferencesDialog.h
    src/dialogs/WelcomeToQuentierDialog.h
    src/dialogs/shortcut_settings/ShortcutButton.h
    src/dialogs/shortcut_settings/ShortcutSettingsWidget.h
    src/initialization/CommandLineParser.h
    src/initialization/DefaultAccountFirstNotebookAndNoteCreator.h
    src/initialization/Initialize.h
    src/initialization/LoadDependencies.h
    src/initialization/ParseStartupAccount.h
    src/initialization/SetupApplicationIcon.h
    src/initialization/SetupTranslations.h
    src/initialization/SetupStartAtLogin.h
    src/models/ColumnChangeRerouter.h
    src/models/ItemModel.h
    src/models/NewItemNameGenerator.hpp
    src/models/SavedSearchModel.h
    src/models/SavedSearchModelItem.h
    src/models/SavedSearchCache.h
    src/models/TagModel.h
    src/models/TagModelItem.h
    src/models/TagItem.h
    src/models/TagLinkedNotebookRootItem.h
    src/models/TagCache.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
    src/models/NotebookLinkedNotebookRootItem.h
    src/models/NotebookCache.h
    src/models/NoteModelItem.h
    src/models/NoteFilterModel.h
    src/models/NoteModel.h
    src/models/NoteCache.h
    src/models/FavoritesModel.h
    src/models/FavoritesModelItem.h
    src/models/LogViewerModel.h
    src/models/LogViewerModelFileReaderAsync.h
    src/models/LogViewerFilterModel.h
    src/delegates/AbstractStyledItemDelegate.h
    src/delegates/LimitedFontsDelegate.h
    src/delegates/NoteItemDelegate.h
    src/delegates/NotebookItemDelegate.h
    src/delegates/DeletedNoteItemDelegate.h
    src/delegates/DirtyColumnDelegate.h
    src/delegates/FavoriteItemDelegate.h
    src/delegates/FromLinkedNotebookColumnDelegate.h
    src/delegates/LogViewerDelegate.h
    src/delegates/SynchronizableColumnDelegate.h
    src/delegates/TagItemDelegate.h
    src/insert-table-tool-button/InsertTableToolButton.h
    src/insert-table-tool-button/TableSettingsDialog.h
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.h
    src/insert-table-tool-button/TableSizeSelectorActionWidget.h
    src/insert-table-tool-button/TableSizeSelector.h
    src/utility/ColorCodeValidator.h
    src/utility/StartAtLogin.h
    src/views/ItemView.h
    src/views/DeletedNoteItemView.h
    src/views/FavoriteItemView.h
    src/views/NotebookItemView.h
    src/views/NoteListView.h
    src/views/TagItemView.h
    src/views/SavedSearchItemView.h
    src/widgets/FindAndReplaceWidget.h
    src/widgets/FilterByNotebookWidget.h
    src/widgets/FilterBySavedSearchWidget.h
    src/widgets/FilterByTagWidget.h
    src/widgets/FlowLayout.h
    src/widgets/ListItemWidget.h
    src/widgets/LogViewerWidget.h
    src/widgets/NewListItemLineEdit.h
    src/widgets/NotebookModelItemInfoWidget.h
    src/widgets/SavedSearchModelItemInfoWidget.h
    src/widgets/NoteEditorWidget.h
    src/widgets/NoteTagsWidget.h
    src/widgets/TagModelItemInfoWidget.h
    src/widgets/AbstractFilterByModelItemWidget.h
    src/widgets/AboutQuentierWidget.h
    src/widgets/TabWidget.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/MainWindowSideBordersController.cpp
    src/ActionsInfo.cpp
    src/AsyncFileWriter.cpp
    src/AccountManager.cpp
    src/SystemTrayIconManager.cpp
    src/ActionShortcuts.inl
    src/BasicXMLSyntaxHighlighter.cpp
    src/EditNoteDialogsManager.cpp
    src/EnexImporter.cpp
    src/NoteEditorTabsAndWindowsCoordinator.cpp
    src/NoteFiltersManager.cpp
    src/EnexExporter.cpp
    src/NetworkProxySettingsHelpers.cpp
    src/color-picker-tool-button/ColorPickerActionWidget.cpp
    src/color-picker-tool-button/ColorPickerToolButton.cpp
    src/dialogs/AddAccountDialog.cpp
    src/dialogs/AddOrEditNotebookDialog.cpp
    src/dialogs/AddOrEditTagDialog.cpp
    src/dialogs/AddOrEditSavedSearchDialog.cpp
    src/dialogs/EditNoteDialog.cpp
    src/dialogs/EnexExportDialog.cpp
    src/dialogs/EnexImportDialog.cpp
    src/dialogs/ManageAccountsDialog.cpp
    src/dialogs/PreferencesDialog.cpp
    src/dialogs/WelcomeToQuentierDialog.cpp
    src/dialogs/shortcut_settings/ShortcutButton.cpp
    src/dialogs/shortcut_settings/ShortcutSettingsWidget.cpp
    src/initialization/CommandLineParser.cpp
    src/initialization/DefaultAccountFirstNotebookAndNoteCreator.cpp
    src/initialization/Initialize.cpp
    src/initialization/LoadDependencies.cpp
    src/initialization/ParseStartupAccount.cpp
    src/initialization/SetupApplicationIcon.cpp
    src/initialization/SetupTranslations.cpp
    src/initialization/SetupStartAtLogin.cpp
    src/insert-table-tool-button/InsertTableToolButton.cpp
    src/insert-table-tool-button/TableSettingsDialog.cpp
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelector.cpp
    src/models/ColumnChangeRerouter.cpp
    src/models/ItemModel.cpp
    src/models/SavedSearchModel.cpp
    src/models/SavedSearchModelItem.cpp
    src/models/TagModel.cpp
    src/models/TagModelItem.cpp
    src/models/TagItem.cpp
    src/models/TagLinkedNotebookRootItem.cpp
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
    src/models/NotebookLinkedNotebookRootItem.cpp
    src/models/NoteModelItem.cpp
    src/models/NoteFilterModel.cpp
    src/models/NoteModel.cpp
    src/models/FavoritesModel.cpp
    src/models/FavoritesModelItem.cpp
    src/models/LogViewerModel.cpp
    src/models/LogViewerModelFileReaderAsync.cpp
    src/models/LogViewerFilterModel.cpp
    src/delegates/AbstractStyledItemDelegate.cpp
    src/delegates/LimitedFontsDelegate.cpp
    src/delegates/NoteItemDelegate.cpp
    src/delegates/NotebookItemDelegate.cpp
    src/delegates/DeletedNoteItemDelegate.cpp
    src/delegates/DirtyColumnDelegate.cpp
    src/delegates/FavoriteItemDelegate.cpp
    src/delegates/FromLinkedNotebookColumnDelegate.cpp
    src/delegates/LogViewerDelegate.cpp
    src/delegates/SynchronizableColumnDelegate.cpp
    src/delegates/TagItemDelegate.cpp
    src/utility/ColorCodeValidator.cpp
    src/utility/StartAtLogin.cpp
    src/views/ItemView.cpp
    src/views/DeletedNoteItemView.cpp
    src/views/FavoriteItemView.cpp
    src/views/NotebookItemView.cpp
    src/views/NoteListView.cpp
    src/views/TagItemView.cpp
    src/views/SavedSearchItemView.cpp
    src/widgets/FindAndReplaceWidget.cpp
    src/widgets/FilterByNotebookWidget.cpp
    src/widgets/FilterBySavedSearchWidget.cpp
    src/widgets/FilterByTagWidget.cpp
    src/widgets/FlowLayout.cpp
    src/widgets/ListItemWidget.cpp
    src/widgets/LogViewerWidget.cpp
    src/widgets/NewListItemLineEdit.cpp
    src/widgets/NotebookModelItemInfoWidget.cpp
    src/widgets/SavedSearchModelItemInfoWidget.cpp
    src/widgets/NoteEditorWidget.cpp
    src/widgets/NoteTagsWidget.cpp
    src/widgets/TagModelItemInfoWidget.cpp
    src/widgets/AbstractFilterByModelItemWidget.cpp
    src/widgets/AboutQuentierWidget.cpp
    src/widgets/TabWidget.cpp
    src/main.cpp)

if(WIN32)
  configure_file(src/quentier.rc.in
                 ${PROJECT_BINARY_DIR}/quentier.rc @ONLY)
  list(APPEND ${PROJECT_NAME}_SOURCES ${PROJECT_BINARY_DIR}/quentier.rc)
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/windows/StartAtLogin.cpp)
elseif(APPLE)
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/darwin/StartAtLogin.cpp)
else()
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/linux/StartAtLogin.cpp)
endif()

set(FORMS
    src/MainWindow.ui
    src/dialogs/AddAccountDialog.ui
    src/dialogs/AddOrEditNotebookDialog.ui
    src/dialogs/AddOrEditTagDialog.ui
    src/dialogs/AddOrEditSavedSearchDialog.ui
    src/dialogs/EditNoteDialog.ui
    src/dialogs/EnexExportDialog.ui
    src/dialogs/EnexImportDialog.ui
    src/dialogs/ManageAccountsDialog.ui
    src/dialogs/PreferencesDialog.ui
    src/dialogs/WelcomeToQuentierDialog.ui
    src/dialogs/shortcut_settings/ShortcutSettingsWidget.ui
    src/insert-table-tool-button/TableSettingsDialog.ui
    src/widgets/AboutQuentierWidget.ui
    src/widgets/FindAndReplaceWidget.ui
    src/widgets/ListItemWidget.ui
    src/widgets/LogViewerWidget.ui
    src/widgets/NewListItemLineEdit.ui
    src/widgets/NotebookModelItemInfoWidget.ui
    src/widgets/NoteEditorWidget.ui
    src/widgets/SavedSearchModelItemInfoWidget.ui
    src/widgets/TagModelItemInfoWidget.ui)

qt_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})

set(RESOURCES
    resource/icons/app/app_icons.qrc
    resource/icons/themes/tango/tango.qrc
    resource/icons/themes/oxygen/oxygen.qrc
    resource/icons/other/icons.qrc)

qt_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

set(${PROJECT_NAME}_TR_SOURCES
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES})

set(${PROJECT_NAME}_TR_FILES
    translations/quentier_ru_RU.ts)

set(${PROJECT_NAME}_QM_FILES "")
update_translation("${${PROJECT_NAME}_TR_SOURCES}" "${FORMS}" "${${PROJECT_NAME}_TR_FILES}")

set(THIRDPARTY_LIBS ${QT_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${QEVERCLOUD_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${LIBQUENTIER_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${Boost_LIBRARIES})
if(WIN32)
  list(APPEND THIRDPARTY_LIBS wsock32 ws2_32)
endif()

if(BREAKPAD_FOUND)
  list(APPEND THIRDPARTY_LIBS ${BREAKPAD_LIBRARIES})
  list(APPEND ${PROJECT_NAME}_HEADERS src/initialization/breakpad/BreakpadIntegration.h)
  list(APPEND ${PROJECT_NAME}_SOURCES src/initialization/breakpad/BreakpadIntegration.cpp)
  if(WIN32)
    list(APPEND ${PROJECT_NAME}_SOURCES src/initialization/breakpad/BreakpadIntegrationWindows.cpp)
  else()
    list(APPEND ${PROJECT_NAME}_SOURCES src/initialization/breakpad/BreakpadIntegrationLinux.cpp)
  endif()
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
    set(QUENTIER_GIT_REVISION "unknown revision")
  else()
    message(STATUS "Last commit short hash: ${QUENTIER_GIT_REVISION}")
  endif()

  # Check for uncommitted changes
  execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
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

set(QUENTIER_VERSION_INFO_HEADER ${PROJECT_BINARY_DIR}/VersionInfo.h)
configure_file(src/VersionInfo.h.in
               ${QUENTIER_VERSION_INFO_HEADER} @ONLY)
list(APPEND ${PROJECT_NAME}_HEADERS ${QUENTIER_VERSION_INFO_HEADER})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
               ${APPLICATION_ICON}
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

get_filename_component(LIBQUENTIER_FILE_NAME "${LIBQUENTIER_LIBRARIES}" NAME)

if(BREAKPAD_FOUND)
  add_subdirectory(crash_handler)
  add_subdirectory(symbols_compressor)

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

target_link_libraries(${PROJECT_NAME} ${THIRDPARTY_LIBS})

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
if(USE_QT5)
  add_definitions("-DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT")
endif()

# Set up the model tests
set(MODEL_TEST_HEADERS
    src/tests/model_test/modeltest.h
    src/tests/model_test/Macros.h
    src/tests/model_test/SavedSearchModelTestHelper.h
    src/tests/model_test/TagModelTestHelper.h
    src/tests/model_test/NotebookModelTestHelper.h
    src/tests/model_test/NoteModelTestHelper.h
    src/tests/model_test/FavoritesModelTestHelper.h
    src/tests/model_test/ModelTester.h
    src/models/ItemModel.h
    src/models/SavedSearchModel.h
    src/models/SavedSearchModelItem.h
    src/models/SavedSearchCache.h
    src/models/TagModel.h
    src/models/TagModelItem.h
    src/models/TagItem.h
    sec/models/TagLinkedNotebookRootItem.h
    src/models/TagCache.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
    src/models/NotebookLinkedNotebookRootItem.h
    src/models/NotebookCache.h
    src/models/NoteModelItem.h
    src/models/NoteFilterModel.h
    src/models/NoteModel.h
    src/models/NoteCache.h
    src/models/FavoritesModel.h
    src/models/FavoritesModelItem.h)

set(MODEL_TEST_SOURCES
    src/tests/model_test/modeltest.cpp
    src/tests/model_test/SavedSearchModelTestHelper.cpp
    src/tests/model_test/TagModelTestHelper.cpp
    src/tests/model_test/NotebookModelTestHelper.cpp
    src/tests/model_test/NoteModelTestHelper.cpp
    src/tests/model_test/FavoritesModelTestHelper.cpp
    src/tests/model_test/ModelTester.cpp
    src/models/ItemModel.cpp
    src/models/SavedSearchModel.cpp
    src/models/SavedSearchModelItem.cpp
    src/models/TagModel.cpp
    src/models/TagModelItem.cpp
    src/models/TagItem.cpp
    src/models/TagLinkedNotebookRootItem.cpp
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
    src/models/NotebookLinkedNotebookRootItem.cpp
    src/models/NoteModelItem.cpp
    src/models/NoteFilterModel.cpp
    src/models/NoteModel.cpp
    src/models/FavoritesModel.cpp
    src/models/FavoritesModelItem.cpp)

add_executable(${PROJECT_NAME}_model_test ${MODEL_TEST_SOURCES} ${MODEL_TEST_SOURCES})
add_test(${PROJECT_NAME}_model_test ${PROJECT_NAME}_model_test)
target_link_libraries(${PROJECT_NAME}_model_test ${THIRDPARTY_LIBS})

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/models")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/widgets")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/tests")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/color-picker-tool-button")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/insert-table-tool-button")

# modify the sources list for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# installation settings
if((${CMAKE_SYSTEM_NAME} MATCHES "Linux") AND CREATE_BUNDLE AND USE_QT5)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/usr")
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
  set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
  set(CMAKE_INSTALL_TRANSLATIONSDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations")
else()
  if(NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
  if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
  endif()
  if(NOT CMAKE_INSTALL_TRANSLATIONSDIR)
    set(CMAKE_INSTALL_TRANSLATIONSDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/translations")
  endif()
endif()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        BUNDLE DESTINATION .)

# install translations
foreach(QM_FILE ${${PROJECT_NAME}_QM_FILES})
  install(CODE "
          set(QM_FILE \"${QM_FILE}\")
          if(EXISTS \"${QM_FILE}\")
            get_filename_component(QM_FILE_BASE_NAME \"${QM_FILE}\" NAME)
            if(APPLE)
              set(DESTDIR \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/translations/quentier\")
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
            message(STATUS \"Installing: \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/translations/libquentier/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
            file(COPY \"\${LIBQUENTIER_TRANSLATION_FILE}\" DESTINATION \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/translations/libquentier/\${LIBQUENTIER_TRANSLATION_FILE_BASE_NAME}\")
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

if(NOT WIN32 AND NOT APPLE)
  # install .desktop file
  install(FILES ${PROJECT_SOURCE_DIR}/resource/Quentier.desktop
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications"
          RENAME org.quentier.Quentier.desktop)
  # install application icons
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_16.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_32.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_48.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_64.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_128.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_256.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_512.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/512x512/apps"
          RENAME quentier.png)
  install(FILES ${PROJECT_SOURCE_DIR}/resource/icons/app/quentier_icon_1024.png
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/1024x1024/apps"
          RENAME quentier.png)
  # install metainfo file
  install(FILES ${PROJECT_SOURCE_DIR}/resource/quentier-metainfo.appdata.xml
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo"
          RENAME org.quentier.Quentier.appdata.xml)
endif()

if(CREATE_BUNDLE AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
  include(QuentierFindLinuxdeployqt)
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

  configure_file(src/installer/windows/wininstaller.nsi.in
                 ${PROJECT_BINARY_DIR}/wininstaller.nsi @ONLY)
endif()

include(QuentierCreateBundle)
CreateQuentierBundle()

# list headers/sources and include dirs for cppcheck
set(QUENTIER_CPPCHECKABLE_SOURCES "${${PROJECT_NAME}_SOURCES}")

if(BREAKPAD_FOUND)
  # workaround: cppcheck fails to parse some macros used within Breakpad integration sources so need to exclude them
  list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/src/initialization/breakpad/BreakpadIntegration.cpp)
  if(WIN32)
    list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/src/initialization/breakpad/BreakpadIntegrationWindows.cpp)
  else()
    list(REMOVE_ITEM QUENTIER_CPPCHECKABLE_SOURCES ${PROJECT_SOURCE_DIR}/src/initialization/breakpad/BreakpadIntegrationLinux.cpp)
  endif()
endif()

set(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${${PROJECT_NAME}_INCLUDE_DIRS}")
list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${LIBQUENTIER_INCLUDE_DIRS}")

include(QuentierSetupCppCheck)
