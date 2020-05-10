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
    TagModel.h
    TagModelItem.h
    TagItem.h
    TagLinkedNotebookRootItem.h
    TagCache.h
    NotebookModel.h
    NotebookModelItem.h
    NotebookItem.h
    NotebookStackItem.h
    NotebookLinkedNotebookRootItem.h
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
    notebook/NotebookItem.h
    notebook/NotebookLinkedNotebookRootItem.h
    notebook/NotebookStackItem.h)

set(SOURCES
    ColumnChangeRerouter.cpp
    ItemModel.cpp
    SavedSearchModel.cpp
    SavedSearchModelItem.cpp
    TagModel.cpp
    TagModelItem.cpp
    TagItem.cpp
    TagLinkedNotebookRootItem.cpp
    NotebookModel.cpp
    NotebookModelItem.cpp
    NotebookItem.cpp
    NotebookStackItem.cpp
    NotebookLinkedNotebookRootItem.cpp
    NoteModelItem.cpp
    NoteModel.cpp
    FavoritesModel.cpp
    FavoritesModelItem.cpp
    LogViewerModel.cpp
    LogViewerModelFileReaderAsync.cpp
    LogViewerModelLogFileParser.cpp
    notebook/INotebookModelItem.cpp
    notebook/NotebookItem.cpp
    notebook/NotebookLinkedNotebookRootItem.cpp
    notebook/NotebookStackItem.cpp)

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

add_subdirectory(tests)
