cmake_minimum_required(VERSION 3.5.1)

SET_POLICIES()

project(quentier_model_tests)

set(HEADERS
    modeltest.h
    TestMacros.h
    SavedSearchModelTestHelper.h
    TagModelTestHelper.h
    NotebookModelTestHelper.h
    NoteModelTestHelper.h
    FavoritesModelTestHelper.h
    ModelTester.h)

set(SOURCES
    modeltest.cpp
    SavedSearchModelTestHelper.cpp
    TagModelTestHelper.cpp
    NotebookModelTestHelper.cpp
    NoteModelTestHelper.cpp
    FavoritesModelTestHelper.cpp
    ModelTester.cpp)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  CXX_STANDARD 14
  CXX_EXTENSIONS OFF)

add_sanitizers(${PROJECT_NAME})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} quentier_model ${THIRDPARTY_LIBS})
