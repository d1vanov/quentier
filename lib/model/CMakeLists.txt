cmake_minimum_required(VERSION 2.8)

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
    LogViewerModelLogFileParser.h)

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
    LogViewerModelLogFileParser.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
