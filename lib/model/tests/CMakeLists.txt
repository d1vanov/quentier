cmake_minimum_required(VERSION 3.16.3)

SET_POLICIES()

project(quentier_model_tests)

set(HEADERS
    FavoritesModelTestHelper.h
    ModelTester.h
    NoteModelTestHelper.h
    NotebookModelTestHelper.h
    SavedSearchModelTestHelper.h
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
    TagModelTestHelper.cpp
    modeltest.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF)

add_sanitizers(${PROJECT_NAME})

add_test(${PROJECT_NAME} ${PROJECT_NAME})
