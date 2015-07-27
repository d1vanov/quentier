cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(libqutenote)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(3rdparty)

get_directory_property(THIRDPARTY_INCLUDE_DIRS DIRECTORY 3rdparty DEFINITION THIRDPARTY_INCLUDE_DIRS)
get_directory_property(THIRDPARTY_SOURCES DIRECTORY 3rdparty DEFINITION THIRDPARTY_SOURCES)

include(QuteNoteAdditionalCompilerWarnings)

get_directory_property(QEVERCLOUD_LIB DIRECTORY 3rdparty DEFINITION QEVERCLOUD_LIB)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  get_directory_property(STACKTRACE_LIB DIRECTORY 3rdparty DEFINITION STACKTRACE_LIB)
endif()

if(NOT USE_SYSTEM_SIMPLECRYPT)
  get_directory_property(SIMPLECRYPT_LIB DIRECTORY 3rdparty DEFINITION SIMPLECRYPT_LIB)
endif()

if(NOT USE_SYSTEM_QT_KEYCHAIN)
  get_directory_property(QTKEYCHAIN_LIB DIRECTORY 3rdparty DEFINITION QTKEYCHAIN_LIB)
endif()

if(NOT USE_SYSTEM_QSLOG)
  get_directory_property(QSLOG_LIB DIRECTORY 3rdparty DEFINITION QSLOG_LIB)
endif()

if(NOT USE_SYSTEM_TIDY_HTML5)
  get_directory_property(TIDY_HTML5_LIB DIRECTORY 3rdparty DEFINITION TIDY_HTML5_LIB)
endif()

if(NOT USE_QT5)
  if(NOT USE_SYSTEM_QT4_MIMETYPES)
    get_directory_property(QT4_MIMETYPES_LIB DIRECTORY 3rdparty DEFINITION QT4_MIMETYPES_LIB)
  endif()
endif()

set(THIRDPARTY_LIBS ${QTKEYCHAIN_LIB})
list(APPEND THIRDPARTY_LIBS ${SIMPLECRYPT_LIB})
list(APPEND THIRDPARTY_LIBS ${QEVERCLOUD_LIB})
list(APPEND THIRDPARTY_LIBS ${QSLOG_LIB})
list(APPEND THIRDPARTY_LIBS ${TIDY_HTML5_LIB})
list(APPEND THIRDPARTY_LIBS ${OPENSSL_LIBRARIES})
if(NOT USE_QT5)
  list(APPEND THIRDPARTY_LIBS ${QT4_MIMETYPES_LIB})
endif()
if(LIBCPP)
  list(APPEND THIRDPARTY_LIBS ${LIBCPP})
endif()
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND THIRDPARTY_LIBS ${STACKTRACE_LIB})
endif()

include_directories(${THIRDPARTY_INCLUDE_DIRS})

