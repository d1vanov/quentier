cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_view)

set(HEADERS
    ItemView.h
    DeletedNoteItemView.h
    FavoriteItemView.h
    NotebookItemView.h
    NoteListView.h
    TagItemView.h
    SavedSearchItemView.h)

set(SOURCES
    ItemView.cpp
    DeletedNoteItemView.cpp
    FavoriteItemView.cpp
    NotebookItemView.cpp
    NoteListView.cpp
    TagItemView.cpp
    SavedSearchItemView.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(quentier_view ${PROJECT_NAME} CACHE INTERNAL "quentier_view")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})