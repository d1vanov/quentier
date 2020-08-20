cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_model)

set(HEADERS
    ColumnChangeRerouter.h
    ItemModel.h
    NewItemNameGenerator.hpp
    SavedSearchModel.h
    SavedSearchModelItem.h
    SavedSearchCache.h
    TagCache.h
    NotebookCache.h
    NoteModelItem.h
    NoteModel.h
    NoteCache.h
    FavoritesModel.h
    FavoritesModelItem.h
    LogViewerModel.h
    LogViewerModelFileReaderAsync.h
    LogViewerModelLogFileParser.h
    notebook/AllNotebooksRootItem.h
    notebook/INotebookModelItem.h
    notebook/InvisibleRootItem.h
    notebook/LinkedNotebookRootItem.h
    notebook/NotebookItem.h
    notebook/NotebookModel.h
    notebook/StackItem.h
    tag/ITagModelItem.h
    tag/TagLinkedNotebookRootItem.h
    tag/TagItem.h
    tag/TagModel.h)

set(SOURCES
    ColumnChangeRerouter.cpp
    ItemModel.cpp
    SavedSearchModel.cpp
    SavedSearchModelItem.cpp
    NoteModelItem.cpp
    NoteModel.cpp
    FavoritesModel.cpp
    FavoritesModelItem.cpp
    LogViewerModel.cpp
    LogViewerModelFileReaderAsync.cpp
    LogViewerModelLogFileParser.cpp
    notebook/INotebookModelItem.cpp
    notebook/LinkedNotebookRootItem.cpp
    notebook/NotebookItem.cpp
    notebook/NotebookModel.cpp
    notebook/StackItem.cpp
    tag/ITagModelItem.cpp
    tag/TagLinkedNotebookRootItem.cpp
    tag/TagModel.cpp
    tag/TagItem.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

set(quentier_model ${PROJECT_NAME} CACHE INTERNAL "quentier_model")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

COLLECT_SOURCES_FOR_CPPCHECK(SOURCES)
COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})

QUENTIER_COLLECT_HEADERS(HEADERS)
QUENTIER_COLLECT_SOURCES(SOURCES)
QUENTIER_COLLECT_INCLUDE_DIRS(${PROJECT_SOURCE_DIR})

add_subdirectory(tests)
