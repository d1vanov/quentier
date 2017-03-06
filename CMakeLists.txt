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

include(QuentierCompilerSettings)
include(QuentierAdditionalCompilerWarnings)
include(QuentierLinkerSettings)
include(QuentierSetupQt)
include(QuentierFindQEverCloud)
include(QuentierFindLibquentier)
include(QuentierFindBoost)
include(QuentierDoxygen)
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
  # set(MACOSX_BUNDLE_ICON_FILE "quentier_icon.icns")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_DOMAIN_SECOND}.${PROJECT_DOMAIN_FIRST}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")

  set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources")
  # set(MACOSX_BUNDLE_ICON "${ICONS_DIR}/${MACOSX_BUNDLE_ICON_FILE}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})
  # execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BUNDLE_ICON} ${MACOSX_BUNDLE_RESOURCES})
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/CommandLineParser.h
    src/LoadDependencies.h
    src/SetupApplicationIcon.h
    src/SystemTrayIconManager.h
    src/AccountManager.h
    src/BasicXMLSyntaxHighlighter.h
    src/EditNoteDialogsManager.h
    src/NoteEditorTabsAndWindowsCoordinator.h
    src/NoteFiltersManager.h
    src/SettingsNames.h
    src/color-picker-tool-button/ColorPickerActionWidget.h
    src/color-picker-tool-button/ColorPickerToolButton.h
    src/dialogs/AddAccountDialog.h
    src/dialogs/AddOrEditNotebookDialog.h
    src/dialogs/AddOrEditTagDialog.h
    src/dialogs/AddOrEditSavedSearchDialog.h
    src/dialogs/EditNoteDialog.h
    src/dialogs/ManageAccountsDialog.h
    src/models/ColumnChangeRerouter.h
    src/models/ItemModel.h
    src/models/NewItemNameGenerator.hpp
    src/models/SavedSearchModel.h
    src/models/SavedSearchModelItem.h
    src/models/SavedSearchCache.h
    src/models/TagModel.h
    src/models/TagModelItem.h
    src/models/TagCache.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
    src/models/NotebookCache.h
    src/models/NoteModelItem.h
    src/models/NoteFilterModel.h
    src/models/NoteModel.h
    src/models/NoteCache.h
    src/models/FavoritesModel.h
    src/models/FavoritesModelItem.h
    src/delegates/AbstractStyledItemDelegate.h
    src/delegates/NoteItemDelegate.h
    src/delegates/NotebookItemDelegate.h
    src/delegates/DeletedNoteItemDelegate.h
    src/delegates/DirtyColumnDelegate.h
    src/delegates/FavoriteItemDelegate.h
    src/delegates/FromLinkedNotebookColumnDelegate.h
    src/delegates/SynchronizableColumnDelegate.h
    src/delegates/TagItemDelegate.h
    src/insert-table-tool-button/InsertTableToolButton.h
    src/insert-table-tool-button/TableSettingsDialog.h
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.h
    src/insert-table-tool-button/TableSizeSelectorActionWidget.h
    src/insert-table-tool-button/TableSizeSelector.h
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
    src/widgets/NewListItemLineEdit.h
    src/widgets/NotebookModelItemInfoWidget.h
    src/widgets/SavedSearchModelItemInfoWidget.h
    src/widgets/NoteEditorWidget.h
    src/widgets/NoteTagsWidget.h
    src/widgets/TagModelItemInfoWidget.h
    src/widgets/AbstractFilterByModelItemWidget.h
    src/widgets/TabWidget.h
    src/tests/ManualTestingHelper.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/CommandLineParser.cpp
    src/LoadDependencies.cpp
    src/AccountManager.cpp
    src/SetupApplicationIcon.cpp
    src/SystemTrayIconManager.cpp
    src/ActionShortcuts.inl
    src/BasicXMLSyntaxHighlighter.cpp
    src/EditNoteDialogsManager.cpp
    src/NoteEditorTabsAndWindowsCoordinator.cpp
    src/NoteFiltersManager.cpp
    src/color-picker-tool-button/ColorPickerActionWidget.cpp
    src/color-picker-tool-button/ColorPickerToolButton.cpp
    src/dialogs/AddAccountDialog.cpp
    src/dialogs/AddOrEditNotebookDialog.cpp
    src/dialogs/AddOrEditTagDialog.cpp
    src/dialogs/AddOrEditSavedSearchDialog.cpp
    src/dialogs/EditNoteDialog.cpp
    src/dialogs/ManageAccountsDialog.cpp
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
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
    src/models/NoteModelItem.cpp
    src/models/NoteFilterModel.cpp
    src/models/NoteModel.cpp
    src/models/FavoritesModel.cpp
    src/models/FavoritesModelItem.cpp
    src/delegates/AbstractStyledItemDelegate.cpp
    src/delegates/NoteItemDelegate.cpp
    src/delegates/NotebookItemDelegate.cpp
    src/delegates/DeletedNoteItemDelegate.cpp
    src/delegates/DirtyColumnDelegate.cpp
    src/delegates/FavoriteItemDelegate.cpp
    src/delegates/FromLinkedNotebookColumnDelegate.cpp
    src/delegates/SynchronizableColumnDelegate.cpp
    src/delegates/TagItemDelegate.cpp
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
    src/widgets/NewListItemLineEdit.cpp
    src/widgets/NotebookModelItemInfoWidget.cpp
    src/widgets/SavedSearchModelItemInfoWidget.cpp
    src/widgets/NoteEditorWidget.cpp
    src/widgets/NoteTagsWidget.cpp
    src/widgets/TagModelItemInfoWidget.cpp
    src/widgets/AbstractFilterByModelItemWidget.cpp
    src/widgets/TabWidget.cpp
    src/tests/ManualTestingHelper.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui
    src/dialogs/AddAccountDialog.ui
    src/dialogs/AddOrEditNotebookDialog.ui
    src/dialogs/AddOrEditTagDialog.ui
    src/dialogs/AddOrEditSavedSearchDialog.ui
    src/dialogs/EditNoteDialog.ui
    src/dialogs/ManageAccountsDialog.ui
    src/insert-table-tool-button/TableSettingsDialog.ui
    src/widgets/FindAndReplaceWidget.ui
    src/widgets/ListItemWidget.ui
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
    resource/icons/other/icons.qrc
    src/tests/test_notes.qrc)

