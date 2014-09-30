cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(qutenotecore)

include_directories(${THIRDPARTY_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_AUTOMOC ON)

set(${PROJECT_NAME}_QT_HEADERS
    src/note_editor/HorizontalLineExtraData.h
    src/note_editor/MediaResourceTextObject.h
    src/note_editor/QuteNoteTextEdit.h
    src/note_editor/ResourceTextObject.h
    src/note_editor/ToDoCheckboxTextObject.h)

set(${PROJECT_NAME}_HEADERS
    src/client/Utility.h
    src/client/types/IDataElementWithShortcut.h
    src/client/types/ISharedNotebook.h
    src/client/types/IResource.h
    src/client/types/IUser.h
    src/client/types/LinkedNotebook.h
    src/client/types/ILocalStorageDataElement.h
    src/client/types/Note.h
    src/client/types/Notebook.h
    src/client/types/INoteStoreDataElement.h
    src/client/types/QEverCloudHelpers.h
    src/client/types/ResourceWrapper.h
    src/client/types/ResourceAdapter.h
    src/client/types/ResourceAdapterAccessException.h
    src/client/types/SavedSearch.h
    src/client/types/SharedNotebookAdapter.h
    src/client/types/SharedNotebookAdapterAccessException.h
    src/client/types/SharedNotebookWrapper.h
    src/client/types/Tag.h
    src/client/types/UserAdapter.h
    src/client/types/UserAdapterAccessException.h
    src/client/types/UserWrapper.h
    src/client/types/data/DataElementWithShortcutData.h
    src/client/types/data/LocalStorageDataElementData.h
    src/client/types/data/LinkedNotebookData.h
    src/client/types/data/NoteStoreDataElementData.h
    src/client/types/data/NoteData.h
    src/client/types/data/NotebookData.h
    src/client/types/data/ResourceWrapperData.h
    src/client/types/data/SharedNotebookWrapperData.h
    src/client/types/data/TagData.h
    src/client/types/data/SavedSearchData.h
    src/client/types/data/UserWrapperData.h
    src/client/enml/ENMLConverter.h
    src/client/local_storage/IAsyncLocalStorageManager.h
    src/client/local_storage/ILocalStorageCacheExpiryChecker.h
    src/client/local_storage/DatabaseOpeningException.h
    src/client/local_storage/DatabaseSqlErrorException.h
    src/client/local_storage/DefaultLocalStorageCacheExpiryChecker.h
    src/client/local_storage/LocalStorageCacheManager.h
    src/client/local_storage/LocalStorageCacheManager_p.h
    src/client/local_storage/LocalStorageCacheManagerException.h
    src/client/local_storage/LocalStorageManager.h
    src/client/local_storage/LocalStorageManager_p.h
    src/client/local_storage/LocalStorageManagerThread.h
    src/client/local_storage/LocalStorageManagerThreadWorker.h
    src/client/local_storage/NoteSearchQuery.h
    src/client/local_storage/NoteSearchQuery_p.h
    src/client/local_storage/Transaction.h
    src/logging/QuteNoteLogger.h
    src/logging/LoggerInitializationException.h
    src/tools/ApplicationSettings.h
    src/tools/ApplicationStoragePersistencePath.h
    src/tools/EmptyDataElementException.h
    src/tools/EventLoopWithExitStatus.h
    src/tools/IQuteNoteException.h
    src/tools/Linkage.h
    src/tools/SysInfo.h
    src/tools/Printable.h
    src/tools/QuteNoteCheckPtr.h
    src/tools/QuteNoteNullPtrException.h
    src/tools/TypeWithError.h)

if(USE_QT5)
    qt5_wrap_cpp(${PROJECT_NAME}_HEADERS_MOC ${${PROJECT_NAME}_QT_HEADERS})
else()
    qt4_wrap_cpp(${PROJECT_NAME}_HEADERS_MOC ${${PROJECT_NAME}_QT_HEADERS})
endif()

set(${PROJECT_NAME}_SOURCES
    src/note_editor/HorizontalLineExtraData.cpp
    src/note_editor/QuteNoteTextEdit.cpp
    src/note_editor/ResourceTextObject.cpp
    src/note_editor/ToDoCheckboxTextObject.cpp
    src/client/Utility.cpp
    src/client/types/ISharedNotebook.cpp
    src/client/types/IResource.cpp
    src/client/types/IUser.cpp
    src/client/types/LinkedNotebook.cpp
    src/client/types/Note.cpp
    src/client/types/Notebook.cpp
    src/client/types/QEverCloudHelpers.cpp
    src/client/types/ResourceWrapper.cpp
    src/client/types/ResourceAdapter.cpp
    src/client/types/ResourceAdapterAccessException.cpp
    src/client/types/SavedSearch.cpp
    src/client/types/SharedNotebookAdapter.cpp
    src/client/types/SharedNotebookAdapterAccessException.cpp
    src/client/types/SharedNotebookWrapper.cpp
    src/client/types/Tag.cpp
    src/client/types/UserAdapter.cpp
    src/client/types/UserAdapterAccessException.cpp
    src/client/types/UserWrapper.cpp
    src/client/types/data/DataElementWithShortcutData.cpp
    src/client/types/data/LocalStorageDataElementData.cpp
    src/client/types/data/LinkedNotebookData.cpp
    src/client/types/data/NoteStoreDataElementData.cpp
    src/client/types/data/NoteData.cpp
    src/client/types/data/NotebookData.cpp
    src/client/types/data/ResourceWrapperData.cpp
    src/client/types/data/SharedNotebookWrapperData.cpp
    src/client/types/data/TagData.cpp
    src/client/types/data/SavedSearchData.cpp
    src/client/types/data/UserWrapperData.cpp
    src/client/enml/ENMLConverter.cpp
    src/client/local_storage/IAsyncLocalStorageManager.cpp
    src/client/local_storage/ILocalStorageCacheExpiryChecker.cpp
    src/client/local_storage/DatabaseOpeningException.cpp
    src/client/local_storage/DatabaseSqlErrorException.cpp
    src/client/local_storage/DefaultLocalStorageCacheExpiryChecker.cpp
    src/client/local_storage/LocalStorageManager.cpp
    src/client/local_storage/LocalStorageManager_p.cpp
    src/client/local_storage/LocalStorageCacheManagerException.cpp
    src/client/local_storage/LocalStorageCacheManager.cpp
    src/client/local_storage/LocalStorageCacheManager_p.cpp
    src/client/local_storage/LocalStorageManagerThread.cpp
    src/client/local_storage/LocalStorageManagerThreadWorker.cpp
    src/client/local_storage/NoteSearchQuery.cpp
    src/client/local_storage/NoteSearchQuery_p.cpp
    src/client/local_storage/Transaction.cpp
    src/logging/QuteNoteLogger.cpp
    src/logging/LoggerInitializationException.cpp
    src/tools/ApplicationSettings.cpp
    src/tools/ApplicationStoragePersistencePath.cpp
    src/tools/EmptyDataElementException.cpp
    src/tools/EventLoopWithExitStatus.cpp
    src/tools/IQuteNoteException.cpp
    src/tools/Printable.cpp
    src/tools/QuteNoteNullPtrException.cpp
    src/tools/TypeWithError.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/windows/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/linux/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/darwin/SysInfo.cpp)
endif()

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_QT_HEADERS}
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES}
            ${${PROJECT_NAME}_QT_HEADERS_MOC})

