cmake_minimum_required(VERSION 2.8)

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

add_sanitizers(${PROJECT_NAME})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} quentier_model ${THIRDPARTY_LIBS})
