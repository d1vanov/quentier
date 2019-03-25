cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_delegate)

set(HEADERS
    AbstractStyledItemDelegate.h
    LimitedFontsDelegate.h
    NoteItemDelegate.h
    NotebookItemDelegate.h
    DeletedNoteItemDelegate.h
    DirtyColumnDelegate.h
    FavoriteItemDelegate.h
    FromLinkedNotebookColumnDelegate.h
    LogViewerDelegate.h
    SynchronizableColumnDelegate.h
    TagItemDelegate.h)

set(SOURCES
    AbstractStyledItemDelegate.cpp
    LimitedFontsDelegate.cpp
    NoteItemDelegate.cpp
    NotebookItemDelegate.cpp
    DeletedNoteItemDelegate.cpp
    DirtyColumnDelegate.cpp
    FavoriteItemDelegate.cpp
    FromLinkedNotebookColumnDelegate.cpp
    LogViewerDelegate.cpp
    SynchronizableColumnDelegate.cpp
    TagItemDelegate.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(quentier_delegate ${PROJECT_NAME} CACHE INTERNAL "quentier_delegate")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})
