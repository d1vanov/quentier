cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_delegate)

set(HEADERS
    ../../src/delegates/AbstractStyledItemDelegate.h
    ../../src/delegates/LimitedFontsDelegate.h
    ../../src/delegates/NoteItemDelegate.h
    ../../src/delegates/NotebookItemDelegate.h
    ../../src/delegates/DeletedNoteItemDelegate.h
    ../../src/delegates/DirtyColumnDelegate.h
    ../../src/delegates/FavoriteItemDelegate.h
    ../../src/delegates/FromLinkedNotebookColumnDelegate.h
    ../../src/delegates/LogViewerDelegate.h
    ../../src/delegates/SynchronizableColumnDelegate.h
    ../../src/delegates/TagItemDelegate.h)

  set(SOURCES
    ../../src/delegates/AbstractStyledItemDelegate.cpp
    ../../src/delegates/LimitedFontsDelegate.cpp
    ../../src/delegates/NoteItemDelegate.cpp
    ../../src/delegates/NotebookItemDelegate.cpp
    ../../src/delegates/DeletedNoteItemDelegate.cpp
    ../../src/delegates/DirtyColumnDelegate.cpp
    ../../src/delegates/FavoriteItemDelegate.cpp
    ../../src/delegates/FromLinkedNotebookColumnDelegate.cpp
    ../../src/delegates/LogViewerDelegate.cpp
    ../../src/delegates/SynchronizableColumnDelegate.cpp
    ../../src/delegates/TagItemDelegate.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