set(${PROJECT_NAME}_HEADERS
    src/note_editor/AttachmentStoragePathConfigDialog.h
    src/note_editor/EncryptedAreaPlugin.h
    src/note_editor/DecryptedTextCache.h
    src/note_editor/GenericResourceDisplayWidget.h
    src/note_editor/NoteDecryptionDialog.h
    src/note_editor/NoteEditorPage.h
    src/note_editor/NoteEditor.h
    src/note_editor/NoteEditor_p.h
    src/note_editor/INoteEditorPlugin.h
    src/note_editor/NoteEditorPluginFactory.h
    src/note_editor/NoteEditorPluginFactory_p.h
    src/note_editor/PluginInitializationException.h
    src/note_editor/ResourceFileStorageManager.h
    src/note_editor/ResourceFileStorageManager_p.h
    src/note_editor/ResourceLocalFileStorageFolderNotFoundException.h
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
    src/client/types/RegisterMetatypes.h
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
    src/client/enml/ENMLConverter_p.h
    src/client/enml/HTMLCleaner.h
    src/client/local_storage/ILocalStorageCacheExpiryChecker.h
    src/client/local_storage/DatabaseOpeningException.h
    src/client/local_storage/DatabaseSqlErrorException.h
    src/client/local_storage/DefaultLocalStorageCacheExpiryChecker.h
    src/client/local_storage/LocalStorageCacheManager.h
    src/client/local_storage/LocalStorageCacheManager_p.h
    src/client/local_storage/LocalStorageCacheManagerException.h
    src/client/local_storage/LocalStorageManager.h
    src/client/local_storage/LocalStorageManager_p.h
    src/client/local_storage/LocalStorageManagerThreadWorker.h
    src/client/local_storage/NoteSearchQuery.h
    src/client/local_storage/NoteSearchQuery_p.h
    src/client/local_storage/Transaction.h
    src/client/synchronization/NoteStore.h
    src/client/synchronization/SynchronizationManager.h
    src/client/synchronization/SynchronizationManager_p.h
    src/client/synchronization/RemoteToLocalSynchronizationManager.h
    src/client/synchronization/SendLocalChangesManager.h
    src/logging/QuteNoteLogger.h
    src/logging/LoggerInitializationException.h
    src/tools/ApplicationSettings.h
    src/tools/BasicXMLSyntaxHighlighter.h
    src/tools/DesktopServices.h
    src/tools/EmptyDataElementException.h
    src/tools/EncryptionManager.h
    src/tools/EncryptionManager_p.h
    src/tools/EventLoopWithExitStatus.h
    src/tools/FileIOThreadWorker.h
    src/tools/FileIOThreadWorker_p.h
    src/tools/IQuteNoteException.h
    src/tools/Linkage.h
    src/tools/SysInfo.h
    src/tools/Printable.h
    src/tools/QuteNoteApplication.h
    src/tools/QuteNoteCheckPtr.h
    src/tools/NullPtrException.h
    src/tools/TypeWithError.h
    src/tools/qt4helper.h)

set(${PROJECT_NAME}_SOURCES
    src/note_editor/AttachmentStoragePathConfigDialog.cpp
    src/note_editor/EncryptedAreaPlugin.cpp
    src/note_editor/GenericResourceDisplayWidget.cpp
    src/note_editor/INoteEditorPlugin.cpp
    src/note_editor/NoteEditorPage.cpp
    src/note_editor/NoteEditor.cpp
    src/note_editor/NoteEditor_p.cpp
    src/note_editor/NoteEditorPluginFactory.cpp
    src/note_editor/NoteEditorPluginFactory_p.cpp
    src/note_editor/NoteDecryptionDialog.cpp
    src/note_editor/PluginInitializationException.cpp
    src/note_editor/ResourceFileStorageManager.cpp
    src/note_editor/ResourceFileStorageManager_p.cpp
    src/note_editor/ResourceLocalFileStorageFolderNotFoundException.cpp
    src/client/Utility.cpp
    src/client/types/ISharedNotebook.cpp
    src/client/types/IResource.cpp
    src/client/types/IUser.cpp
    src/client/types/LinkedNotebook.cpp
    src/client/types/Note.cpp
    src/client/types/Notebook.cpp
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
    src/client/types/RegisterMetatypes.cpp
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
    src/client/enml/ENMLConverter_p.cpp
    src/client/enml/HTMLCleaner.cpp
    src/client/local_storage/ILocalStorageCacheExpiryChecker.cpp
    src/client/local_storage/DatabaseOpeningException.cpp
    src/client/local_storage/DatabaseSqlErrorException.cpp
    src/client/local_storage/DefaultLocalStorageCacheExpiryChecker.cpp
    src/client/local_storage/LocalStorageManager.cpp
    src/client/local_storage/LocalStorageManager_p.cpp
    src/client/local_storage/LocalStorageCacheManagerException.cpp
    src/client/local_storage/LocalStorageCacheManager.cpp
    src/client/local_storage/LocalStorageCacheManager_p.cpp
    src/client/local_storage/LocalStorageManagerThreadWorker.cpp
    src/client/local_storage/NoteSearchQuery.cpp
    src/client/local_storage/NoteSearchQuery_p.cpp
    src/client/local_storage/Transaction.cpp
    src/client/synchronization/NoteStore.cpp
    src/client/synchronization/SynchronizationManager.cpp
    src/client/synchronization/SynchronizationManager_p.cpp
    src/client/synchronization/RemoteToLocalSynchronizationManager.cpp
    src/client/synchronization/SendLocalChangesManager.cpp
    src/logging/LoggerInitializationException.cpp
    src/tools/ApplicationSettings.cpp
    src/tools/BasicXMLSyntaxHighlighter.cpp
    src/tools/DesktopServices.cpp
    src/tools/EmptyDataElementException.cpp
    src/tools/EncryptionManager.cpp
    src/tools/EncryptionManager_p.cpp
    src/tools/EventLoopWithExitStatus.cpp
    src/tools/FileIOThreadWorker.cpp
    src/tools/FileIOThreadWorker_p.cpp
    src/tools/IQuteNoteException.cpp
    src/tools/Printable.cpp
    src/tools/QuteNoteApplication.cpp
    src/tools/NullPtrException.cpp
    src/tools/TypeWithError.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/windows/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/linux/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND ${PROJECT_NAME}_SOURCES src/tools/darwin/SysInfo.cpp)