if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core)
  qt5_use_modules(${PROJECT_NAME} Gui)
  qt5_use_modules(${PROJECT_NAME} Widgets)
  qt5_use_modules(${PROJECT_NAME} Network)
  qt5_use_modules(${PROJECT_NAME} Webkit)
  qt5_use_modules(${PROJECT_NAME} WebKitWidgets)
  qt5_use_modules(${PROJECT_NAME} Xml)
  qt5_use_modules(${PROJECT_NAME} Sql)
else()
  target_link_libraries(${PROJECT_NAME}
                        ${QT_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} ${SIMPLECRYPT_LIB}
                      ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES} ${QEVERCLOUD_LIB})

set(TEST_HEADERS
    src/tests/LinkedNotebookLocalStorageManagerAsyncTester.h
    src/tests/NotebookLocalStorageManagerAsyncTester.h
    src/tests/NoteLocalStorageManagerAsyncTester.h
    src/tests/NoteSearchQueryTest.h
    src/tests/ResourceLocalStorageManagerAsyncTester.h
    src/tests/TagLocalStorageManagerAsyncTester.h
    src/tests/SavedSearchLocalStorageManagerAsyncTester.h
    src/tests/UserLocalStorageManagerAsyncTester.h
    src/tests/LocalStorageManagerTests.h
    src/tests/LocalStorageManagerNoteSearchQueryTest.h
    src/tests/CoreTester.h)

set(TEST_SOURCES
    src/tests/LinkedNotebookLocalStorageManagerAsyncTester.cpp
    src/tests/NotebookLocalStorageManagerAsyncTester.cpp
    src/tests/NoteLocalStorageManagerAsyncTester.cpp
    src/tests/NoteSearchQueryTest.cpp
    src/tests/ResourceLocalStorageManagerAsyncTester.cpp
    src/tests/TagLocalStorageManagerAsyncTester.cpp
    src/tests/SavedSearchLocalStorageManagerAsyncTester.cpp
    src/tests/UserLocalStorageManagerAsyncTester.cpp
    src/tests/LocalStorageManagerTests.cpp
    src/tests/LocalStorageManagerNoteSearchQueryTest.cpp
    src/tests/CoreTester.cpp
    src/tests/CoreTestMain.cpp)

add_executable(test_${PROJECT_NAME} ${TEST_HEADERS} ${TEST_SOURCES})
add_test(test_${PROJECT_NAME} test_${PROJECT_NAME})
target_link_libraries(test_${PROJECT_NAME} ${PROJECT_NAME})

if(USE_QT5)
  qt5_use_modules(test_${PROJECT_NAME} Core)
  qt5_use_modules(test_${PROJECT_NAME} Test)
  qt5_use_modules(test_${PROJECT_NAME} Sql)
else()
  target_link_libraries(test_${PROJECT_NAME} ${QT_LIBRARIES})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "BUILDING_QUTE_NOTE_DLL=1")
endif()

# modifying sources list with absolute paths for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
