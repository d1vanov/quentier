cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_view)

set(HEADERS
    ../../src/views/ItemView.h
    ../../src/views/DeletedNoteItemView.h
    ../../src/views/FavoriteItemView.h
    ../../src/views/NotebookItemView.h
    ../../src/views/NoteListView.h
    ../../src/views/TagItemView.h
    ../../src/views/SavedSearchItemView.h)

set(SOURCES
    ../../src/views/ItemView.cpp
    ../../src/views/DeletedNoteItemView.cpp
    ../../src/views/FavoriteItemView.cpp
    ../../src/views/NotebookItemView.cpp
    ../../src/views/NoteListView.cpp
    ../../src/views/TagItemView.cpp
    ../../src/views/SavedSearchItemView.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
