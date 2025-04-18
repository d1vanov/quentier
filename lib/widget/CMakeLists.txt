cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_widget)

set(HEADERS
    AboutQuentierWidget.h
    AbstractFilterByModelItemWidget.h
    FindAndReplaceWidget.h
    FilterByNotebookWidget.h
    FilterBySavedSearchWidget.h
    FilterBySearchStringWidget.h
    FilterByTagWidget.h
    FlowLayout.h
    ListItemWidget.h
    LogViewerWidget.h
    NewListItemLineEdit.h
    NotebookModelItemInfoWidget.h
    NoteCountLabelController.h
    NoteEditorWidget.h
    NoteEditorTabsAndWindowsCoordinator.h
    NoteFiltersManager.h
    NoteTagsWidget.h
    PanelWidget.h
    SavedSearchModelItemInfoWidget.h
    TagModelItemInfoWidget.h
    TabWidget.h
    color-picker-tool-button/ColorPickerActionWidget.h
    color-picker-tool-button/ColorPickerToolButton.h
    insert-table-tool-button/InsertTableToolButton.h
    insert-table-tool-button/TableSettingsDialog.h
    insert-table-tool-button/TableSizeConstraintsActionWidget.h
    insert-table-tool-button/TableSizeSelectorActionWidget.h
    insert-table-tool-button/TableSizeSelector.h
    panel/PanelStyleController.h
    panel/SidePanelStyleController.h)

set(SOURCES
    AboutQuentierWidget.cpp
    AbstractFilterByModelItemWidget.cpp
    FindAndReplaceWidget.cpp
    FilterByNotebookWidget.cpp
    FilterBySavedSearchWidget.cpp
    FilterBySearchStringWidget.cpp
    FilterByTagWidget.cpp
    FlowLayout.cpp
    ListItemWidget.cpp
    LogViewerWidget.cpp
    NewListItemLineEdit.cpp
    NotebookModelItemInfoWidget.cpp
    NoteCountLabelController.cpp
    NoteEditorWidget.cpp
    NoteEditorTabsAndWindowsCoordinator.cpp
    NoteFiltersManager.cpp
    NoteTagsWidget.cpp
    PanelWidget.cpp
    SavedSearchModelItemInfoWidget.cpp
    TagModelItemInfoWidget.cpp
    TabWidget.cpp
    color-picker-tool-button/ColorPickerActionWidget.cpp
    color-picker-tool-button/ColorPickerToolButton.cpp
    insert-table-tool-button/InsertTableToolButton.cpp
    insert-table-tool-button/TableSettingsDialog.cpp
    insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    insert-table-tool-button/TableSizeSelector.cpp
    panel/PanelStyleController.cpp
    panel/SidePanelStyleController.cpp)

set(FORMS
    AboutQuentierWidget.ui
    FindAndReplaceWidget.ui
    FilterBySearchStringWidget.ui
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
    ${PROJECT_SOURCE_DIR}/panel
    ${PROJECT_SOURCE_DIR}/color-picker-tool-button
    ${PROJECT_SOURCE_DIR}/insert-table-tool-button)
COLLECT_INCLUDE_DIRS(DIRS)

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(DIRS)
