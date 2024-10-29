cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_view)

set(HEADERS
    AbstractNoteFilteringTreeView.h
    DeletedNoteItemView.h
    FavoriteItemView.h
    ItemSelectionModel.h
    NotebookItemView.h
    NoteListView.h
    SavedSearchItemView.h
    TagItemView.h
    TreeView.h
    Utils.h)

set(SOURCES
    AbstractNoteFilteringTreeView.cpp
    DeletedNoteItemView.cpp
    FavoriteItemView.cpp
    ItemSelectionModel.cpp
    NotebookItemView.cpp
    NoteListView.cpp
    SavedSearchItemView.cpp
    TagItemView.cpp
    TreeView.cpp
    Utils.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Qt6::Widgets
      Libquentier::libqt6quentier
      quentier_model
  )
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Qt5::Widgets
      Libquentier::libqt5quentier
      quentier_model
  )
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_view ${PROJECT_NAME} CACHE INTERNAL "quentier_view")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
