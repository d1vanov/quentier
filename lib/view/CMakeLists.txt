cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_view)

set(HEADERS
    AbstractMultiSelectionItemView.h
    ItemSelectionModel.h
    ItemView.h
    DeletedNoteItemView.h
    FavoriteItemView.h
    NotebookItemView.h
    NoteListView.h
    TagItemView.h
    SavedSearchItemView.h)

set(SOURCES
    AbstractMultiSelectionItemView.cpp
    ItemSelectionModel.cpp
    ItemView.cpp
    DeletedNoteItemView.cpp
    FavoriteItemView.cpp
    NotebookItemView.cpp
    NoteListView.cpp
    TagItemView.cpp
    SavedSearchItemView.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_view ${PROJECT_NAME} CACHE INTERNAL "quentier_view")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
