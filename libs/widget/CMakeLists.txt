cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_widget)

set(HEADERS
    ../../src/widgets/FindAndReplaceWidget.h
    ../../src/widgets/FilterByNotebookWidget.h
    ../../src/widgets/FilterBySavedSearchWidget.h
    ../../src/widgets/FilterByTagWidget.h
    ../../src/widgets/FlowLayout.h
    ../../src/widgets/ListItemWidget.h
    ../../src/widgets/LogViewerWidget.h
    ../../src/widgets/NewListItemLineEdit.h
    ../../src/widgets/NotebookModelItemInfoWidget.h
    ../../src/widgets/SavedSearchModelItemInfoWidget.h
    ../../src/widgets/NoteEditorWidget.h
    ../../src/widgets/NoteTagsWidget.h
    ../../src/widgets/TagModelItemInfoWidget.h
    ../../src/widgets/AbstractFilterByModelItemWidget.h
    ../../src/widgets/AboutQuentierWidget.h
    ../../src/widgets/TabWidget.h)

set(SOURCES
    ../../src/widgets/FindAndReplaceWidget.cpp
    ../../src/widgets/FilterByNotebookWidget.cpp
    ../../src/widgets/FilterBySavedSearchWidget.cpp
    ../../src/widgets/FilterByTagWidget.cpp
    ../../src/widgets/FlowLayout.cpp
    ../../src/widgets/ListItemWidget.cpp
    ../../src/widgets/LogViewerWidget.cpp
    ../../src/widgets/NewListItemLineEdit.cpp
    ../../src/widgets/NotebookModelItemInfoWidget.cpp
    ../../src/widgets/SavedSearchModelItemInfoWidget.cpp
    ../../src/widgets/NoteEditorWidget.cpp
    ../../src/widgets/NoteTagsWidget.cpp
    ../../src/widgets/TagModelItemInfoWidget.cpp
    ../../src/widgets/AbstractFilterByModelItemWidget.cpp
    ../../src/widgets/AboutQuentierWidget.cpp
    ../../src/widgets/TabWidget.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
