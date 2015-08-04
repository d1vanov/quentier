cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(libqutenote)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/headers
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
list(APPEND THIRDPARTY_LIBS ${LIBXML2_LIBRARIES})
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
include_directories(headers)

set(NOTE_EDITOR_HEADERS
    headers/qute_note/note_editor/AttachmentStoragePathConfigDialog.h
    headers/qute_note/note_editor/EncryptedAreaPlugin.h
    headers/qute_note/note_editor/DecryptedTextCache.h
    headers/qute_note/note_editor/GenericResourceDisplayWidget.h
    headers/qute_note/note_editor/NoteDecryptionDialog.h
    headers/qute_note/note_editor/NoteEditor.h
    headers/qute_note/note_editor/INoteEditorPlugin.h
    headers/qute_note/note_editor/NoteEditorPluginFactory.h
    headers/qute_note/note_editor/ResourceFileStorageManager.h)

set(TYPES_HEADERS
    headers/qute_note/types/IDataElementWithShortcut.h
    headers/qute_note/types/ISharedNotebook.h
    headers/qute_note/types/IResource.h
    headers/qute_note/types/IUser.h
    headers/qute_note/types/LinkedNotebook.h
    headers/qute_note/types/ILocalStorageDataElement.h
    headers/qute_note/types/Note.h
    headers/qute_note/types/Notebook.h
    headers/qute_note/types/INoteStoreDataElement.h
    headers/qute_note/types/ResourceWrapper.h
    headers/qute_note/types/ResourceAdapter.h
    headers/qute_note/types/SavedSearch.h
    headers/qute_note/types/SharedNotebookAdapter.h
    headers/qute_note/types/SharedNotebookWrapper.h
    headers/qute_note/types/Tag.h
    headers/qute_note/types/UserAdapter.h
    headers/qute_note/types/UserWrapper.h
    headers/qute_note/types/RegisterMetatypes.h)

set(ENML_HEADERS
    headers/qute_note/enml/ENMLConverter.h
    headers/qute_note/enml/HTMLCleaner.h)

set(LOCAL_STORAGE_HEADERS
    headers/qute_note/local_storage/ILocalStorageCacheExpiryChecker.h
    headers/qute_note/local_storage/DefaultLocalStorageCacheExpiryChecker.h
    headers/qute_note/local_storage/LocalStorageCacheManager.h
    headers/qute_note/local_storage/LocalStorageManager.h
    headers/qute_note/local_storage/LocalStorageManagerThreadWorker.h
    headers/qute_note/local_storage/NoteSearchQuery.h)

set(SYNCHRONIZATION_HEADERS
    headers/qute_note/synchronization/SynchronizationManager.h)

set(LOGGING_HEADERS
    headers/qute_note/logging/QuteNoteLogger.h)

set(EXCEPTION_HEADERS
    headers/qute_note/exception/ApplicationSettingsInitializationException.h
    headers/qute_note/exception/DatabaseOpeningException.h
    headers/qute_note/exception/DatabaseSqlErrorException.h
    headers/qute_note/exception/EmptyDataElementException.h
    headers/qute_note/exception/IQuteNoteException.h
    headers/qute_note/exception/LocalStorageCacheManagerException.h
    headers/qute_note/exception/LoggerInitializationException.h
    headers/qute_note/exception/NoteEditorPluginInitializationException.h
    headers/qute_note/exception/NullPtrException.h
    headers/qute_note/exception/ResourceAdapterAccessException.h
    headers/qute_note/exception/ResourceLocalFileStorageFolderNotFoundException.h
    headers/qute_note/exception/SharedNotebookAdapterAccessException.h
    headers/qute_note/exception/UserAdapterAccessException.h)

set(UTILITY_HEADERS
    headers/qute_note/utility/ApplicationSettings.h
    headers/qute_note/utility/DesktopServices.h
    headers/qute_note/utility/EncryptionManager.h
    headers/qute_note/utility/EventLoopWithExitStatus.h
    headers/qute_note/utility/FileIOThreadWorker.h
    headers/qute_note/utility/Linkage.h
    headers/qute_note/utility/SysInfo.h
    headers/qute_note/utility/Printable.h
    headers/qute_note/utility/QuteNoteApplication.h
    headers/qute_note/utility/QuteNoteCheckPtr.h
    headers/qute_note/utility/Qt4Helper.h
    headers/qute_note/utility/Utility.h)

set(PUBLIC_HEADERS ${NOTE_EDITOR_HEADERS})
list(APPEND PUBLIC_HEADERS ${TYPES_HEADERS})
list(APPEND PUBLIC_HEADERS ${ENML_HEADERS})
list(APPEND PUBLIC_HEADERS ${LOCAL_STORAGE_HEADERS})
list(APPEND PUBLIC_HEADERS ${SYNCHRONIZATION_HEADERS})
list(APPEND PUBLIC_HEADERS ${LOGGING_HEADERS})
list(APPEND PUBLIC_HEADERS ${EXCEPTION_HEADERS})
list(APPEND PUBLIC_HEADERS ${UTILITY_HEADERS})

