cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_model)

set(HEADERS
    common/ColumnChangeRerouter.h
    common/IModelItem.h
    common/AbstractItemModel.h
    common/NewItemNameGenerator.hpp
    favorites/FavoritesModel.h
    favorites/FavoritesModelItem.h
    log_viewer/LogViewerModel.h
    log_viewer/LogViewerModelFileReaderAsync.h
    log_viewer/LogViewerModelLogFileParser.h
    note/NoteModelItem.h
    note/NoteModel.h
    note/NoteCache.h
    notebook/AllNotebooksRootItem.h
    notebook/INotebookModelItem.h
    notebook/InvisibleNotebookRootItem.h
    notebook/LinkedNotebookRootItem.h
    notebook/NotebookCache.h
    notebook/NotebookItem.h
    notebook/NotebookModel.h
    notebook/StackItem.h
    saved_search/AllSavedSearchesRootItem.h
    saved_search/InvisibleSavedSearchRootItem.h
    saved_search/ISavedSearchModelItem.h
    saved_search/SavedSearchCache.h
    saved_search/SavedSearchModel.h
    saved_search/SavedSearchItem.h
    tag/AllTagsRootItem.h
    tag/InvisibleTagRootItem.h
    tag/ITagModelItem.h
    tag/TagCache.h
    tag/TagLinkedNotebookRootItem.h
    tag/TagItem.h
    tag/TagLinkedNotebookRootItem.h
    tag/TagModel.h)

set(SOURCES
    common/ColumnChangeRerouter.cpp
    common/AbstractItemModel.cpp
    favorites/FavoritesModel.cpp
    favorites/FavoritesModelItem.cpp
    log_viewer/LogViewerModel.cpp
    log_viewer/LogViewerModelFileReaderAsync.cpp
    log_viewer/LogViewerModelLogFileParser.cpp
    note/NoteModelItem.cpp
    note/NoteModel.cpp
    notebook/INotebookModelItem.cpp
    notebook/LinkedNotebookRootItem.cpp
    notebook/NotebookItem.cpp
    notebook/NotebookModel.cpp
    notebook/StackItem.cpp
    saved_search/ISavedSearchModelItem.cpp
    saved_search/SavedSearchItem.cpp
    saved_search/SavedSearchModel.cpp
    tag/ITagModelItem.cpp
    tag/TagLinkedNotebookRootItem.cpp
    tag/TagModel.cpp
    tag/TagItem.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Boost::boost)

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
  	  Qt6::Core
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
	PUBLIC
  	  Qt5::Core
      Libquentier::libqt5quentier)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_model ${PROJECT_NAME} CACHE INTERNAL "quentier_model")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")

add_subdirectory(tests)