endif()

set(RESOURCES
    src/note_editor/icons/checkbox_icons/checkbox_icons.qrc
    src/note_editor/icons/encrypted_area_icons/encrypted_area_icons.qrc
    src/note_editor/icons/generic_resource_icons/generic_resource_icons.qrc
    src/note_editor/javascript/jquery/jquery.qrc
    src/note_editor/javascript/colResizable/colResizable.qrc
    src/note_editor/javascript/scripts/scripts.qrc
    src/client/enml/enml.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

set(FORMS
    src/note_editor/AttachmentStoragePathConfigDialog.ui
    src/note_editor/EncryptedAreaPlugin.ui
    src/note_editor/NoteDecryptionDialog.ui
    src/note_editor/GenericResourceDisplayWidget.ui)

if(USE_QT5)
  qt5_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
else()
  qt4_wrap_ui(${PROJECT_NAME}_FORMS_HEADERS ${FORMS})
endif()

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES}
            ${${PROJECT_NAME}_RESOURCES_RCC}
            ${${PROJECT_NAME}_FORMS_HEADERS})

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

target_link_libraries(${PROJECT_NAME} ${THIRDPARTY_LIBS})

set(TEST_HEADERS
    src/tests/EncryptionManagerTests.h
    src/tests/ENMLConverterTests.h
    src/tests/LocalStorageCacheAsyncTester.h
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
    src/tests/EncryptionManagerTests.cpp
    src/tests/ENMLConverterTests.cpp
    src/tests/LocalStorageCacheAsyncTester.cpp
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

set(TEST_RESOURCES
    src/tests/test_resources.qrc)

if(USE_QT5)
    qt5_add_resources(${PROJECT_NAME}_TEST_RESOURCES_RCC ${TEST_RESOURCES})
else()
    qt4_add_resources(${PROJECT_NAME}_TEST_RESOURCES_RCC ${TEST_RESOURCES})
endif()

add_executable(test_${PROJECT_NAME} ${TEST_HEADERS} ${TEST_SOURCES} ${${PROJECT_NAME}_TEST_RESOURCES_RCC})
add_test(test_${PROJECT_NAME} test_${PROJECT_NAME})

if(USE_QT5)
  qt5_use_modules(test_${PROJECT_NAME} Core)
  qt5_use_modules(test_${PROJECT_NAME} Test)
  qt5_use_modules(test_${PROJECT_NAME} Sql)
else()
  target_link_libraries(test_${PROJECT_NAME} ${QT_LIBRARIES})
endif()

target_link_libraries(test_${PROJECT_NAME} ${PROJECT_NAME} ${THIRDPARTY_LIBS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "BUILDING_QUTE_NOTE_DLL=1")
endif()

# modifying sources list with absolute paths for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})
prepend_path(TEST_SOURCES "${TEST_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# collect the list of sources to be checked by static analyzer
set(${PROJECT_NAME}_CHECKABLE_SOURCES ${${PROJECT_NAME}_SOURCES})
list(APPEND ${PROJECT_NAME}_CHECKABLE_SOURCES ${TEST_SOURCES})
list(APPEND ${PROJECT_NAME}_CHECKABLE_SOURCES ${THIRDPARTY_SOURCES})

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()

