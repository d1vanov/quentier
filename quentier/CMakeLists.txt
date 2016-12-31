cmake_minimum_required(VERSION 2.8)

include(QuentierCMakePolicies)
SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(quentier VERSION 0.2.0)
else()
  project(quentier)
  set(PROJECT_VERSION_MAJOR "0")
  set(PROJECT_VERSION_MINOR "2")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2016")
set(PROJECT_DOMAIN_FIRST "d1vanov")
set(PROJECT_DOMAIN_SECOND "com")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

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

include(QuentierAdditionalCompilerWarnings)

include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libquentier/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libquentier/headers)

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/AccountToKey.h
    src/AccountManager.h
    src/BasicXMLSyntaxHighlighter.h
    src/NoteEditorTabWidgetManager.h
    src/color-picker-tool-button/ColorPickerActionWidget.h
    src/color-picker-tool-button/ColorPickerToolButton.h
    src/dialogs/AddAccountDialog.h
    src/dialogs/AddOrEditNotebookDialog.h
    src/dialogs/AddOrEditTagDialog.h
    src/dialogs/AddOrEditSavedSearchDialog.h
    src/dialogs/ManageAccountsDialog.h
    src/models/ColumnChangeRerouter.h
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
    src/delegates/DeletedNoteTitleColumnDelegate.h
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
    src/views/FavoriteItemView.h
    src/views/NotebookItemView.h
    src/views/TagItemView.h
    src/views/SavedSearchItemView.h
    src/widgets/FindAndReplaceWidget.h
    src/widgets/FlowLayout.h
    src/widgets/NewTagLineEditor.h
    src/widgets/NotebookModelItemInfoWidget.h
    src/widgets/SavedSearchModelItemInfoWidget.h
    src/widgets/NoteEditorWidget.h
    src/widgets/NoteTagWidget.h
    src/widgets/NoteTagsWidget.h
    src/widgets/TagModelItemInfoWidget.h
    src/widgets/TabWidget.h
    src/tests/ManualTestingHelper.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/AccountToKey.cpp
    src/AccountManager.cpp
    src/ActionShortcuts.inl
    src/BasicXMLSyntaxHighlighter.cpp
    src/NoteEditorTabWidgetManager.cpp
    src/color-picker-tool-button/ColorPickerActionWidget.cpp
    src/color-picker-tool-button/ColorPickerToolButton.cpp
    src/dialogs/AddAccountDialog.cpp
    src/dialogs/AddOrEditNotebookDialog.cpp
    src/dialogs/AddOrEditTagDialog.cpp
    src/dialogs/AddOrEditSavedSearchDialog.cpp
    src/dialogs/ManageAccountsDialog.cpp
    src/insert-table-tool-button/InsertTableToolButton.cpp
    src/insert-table-tool-button/TableSettingsDialog.cpp
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelector.cpp
    src/models/ColumnChangeRerouter.cpp
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
    src/delegates/DeletedNoteTitleColumnDelegate.cpp
    src/delegates/DirtyColumnDelegate.cpp
    src/delegates/FavoriteItemDelegate.cpp
    src/delegates/FromLinkedNotebookColumnDelegate.cpp
    src/delegates/SynchronizableColumnDelegate.cpp
    src/delegates/TagItemDelegate.cpp
    src/views/ItemView.cpp
    src/views/FavoriteItemView.cpp
    src/views/NotebookItemView.cpp
    src/views/TagItemView.cpp
    src/views/SavedSearchItemView.cpp
    src/widgets/FindAndReplaceWidget.cpp
    src/widgets/FlowLayout.cpp
    src/widgets/NewTagLineEditor.cpp
    src/widgets/NotebookModelItemInfoWidget.cpp
    src/widgets/SavedSearchModelItemInfoWidget.cpp
    src/widgets/NoteEditorWidget.cpp
    src/widgets/NoteTagWidget.cpp
    src/widgets/NoteTagsWidget.cpp
    src/widgets/TagModelItemInfoWidget.cpp
    src/widgets/TabWidget.cpp
    src/tests/ManualTestingHelper.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui
    src/dialogs/AddAccountDialog.ui
    src/dialogs/AddOrEditNotebookDialog.ui
    src/dialogs/AddOrEditTagDialog.ui
    src/dialogs/AddOrEditSavedSearchDialog.ui
    src/dialogs/ManageAccountsDialog.ui
    src/insert-table-tool-button/TableSettingsDialog.ui
    src/widgets/FindAndReplaceWidget.ui
    src/widgets/NewTagLineEditor.ui
    src/widgets/NotebookModelItemInfoWidget.ui
    src/widgets/SavedSearchModelItemInfoWidget.ui
    src/widgets/NoteEditorWidget.ui
    src/widgets/NoteTagWidget.ui
    src/widgets/TagModelItemInfoWidget.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

set(RESOURCES
    resource/icons/themes/tango/tango.qrc
    resource/icons/themes/oxygen/oxygen.qrc
    resource/icons/other/icons.qrc
    src/tests/test_notes.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})
target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES} ${LIB_QUENTIER} ${THIRDPARTY_LIBS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
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
target_link_libraries(${PROJECT_NAME}_model_test ${QT_LIBRARIES} ${LIB_QUENTIER} ${THIRDPARTY_LIBS})

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/models")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/widgets")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/tests")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/color-picker-tool-button")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/insert-table-tool-button")

# modify the sources list for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# install executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