set(PRIVATE_HEADERS
    src/local_storage/Transaction.h
    src/note_editor/NoteEditorPage.h
    src/note_editor/NoteEditor_p.h
    src/note_editor/NoteEditorPluginFactory_p.h
    src/note_editor/ResourceFileStorageManager_p.h
    src/types/data/DataElementWithShortcutData.h
    src/types/data/LocalStorageDataElementData.h
    src/types/data/LinkedNotebookData.h
    src/types/data/NoteStoreDataElementData.h
    src/types/data/NoteData.h
    src/types/data/NotebookData.h
    src/types/data/ResourceWrapperData.h
    src/types/data/SharedNotebookWrapperData.h
    src/types/data/TagData.h
    src/types/data/SavedSearchData.h
    src/types/data/UserWrapperData.h
    src/enml/ENMLConverter_p.h
    src/local_storage/LocalStorageCacheManager_p.h
    src/local_storage/LocalStorageManager_p.h
    src/local_storage/NoteSearchQuery_p.h
    src/synchronization/NoteStore.h
    src/synchronization/RemoteToLocalSynchronizationManager.h
    src/synchronization/SendLocalChangesManager.h
    src/synchronization/SynchronizationManager_p.h
    src/utility/EncryptionManager_p.h
    src/utility/FileIOThreadWorker_p.h)

set(${PROJECT_NAME}_HEADERS ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})

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
    src/note_editor/ResourceFileStorageManager.cpp
    src/note_editor/ResourceFileStorageManager_p.cpp
    src/types/ISharedNotebook.cpp
    src/types/IResource.cpp
    src/types/IUser.cpp
    src/types/LinkedNotebook.cpp
    src/types/Note.cpp
    src/types/Notebook.cpp
    src/types/ResourceWrapper.cpp
    src/types/ResourceAdapter.cpp
    src/types/SavedSearch.cpp
    src/types/SharedNotebookAdapter.cpp
    src/types/SharedNotebookWrapper.cpp
    src/types/Tag.cpp
    src/types/UserAdapter.cpp
    src/types/UserWrapper.cpp
    src/types/RegisterMetatypes.cpp
    src/types/data/DataElementWithShortcutData.cpp
    src/types/data/LocalStorageDataElementData.cpp
    src/types/data/LinkedNotebookData.cpp
    src/types/data/NoteStoreDataElementData.cpp
    src/types/data/NoteData.cpp
    src/types/data/NotebookData.cpp
    src/types/data/ResourceWrapperData.cpp
    src/types/data/SharedNotebookWrapperData.cpp
    src/types/data/TagData.cpp
    src/types/data/SavedSearchData.cpp
    src/types/data/UserWrapperData.cpp
    src/enml/ENMLConverter.cpp
    src/enml/ENMLConverter_p.cpp
    src/enml/HTMLCleaner.cpp
    src/local_storage/ILocalStorageCacheExpiryChecker.cpp
    src/local_storage/DefaultLocalStorageCacheExpiryChecker.cpp
    src/local_storage/LocalStorageManager.cpp
    src/local_storage/LocalStorageManager_p.cpp
    src/local_storage/LocalStorageCacheManager.cpp
    src/local_storage/LocalStorageCacheManager_p.cpp
    src/local_storage/LocalStorageManagerThreadWorker.cpp
    src/local_storage/NoteSearchQuery.cpp
    src/local_storage/NoteSearchQuery_p.cpp
    src/local_storage/Transaction.cpp
    src/synchronization/NoteStore.cpp
    src/synchronization/SynchronizationManager.cpp
    src/synchronization/SynchronizationManager_p.cpp
    src/synchronization/RemoteToLocalSynchronizationManager.cpp
    src/synchronization/SendLocalChangesManager.cpp
    src/exception/ApplicationSettingsInitializationException.cpp
    src/exception/EmptyDataElementException.cpp
    src/exception/DatabaseOpeningException.cpp
    src/exception/DatabaseSqlErrorException.cpp
    src/exception/NoteEditorPluginInitializationException.cpp
    src/exception/NullPtrException.cpp
    src/exception/LocalStorageCacheManagerException.cpp
    src/exception/LoggerInitializationException.cpp
    src/exception/ResourceAdapterAccessException.cpp
    src/exception/ResourceLocalFileStorageFolderNotFoundException.cpp
    src/exception/SharedNotebookAdapterAccessException.cpp
    src/exception/UserAdapterAccessException.cpp
    src/utility/ApplicationSettings.cpp
    src/utility/DesktopServices.cpp
    src/utility/EncryptionManager.cpp
    src/utility/EncryptionManager_p.cpp
    src/utility/EventLoopWithExitStatus.cpp
    src/utility/FileIOThreadWorker.cpp
    src/utility/FileIOThreadWorker_p.cpp
    src/utility/IQuteNoteException.cpp
    src/utility/Printable.cpp
    src/utility/QuteNoteApplication.cpp
    src/utility/Utility.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/windows/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/linux/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/darwin/SysInfo.cpp)
endif()

set(RESOURCES
    src/note_editor/icons/checkbox_icons/checkbox_icons.qrc
    src/note_editor/icons/encrypted_area_icons/encrypted_area_icons.qrc
    src/note_editor/icons/generic_resource_icons/generic_resource_icons.qrc
    src/note_editor/javascript/jquery/jquery.qrc
    src/note_editor/javascript/colResizable/colResizable.qrc
    src/note_editor/javascript/scripts/scripts.qrc
    src/enml/enml.qrc)

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
foreach(ITEM ${NOTE_EDITOR_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/note_editor)
endforeach()

foreach(ITEM ${TYPES_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/types)
endforeach()

foreach(ITEM ${ENML_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/enml)
endforeach()

foreach(ITEM ${LOCAL_STORAGE_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/local_storage)
endforeach()

foreach(ITEM ${SYNCHRONIZATION_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/synchronization)
endforeach()

foreach(ITEM ${LOGGING_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/logging)
endforeach()

foreach(ITEM ${EXCEPTION_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/exception)
endforeach()

foreach(ITEM ${UTILITY_HEADERS})
  install(FILES ${ITEM} DESTINATION include/qute_note/utility)
endforeach()
