cmake_minimum_required(VERSION 2.8)

SET_POLICIES()

project(quentier_model)

set(HEADERS
    ../../src/models/ColumnChangeRerouter.h
    ../../src/models/ItemModel.h
    ../../src/models/NewItemNameGenerator.hpp
    ../../src/models/SavedSearchModel.h
    ../../src/models/SavedSearchModelItem.h
    ../../src/models/SavedSearchCache.h
    ../../src/models/TagModel.h
    ../../src/models/TagModelItem.h
    ../../src/models/TagItem.h
    ../../src/models/TagLinkedNotebookRootItem.h
    ../../src/models/TagCache.h
    ../../src/models/NotebookModel.h
    ../../src/models/NotebookModelItem.h
    ../../src/models/NotebookItem.h
    ../../src/models/NotebookStackItem.h
    ../../src/models/NotebookLinkedNotebookRootItem.h
    ../../src/models/NotebookCache.h
    ../../src/models/NoteModelItem.h
    ../../src/models/NoteModel.h
    ../../src/models/NoteCache.h
    ../../src/models/FavoritesModel.h
    ../../src/models/FavoritesModelItem.h
    ../../src/models/LogViewerModel.h
    ../../src/models/LogViewerModelFileReaderAsync.h
    ../../src/models/LogViewerModelLogFileParser.h)

set(SOURCES
    ../../src/models/ColumnChangeRerouter.cpp
    ../../src/models/ItemModel.cpp
    ../../src/models/SavedSearchModel.cpp
    ../../src/models/SavedSearchModelItem.cpp
    ../../src/models/TagModel.cpp
    ../../src/models/TagModelItem.cpp
    ../../src/models/TagItem.cpp
    ../../src/models/TagLinkedNotebookRootItem.cpp
    ../../src/models/NotebookModel.cpp
    ../../src/models/NotebookModelItem.cpp
    ../../src/models/NotebookItem.cpp
    ../../src/models/NotebookStackItem.cpp
    ../../src/models/NotebookLinkedNotebookRootItem.cpp
    ../../src/models/NoteModelItem.cpp
    ../../src/models/NoteModel.cpp
    ../../src/models/FavoritesModel.cpp
    ../../src/models/FavoritesModelItem.cpp
    ../../src/models/LogViewerModel.cpp
    ../../src/models/LogViewerModelFileReaderAsync.cpp
    ../../src/models/LogViewerModelLogFileParser.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
