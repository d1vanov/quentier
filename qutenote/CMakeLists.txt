cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(qutenote VERSION 0.2.0)
else()
  project(qutenote)
  set(PROJECT_VERSION_MAJOR "0")
  set(PROJECT_VERSION_MINOR "2")
  set(PROJECT_VERSION_PATCH "0")
  set(PROJECT_VERSION_COUNT 3)
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(PROJECT_VENDOR "Dmitry Ivanov")
set(PROJECT_COPYRIGHT_YEAR "2016")
set(PROJECT_DOMAIN_FIRST "d1vanov")
set(PROJECT_DOMAIN_SECOND "com")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

if(APPLE)
    set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_COPYRIGHT "${PROJECT_COPYRIGHT_YEAR} ${PROJECT_VENDOR}")
    # set(MACOSX_BUNDLE_ICON_FILE "qute_note_icon.icns")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_DOMAIN_SECOND}.${PROJECT_DOMAIN_FIRST}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")

    set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources")
    # set(MACOSX_BUNDLE_ICON "${ICONS_DIR}/${MACOSX_BUNDLE_ICON_FILE}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})
    # execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BUNDLE_ICON} ${MACOSX_BUNDLE_RESOURCES})
endif()

include(QuteNoteAdditionalCompilerWarnings)

include_directories(${THIRDPARTY_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libqutenote/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libqutenote/headers)

set(${PROJECT_NAME}_HEADERS
    src/MainWindow.h
    src/BasicXMLSyntaxHighlighter.h
    src/FindAndReplaceWidget.h
    src/color-picker-tool-button/ColorPickerActionWidget.h
    src/color-picker-tool-button/ColorPickerToolButton.h
    src/models/NewItemNameGenerator.hpp
    src/models/SavedSearchModel.h
    src/models/SavedSearchModelItem.h
    src/models/TagModel.h
    src/models/TagModelItem.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
    src/models/NoteModelItem.h
    src/models/NoteModel.h
    src/insert-table-tool-button/InsertTableToolButton.h
    src/insert-table-tool-button/TableSettingsDialog.h
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.h
    src/insert-table-tool-button/TableSizeSelectorActionWidget.h
    src/insert-table-tool-button/TableSizeSelector.h
    src/tests/ManualTestingHelper.h)

set(${PROJECT_NAME}_SOURCES
    src/MainWindow.cpp
    src/ActionShortcuts.inl
    src/BasicXMLSyntaxHighlighter.cpp
    src/FindAndReplaceWidget.cpp
    src/color-picker-tool-button/ColorPickerActionWidget.cpp
    src/color-picker-tool-button/ColorPickerToolButton.cpp
    src/insert-table-tool-button/InsertTableToolButton.cpp
    src/insert-table-tool-button/TableSettingsDialog.cpp
    src/insert-table-tool-button/TableSizeConstraintsActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelectorActionWidget.cpp
    src/insert-table-tool-button/TableSizeSelector.cpp
    src/models/SavedSearchModel.cpp
    src/models/SavedSearchModelItem.cpp
    src/models/TagModel.cpp
    src/models/TagModelItem.cpp
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
    src/models/NoteModelItem.cpp
    src/models/NoteModel.cpp
    src/tests/ManualTestingHelper.cpp
    src/main.cpp)

set(FORMS
    src/MainWindow.ui
    src/FindAndReplaceWidget.ui
    src/insert-table-tool-button/TableSettingsDialog.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

set(RESOURCES
    resource/icons.qrc
    src/tests/test_notes.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
               ${${PROJECT_NAME}_HEADERS}
               ${${PROJECT_NAME}_SOURCES}
               ${${PROJECT_NAME}_FORMS_HEADERS}
               ${${PROJECT_NAME}_RESOURCES_RCC})

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core)
  qt5_use_modules(${PROJECT_NAME} Gui)
  qt5_use_modules(${PROJECT_NAME} Widgets)
  qt5_use_modules(${PROJECT_NAME} Network)
  qt5_use_modules(${PROJECT_NAME} Xml)
  qt5_use_modules(${PROJECT_NAME} Sql)
  if(USE_QT_WEB_ENGINE)
    qt5_use_modules(${PROJECT_NAME} WebEngine)
    qt5_use_modules(${PROJECT_NAME} WebEngineWidgets)
    qt5_use_modules(${PROJECT_NAME} WebSockets)
    qt5_use_modules(${PROJECT_NAME} WebChannel)
  else()
    qt5_use_modules(${PROJECT_NAME} Webkit)
    qt5_use_modules(${PROJECT_NAME} WebKitWidgets)
  endif()
else()
  target_link_libraries(${PROJECT_NAME}
                        ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} ${LIB_QUTE_NOTE} ${THIRDPARTY_LIBS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

# Set up the model tests
set(MODEL_TEST_HEADERS
    src/tests/model_test/modeltest.h
    src/tests/model_test/dynamictreemodel.h
    src/tests/model_test/Macros.h
    src/tests/model_test/SavedSearchModelTestHelper.h
    src/tests/model_test/TagModelTestHelper.h
    src/tests/model_test/NotebookModelTestHelper.h
    src/models/SavedSearchModel.h
    src/models/SavedSearchModelItem.h
    src/models/TagModel.h
    src/models/TagModelItem.h
    src/models/NotebookModel.h
    src/models/NotebookModelItem.h
    src/models/NotebookItem.h
    src/models/NotebookStackItem.h
    src/models/NoteModelItem.h
    src/models/NoteModel.h)

set(MODEL_TEST_SOURCES
    src/tests/model_test/modeltest.cpp
    src/tests/model_test/dynamictreemodel.cpp
    src/tests/model_test/SavedSearchModelTestHelper.cpp
    src/tests/model_test/TagModelTestHelper.cpp
    src/tests/model_test/NotebookModelTestHelper.cpp
    src/tests/model_test/tst_modeltest.cpp
    src/models/SavedSearchModel.cpp
    src/models/SavedSearchModelItem.cpp
    src/models/TagModel.cpp
    src/models/TagModelItem.cpp
    src/models/NotebookModel.cpp
    src/models/NotebookModelItem.cpp
    src/models/NotebookItem.cpp
    src/models/NotebookStackItem.cpp
    src/models/NoteModelItem.cpp
    src/models/NoteModel.cpp)

add_executable(${PROJECT_NAME}_model_test ${MODEL_TEST_SOURCES} ${MODEL_TEST_SOURCES})
add_test(${PROJECT_NAME}_model_test ${PROJECT_NAME}_model_test)

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME}_model_test Core)
  qt5_use_modules(${PROJECT_NAME}_model_test Gui)
  qt5_use_modules(${PROJECT_NAME}_model_test Widgets)
  qt5_use_modules(${PROJECT_NAME}_model_test Test)
  qt5_use_modules(${PROJECT_NAME}_model_test Xml)
  qt5_use_modules(${PROJECT_NAME}_model_test Sql)
else()
  target_link_libraries(${PROJECT_NAME}_model_test ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME}_model_test ${LIB_QUTE_NOTE} ${THIRDPARTY_LIBS})

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/client")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/enml")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/types")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/evernote_sdk/src")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/gui")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/note_editor")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/evernote_client/tools")

# modify the sources list for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# install executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
