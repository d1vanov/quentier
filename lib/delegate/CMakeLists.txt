cmake_minimum_required(VERSION 3.16.3) 
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

if(BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt6::Core
      Qt6::Widgets
      Libquentier::libqt6quentier)
else()
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      Qt5::Core
      Qt5::Widgets
      Libquentier::libqt5quentier)
endif()

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${QUENTIER_ROOT_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

set(quentier_delegate ${PROJECT_NAME} CACHE INTERNAL "quentier_delegate")

set(TRANSLATABLE_SOURCES
    ${HEADERS}
    ${SOURCES})

COLLECT_SOURCES_FOR_TRANSLATION(TRANSLATABLE_SOURCES, "")