qt_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

set(THIRDPARTY_LIBS ${QT_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${QEVERCLOUD_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${LIBQUENTIER_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${Boost_LIBRARIES})
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND THIRDPARTY_LIBS wsock32 ws2_32)
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

target_link_libraries(${PROJECT_NAME} ${THIRDPARTY_LIBS})

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
    src/models/TagCache.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
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
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
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
if(NOT CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(NOT CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        BUNDLE DESTINATION .)

# install application bundle
if((WIN32 OR APPLE) AND CREATE_BUNDLE)
  if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      set(DEBUG_SUFFIX "d")
    else()
      set(DEBUG_SUFFIX "")
    endif()
  endif()

  if(WIN32)
    set(APPS ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}.exe)
  elseif(APPLE)
    set(APPS ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app)
  endif()

  set(DIRS ${THIRDPARTY_LIB_DIRS})
  foreach(THIRDPARTY_LIB_DIR ${THIRDPARTY_LIB_DIRS})
    list(APPEND DIRS ${THIRDPARTY_LIB_DIR}/../bin)
  endforeach()

  if(WIN32)
    if(USE_QT5)
      install(CODE "
              message(STATUS \"Running deploy Qt tool: ${DEPLOYQT_TOOL}\")
              execute_process(COMMAND \"${DEPLOYQT_TOOL}\" ${APPS})
              " COMPONENT Runtime)

      if(QUENTIER_USE_QT_WEB_ENGINE)
        set(QTWEBENGINEPROCESS ${Qt5Core_DIR}/../../../bin/QtWebEngineProcess${DEBUG_SUFFIX}.exe)
        install(FILES ${QTWEBENGINEPROCESS} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
        install(DIRECTORY ${Qt5Core_DIR}/../../../resources DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
        install(DIRECTORY ${Qt5Core_DIR}/../../../translations/qtwebengine_locales DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/translations)
      endif()

      # deploying the SQLite driver which windeployqt/macdeployqt misses for some reason
      install(FILES ${Qt5Core_DIR}/../../../plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/sqldrivers)

      # fixup other dependencies not taken care of by windeployqt
      install(CODE "
              include(CMakeParseArguments)
              if(${CMAKE_VERSION} VERSION_LESS \"3.8.0\")
                include(${QUENTIER_CMAKE_MODULE_PATH}/BundleUtilities.cmake)
              else()
                include(BundleUtilities)
              endif()
              include(InstallRequiredSystemLibraries)
              fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
              " COMPONENT Runtime)
    else()
      install(CODE "
              include(DeployQt4)
              include(InstallRequiredSystemLibraries)
              fixup_qt4_executable(${APPS} \"qsqlite\" \"\" \"${DIRS}\")
              " COMPONENT Runtime)
    endif(USE_QT5)
  elseif(APPLE)
    install(CODE "
            message(STATUS \"Running deploy Qt tool: ${DEPLOYQT_TOOL}\")
            execute_process(COMMAND \"${DEPLOYQT_TOOL}\" ${APPS} -no-strip ERROR_QUIET)
            execute_process(COMMAND \"${CMAKE_INSTALL_NAME_TOOL}\" -add_rpath @executable_path/../Frameworks ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME})
            " COMPONENT Runtime)
  endif()
endif()

set(QUENTIER_CPPCHECKABLE_SOURCES "${${PROJECT_NAME}_SOURCES}")

set(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${${PROJECT_NAME}_INCLUDE_DIRS}")
list(APPEND QUENTIER_CPPCHECKABLE_INCLUDE_DIRS "${LIBQUENTIER_INCLUDE_DIRS}")

include(QuentierSetupCppCheck)
