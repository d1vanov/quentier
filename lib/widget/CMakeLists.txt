cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_widget)

set(HEADERS
    FindAndReplaceWidget.h
    FilterByNotebookWidget.h
    FilterBySavedSearchWidget.h
    FilterByTagWidget.h
    FlowLayout.h
    ListItemWidget.h
    LogViewerWidget.h
    NewListItemLineEdit.h
    NotebookModelItemInfoWidget.h
    SavedSearchModelItemInfoWidget.h
    NoteCountLabelController.h
    NoteEditorWidget.h
    NoteEditorTabsAndWindowsCoordinator.h
    NoteFiltersManager.h
    NoteTagsWidget.h
    TagModelItemInfoWidget.h
    AbstractFilterByModelItemWidget.h
    AboutQuentierWidget.h
    TabWidget.h
    color-picker-tool-button/ColorPickerActionWidget.h
    color-picker-tool-button/ColorPickerToolButton.h
    insert-table-tool-button/InsertTableToolButton.h
    insert-table-tool-button/TableSettingsDialog.h
    insert-table-tool-button/TableSizeConstraintsActionWidget.h
    insert-table-tool-button/TableSizeSelectorActionWidget.h
    insert-table-tool-button/TableSizeSelector.h)

set(SOURCES
    FindAndReplaceWidget.cpp
    FilterByNotebookWidget.cpp
    FilterBySavedSearchWidget.cpp
    FilterByTagWidget.cpp
    FlowLayout.cpp
    ListItemWidget.cpp
    LogViewerWidget.cpp
    NewListItemLineEdit.cpp
    NotebookModelItemInfoWidget.cpp
    SavedSearchModelItemInfoWidget.cpp
    NoteCountLabelController.cpp
    NoteEditorWidget.cpp
    NoteEditorTabsAndWindowsCoordinator.cpp
    NoteFiltersManager.cpp
    NoteTagsWidget.cpp
    TagModelItemInfoWidget.cpp
    AbstractFilterByModelItemWidget.cpp
    AboutQuentierWidget.cpp
    TabWidget.cpp
    color-picker-tool-button/ColorPickerActionWidget.cpp
    color-picker-tool-button/ColorPickerToolButton.cpp
    insert-table-tool-button/InsertTableToolButton.cpp
    insert-table-tool-button/TableSettingsDialog.cpp
    insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    insert-table-tool-button/TableSizeSelector.cpp)

set(FORMS
    AboutQuentierWidget.ui
    FindAndReplaceWidget.ui
    ListItemWidget.ui
    LogViewerWidget.ui
    NewListItemLineEdit.ui
    NotebookModelItemInfoWidget.ui
    NoteEditorWidget.ui
    SavedSearchModelItemInfoWidget.ui
    TagModelItemInfoWidget.ui
    insert-table-tool-button/TableSettingsDialog.ui)

qt_wrap_ui(FORMS_HEADERS ${FORMS})

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${FORMS_HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_widget ${PROJECT_NAME} CACHE INTERNAL "quentier_widget")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, FORMS)

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)

set(DIRS
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/color-picker-tool-button
    ${PROJECT_SOURCE_DIR}/insert-table-tool-button)
COLLECT_INCLUDE_DIRS(DIRS)
