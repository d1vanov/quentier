cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_model_tests)

set(HEADERS
    FavoritesModelTestHelper.h
    ModelTester.h
    NoteModelTestHelper.h
    NotebookModelTestHelper.h
    SavedSearchModelTestHelper.h
    SynchronousLocalStorage.h
    SynchronousLocalStorageNotifier.h
    TagModelTestHelper.h
    TestMacros.h
    TestUtils.h
    modeltest.h)

set(SOURCES
    FavoritesModelTestHelper.cpp
    ModelTester.cpp
    NoteModelTestHelper.cpp
    NotebookModelTestHelper.cpp
    SavedSearchModelTestHelper.cpp
    SynchronousLocalStorage.cpp
    SynchronousLocalStorageNotifier.cpp
    TagModelTestHelper.cpp
    modeltest.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ../../..)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    quentier_model
    quentier_preferences
    quentier_utility)

if (BUILD_WITH_QT6)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt6::Core
      Qt6::Gui
      Qt6::Sql
      Qt6::Test
      Libquentier::libqt6quentier
      QEverCloud::qt6qevercloud)
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      Qt5::Core
      Qt5::Gui
      Qt5::Sql
      Qt5::Test
      Libquentier::libqt5quentier
      QEverCloud::qt5qevercloud)
endif()
    
set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

add_sanitizers(${PROJECT_NAME})

add_test(${PROJECT_NAME} ${PROJECT_NAME})
