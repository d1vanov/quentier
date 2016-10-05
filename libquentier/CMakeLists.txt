cmake_minimum_required(VERSION 2.8)

include(QuentierCMakePolicies)
SET_POLICIES()

if("${CMAKE_MAJOR_VERSION}" GREATER "2")
  project(libquentier
          VERSION 0.2.0)
else()
  project(libquentier)
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/headers
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(QuentierAdditionalCompilerWarnings)

set(THIRDPARTY_LIBS ${QEVERCLOUD_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${LIB_TIDY_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${QTKEYCHAIN_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${OPENSSL_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${LIBXML2_LIBRARIES})
list(APPEND THIRDPARTY_LIBS ${HUNSPELL_LIBRARIES})
if(NOT USE_QT5)
  list(APPEND THIRDPARTY_LIBS ${QT4-MIMETYPES_LIBRARIES})
endif()
if(LIBCPP)
  list(APPEND THIRDPARTY_LIBS ${LIBCPP})
endif()

include_directories(headers)

set(NOTE_EDITOR_HEADERS
    headers/quentier/note_editor/DecryptedTextManager.h
    headers/quentier/note_editor/NoteEditor.h
    headers/quentier/note_editor/INoteEditorBackend.h
    headers/quentier/note_editor/ResourceFileStorageManager.h)

set(TYPES_HEADERS
    headers/quentier/types/Account.h
    headers/quentier/types/IFavoritableDataElement.h
    headers/quentier/types/ISharedNotebook.h
    headers/quentier/types/IResource.h
    headers/quentier/types/IUser.h
    headers/quentier/types/LinkedNotebook.h
    headers/quentier/types/ILocalStorageDataElement.h
    headers/quentier/types/Note.h
    headers/quentier/types/Notebook.h
    headers/quentier/types/INoteStoreDataElement.h
    headers/quentier/types/ResourceWrapper.h
    headers/quentier/types/ResourceAdapter.h
    headers/quentier/types/ResourceRecognitionIndexItem.h
    headers/quentier/types/ResourceRecognitionIndices.h
    headers/quentier/types/SavedSearch.h
    headers/quentier/types/SharedNote.h
    headers/quentier/types/SharedNotebookAdapter.h
    headers/quentier/types/SharedNotebookWrapper.h
    headers/quentier/types/Tag.h
    headers/quentier/types/UserAdapter.h
    headers/quentier/types/UserWrapper.h
    headers/quentier/types/RegisterMetatypes.h)

set(ENML_HEADERS
    headers/quentier/enml/ENMLConverter.h
    headers/quentier/enml/HTMLCleaner.h)

set(LOCAL_STORAGE_HEADERS
    headers/quentier/local_storage/ILocalStorageCacheExpiryChecker.h
    headers/quentier/local_storage/DefaultLocalStorageCacheExpiryChecker.h
    headers/quentier/local_storage/LocalStorageCacheManager.h
    headers/quentier/local_storage/LocalStorageManager.h
    headers/quentier/local_storage/LocalStorageManagerThreadWorker.h
    headers/quentier/local_storage/NoteSearchQuery.h)

set(SYNCHRONIZATION_HEADERS
    headers/quentier/synchronization/SynchronizationManager.h)

set(LOGGING_HEADERS
    headers/quentier/logging/QuentierLogger.h)

set(EXCEPTION_HEADERS
    headers/quentier/exception/ApplicationSettingsInitializationException.h
    headers/quentier/exception/DatabaseLockedException.h
    headers/quentier/exception/DatabaseLockFailedException.h
    headers/quentier/exception/DatabaseOpeningException.h
    headers/quentier/exception/DatabaseSqlErrorException.h
    headers/quentier/exception/EmptyDataElementException.h
    headers/quentier/exception/IQuentierException.h
    headers/quentier/exception/LocalStorageCacheManagerException.h
    headers/quentier/exception/LoggerInitializationException.h
    headers/quentier/exception/NoteEditorInitializationException.h
    headers/quentier/exception/NoteEditorPluginInitializationException.h
    headers/quentier/exception/NullPtrException.h
    headers/quentier/exception/ResourceAdapterAccessException.h
    headers/quentier/exception/ResourceLocalFileStorageFolderNotFoundException.h
    headers/quentier/exception/SharedNotebookAdapterAccessException.h
    headers/quentier/exception/UserAdapterAccessException.h)

set(UTILITY_HEADERS
    headers/quentier/utility/ApplicationSettings.h
    headers/quentier/utility/DesktopServices.h
    headers/quentier/utility/EncryptionManager.h
    headers/quentier/utility/EventLoopWithExitStatus.h
    headers/quentier/utility/FileIOThreadWorker.h
    headers/quentier/utility/FileSystemWatcher.h
    headers/quentier/utility/LimitedStack.h
    headers/quentier/utility/Linkage.h
    headers/quentier/utility/ShortcutManager.h
    headers/quentier/utility/StringUtils.h
    headers/quentier/utility/SysInfo.h
    headers/quentier/utility/Printable.h
    headers/quentier/utility/QNLocalizedString.h
    headers/quentier/utility/QuentierApplication.h
    headers/quentier/utility/QuentierCheckPtr.h
    headers/quentier/utility/QuentierUndoCommand.h
    headers/quentier/utility/Qt4Helper.h
    headers/quentier/utility/UidGenerator.h
    headers/quentier/utility/Utility.h)

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
    src/logging/QuentierLogger_p.h
    src/note_editor/GenericResourceImageManager.h
    src/note_editor/DecryptedTextManager_p.h
    src/note_editor/NoteEditorPage.h
    src/note_editor/NoteEditor_p.h
    src/note_editor/JavaScriptInOrderExecutor.h
    src/note_editor/ResourceFileStorageManager_p.h
    src/note_editor/ResourceInfo.h
    src/note_editor/SpellChecker.h
    src/note_editor/SpellCheckerDictionariesFinder.h
    src/note_editor/dialogs/AttachmentStoragePathConfigDialog.h
    src/note_editor/dialogs/EncryptionDialog.h
    src/note_editor/dialogs/DecryptionDialog.h
    src/note_editor/dialogs/EditHyperlinkDialog.h
    src/note_editor/dialogs/RenameResourceDialog.h
    src/note_editor/delegates/JsResultCallbackFunctor.hpp
    src/note_editor/delegates/HtmlCallbackFunctor.hpp
    src/note_editor/delegates/ParsePageScrollData.h
    src/note_editor/delegates/ImageResourceRotationDelegate.h
    src/note_editor/delegates/AddResourceDelegate.h
    src/note_editor/delegates/RemoveResourceDelegate.h
    src/note_editor/delegates/EncryptSelectedTextDelegate.h
    src/note_editor/delegates/DecryptEncryptedTextDelegate.h
    src/note_editor/delegates/AddHyperlinkToSelectedTextDelegate.h
    src/note_editor/delegates/EditHyperlinkDelegate.h
    src/note_editor/delegates/RemoveHyperlinkDelegate.h
    src/note_editor/delegates/RenameResourceDelegate.h
    src/note_editor/javascript_glue/ResourceInfoJavaScriptHandler.h
    src/note_editor/javascript_glue/ResizableImageJavaScriptHandler.h
    src/note_editor/javascript_glue/TextCursorPositionJavaScriptHandler.h
    src/note_editor/javascript_glue/ContextMenuEventJavaScriptHandler.h
    src/note_editor/javascript_glue/PageMutationHandler.h
    src/note_editor/javascript_glue/ToDoCheckboxOnClickHandler.h
    src/note_editor/javascript_glue/TableResizeJavaScriptHandler.h
    src/note_editor/javascript_glue/SpellCheckerDynamicHelper.h
    src/note_editor/undo_stack/INoteEditorUndoCommand.h
    src/note_editor/undo_stack/AddResourceUndoCommand.h
    src/note_editor/undo_stack/RemoveResourceUndoCommand.h
    src/note_editor/undo_stack/UpdateResourceUndoCommand.h
    src/note_editor/undo_stack/RenameResourceUndoCommand.h
    src/note_editor/undo_stack/NoteEditorContentEditUndoCommand.h
    src/note_editor/undo_stack/EncryptDecryptUndoCommandInfo.h
    src/note_editor/undo_stack/EncryptUndoCommand.h
    src/note_editor/undo_stack/DecryptUndoCommand.h
    src/note_editor/undo_stack/AddHyperlinkUndoCommand.h
    src/note_editor/undo_stack/RemoveHyperlinkUndoCommand.h
    src/note_editor/undo_stack/EditHyperlinkUndoCommand.h
    src/note_editor/undo_stack/ToDoCheckboxUndoCommand.h
    src/note_editor/undo_stack/ImageResourceRotationUndoCommand.h
    src/note_editor/undo_stack/ImageResizeUndoCommand.h
    src/note_editor/undo_stack/ReplaceUndoCommand.h
    src/note_editor/undo_stack/ReplaceAllUndoCommand.h
    src/note_editor/undo_stack/SpellCorrectionUndoCommand.h
    src/note_editor/undo_stack/SpellCheckIgnoreWordUndoCommand.h
    src/note_editor/undo_stack/SpellCheckAddToUserWordListUndoCommand.h
    src/note_editor/undo_stack/TableActionUndoCommand.h
    src/note_editor/undo_stack/HideDecryptedTextUndoCommand.h
    src/types/data/AccountData.h
    src/types/data/FavoritableDataElementData.h
    src/types/data/LocalStorageDataElementData.h
    src/types/data/LinkedNotebookData.h
    src/types/data/NoteStoreDataElementData.h
    src/types/data/NoteData.h
    src/types/data/NotebookData.h
    src/types/data/ResourceWrapperData.h
    src/types/data/SharedNotebookWrapperData.h
    src/types/data/TagData.h
    src/types/data/SharedNoteData.h
    src/types/data/SavedSearchData.h
    src/types/data/UserWrapperData.h
    src/types/data/ResourceRecognitionIndexItemData.h
    src/types/data/ResourceRecognitionIndicesData.h
    src/enml/ENMLConverter_p.h
    src/local_storage/LocalStorageCacheManager_p.h
    src/local_storage/LocalStorageManager_p.h
    src/local_storage/NoteSearchQuery_p.h
    src/synchronization/InkNoteImageDownloader.h
    src/synchronization/NoteStore.h
    src/synchronization/UserStore.h
    src/synchronization/NoteThumbnailDownloader.h
    src/synchronization/RemoteToLocalSynchronizationManager.h
    src/synchronization/SendLocalChangesManager.h
    src/synchronization/SynchronizationManager_p.h
    src/utility/EncryptionManager_p.h
    src/utility/FileIOThreadWorker_p.h
    src/utility/FileSystemWatcher_p.h
    src/utility/ShortcutManager_p.h
    src/utility/StringUtils_p.h)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND PRIVATE_HEADERS
       src/utility/unix/StackTrace.h)
endif()

if(NOT QUENTIER_USE_QT_WEB_ENGINE)
  list(APPEND PRIVATE_HEADERS
       src/note_editor/NoteEditorPluginFactory.h
       src/note_editor/INoteEditorResourcePlugin.h
       src/note_editor/EncryptedAreaPlugin.h
       src/note_editor/GenericResourceDisplayWidget.h)
else()
  list(APPEND PRIVATE_HEADERS
       src/note_editor/javascript_glue/EnCryptElementOnClickHandler.h
       src/note_editor/javascript_glue/GenericResourceOpenAndSaveButtonsOnClickHandler.h
       src/note_editor/javascript_glue/GenericResourceImageJavaScriptHandler.h
       src/note_editor/javascript_glue/HyperlinkClickJavaScriptHandler.h
       src/note_editor/WebSocketClientWrapper.h
       src/note_editor/WebSocketTransport.h)
endif()

set(${PROJECT_NAME}_HEADERS ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})

set(${PROJECT_NAME}_SOURCES
    src/logging/QuentierLogger.cpp
    src/logging/QuentierLogger_p.cpp
    src/note_editor/GenericResourceImageManager.cpp
    src/note_editor/DecryptedTextManager.cpp
    src/note_editor/DecryptedTextManager_p.cpp
    src/note_editor/NoteEditorPage.cpp
    src/note_editor/NoteEditor.cpp
    src/note_editor/INoteEditorBackend.cpp
    src/note_editor/NoteEditor_p.cpp
    src/note_editor/JavaScriptInOrderExecutor.cpp
    src/note_editor/ResourceFileStorageManager.cpp
    src/note_editor/ResourceFileStorageManager_p.cpp
    src/note_editor/ResourceInfo.cpp
    src/note_editor/SpellChecker.cpp
    src/note_editor/SpellCheckerDictionariesFinder.cpp
    src/note_editor/dialogs/AttachmentStoragePathConfigDialog.cpp
    src/note_editor/dialogs/EncryptionDialog.cpp
    src/note_editor/dialogs/DecryptionDialog.cpp
    src/note_editor/dialogs/EditHyperlinkDialog.cpp
    src/note_editor/dialogs/RenameResourceDialog.cpp
    src/note_editor/delegates/ParsePageScrollData.cpp
    src/note_editor/delegates/ImageResourceRotationDelegate.cpp
    src/note_editor/delegates/AddResourceDelegate.cpp
    src/note_editor/delegates/RemoveResourceDelegate.cpp
    src/note_editor/delegates/EncryptSelectedTextDelegate.cpp
    src/note_editor/delegates/DecryptEncryptedTextDelegate.cpp
    src/note_editor/delegates/AddHyperlinkToSelectedTextDelegate.cpp
    src/note_editor/delegates/EditHyperlinkDelegate.cpp
    src/note_editor/delegates/RemoveHyperlinkDelegate.cpp
    src/note_editor/delegates/RenameResourceDelegate.cpp
    src/note_editor/javascript_glue/ResourceInfoJavaScriptHandler.cpp
    src/note_editor/javascript_glue/ResizableImageJavaScriptHandler.cpp
    src/note_editor/javascript_glue/TextCursorPositionJavaScriptHandler.cpp
    src/note_editor/javascript_glue/ContextMenuEventJavaScriptHandler.cpp
    src/note_editor/javascript_glue/PageMutationHandler.cpp
    src/note_editor/javascript_glue/ToDoCheckboxOnClickHandler.cpp
    src/note_editor/javascript_glue/TableResizeJavaScriptHandler.cpp
    src/note_editor/javascript_glue/SpellCheckerDynamicHelper.cpp
    src/note_editor/undo_stack/INoteEditorUndoCommand.cpp
    src/note_editor/undo_stack/AddResourceUndoCommand.cpp
    src/note_editor/undo_stack/RemoveResourceUndoCommand.cpp
    src/note_editor/undo_stack/RenameResourceUndoCommand.cpp
    src/note_editor/undo_stack/UpdateResourceUndoCommand.cpp
    src/note_editor/undo_stack/NoteEditorContentEditUndoCommand.cpp
    src/note_editor/undo_stack/EncryptUndoCommand.cpp
    src/note_editor/undo_stack/DecryptUndoCommand.cpp
    src/note_editor/undo_stack/AddHyperlinkUndoCommand.cpp
    src/note_editor/undo_stack/RemoveHyperlinkUndoCommand.cpp
    src/note_editor/undo_stack/EditHyperlinkUndoCommand.cpp
    src/note_editor/undo_stack/ToDoCheckboxUndoCommand.cpp
    src/note_editor/undo_stack/ImageResourceRotationUndoCommand.cpp
    src/note_editor/undo_stack/ImageResizeUndoCommand.cpp
    src/note_editor/undo_stack/ReplaceUndoCommand.cpp
    src/note_editor/undo_stack/ReplaceAllUndoCommand.cpp
    src/note_editor/undo_stack/SpellCorrectionUndoCommand.cpp
    src/note_editor/undo_stack/SpellCheckIgnoreWordUndoCommand.cpp
    src/note_editor/undo_stack/SpellCheckAddToUserWordListUndoCommand.cpp
    src/note_editor/undo_stack/TableActionUndoCommand.cpp
    src/note_editor/undo_stack/HideDecryptedTextUndoCommand.cpp
    src/types/Account.cpp
    src/types/ISharedNotebook.cpp
    src/types/IResource.cpp
    src/types/IUser.cpp
    src/types/LinkedNotebook.cpp
    src/types/Note.cpp
    src/types/Notebook.cpp
    src/types/ResourceWrapper.cpp
    src/types/ResourceAdapter.cpp
    src/types/ResourceRecognitionIndexItem.cpp
    src/types/ResourceRecognitionIndices.cpp
    src/types/SavedSearch.cpp
    src/types/SharedNote.cpp
    src/types/SharedNotebookAdapter.cpp
    src/types/SharedNotebookWrapper.cpp
    src/types/Tag.cpp
    src/types/UserAdapter.cpp
    src/types/UserWrapper.cpp
    src/types/RegisterMetatypes.cpp
    src/types/data/AccountData.cpp
    src/types/data/FavoritableDataElementData.cpp
    src/types/data/LocalStorageDataElementData.cpp
    src/types/data/LinkedNotebookData.cpp
    src/types/data/NoteStoreDataElementData.cpp
    src/types/data/NoteData.cpp
    src/types/data/NotebookData.cpp
    src/types/data/ResourceWrapperData.cpp
    src/types/data/ResourceRecognitionIndexItemData.cpp
    src/types/data/ResourceRecognitionIndicesData.cpp
    src/types/data/SharedNotebookWrapperData.cpp
    src/types/data/TagData.cpp
    src/types/data/SavedSearchData.cpp
    src/types/data/SharedNoteData.cpp
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
    src/synchronization/InkNoteImageDownloader.cpp
    src/synchronization/NoteStore.cpp
    src/synchronization/UserStore.cpp
    src/synchronization/NoteThumbnailDownloader.cpp
    src/synchronization/SynchronizationManager.cpp
    src/synchronization/SynchronizationManager_p.cpp
    src/synchronization/RemoteToLocalSynchronizationManager.cpp
    src/synchronization/SendLocalChangesManager.cpp
    src/exception/ApplicationSettingsInitializationException.cpp
    src/exception/EmptyDataElementException.cpp
    src/exception/DatabaseLockedException.cpp
    src/exception/DatabaseLockFailedException.cpp
    src/exception/DatabaseOpeningException.cpp
    src/exception/DatabaseSqlErrorException.cpp
    src/exception/IQuentierException.cpp
    src/exception/NoteEditorInitializationException.cpp
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
    src/utility/FileSystemWatcher.cpp
    src/utility/FileSystemWatcher_p.cpp
    src/utility/Printable.cpp
    src/utility/QNLocalizedString.cpp
    src/utility/QuentierApplication.cpp
    src/utility/QuentierUndoCommand.cpp
    src/utility/ShortcutManager.cpp
    src/utility/ShortcutManager_p.cpp
    src/utility/StringUtils.cpp
    src/utility/StringUtils_p.cpp
    src/utility/Utility.cpp
    src/utility/UidGenerator.cpp)

if(NOT QUENTIER_USE_QT_WEB_ENGINE)
  list(APPEND ${PROJECT_NAME}_SOURCES
    src/note_editor/EncryptedAreaPlugin.cpp
    src/note_editor/GenericResourceDisplayWidget.cpp
    src/note_editor/INoteEditorResourcePlugin.cpp
    src/note_editor/NoteEditorPluginFactory.cpp)
else()
  list(APPEND ${PROJECT_NAME}_SOURCES
    src/note_editor/javascript_glue/EnCryptElementOnClickHandler.cpp
    src/note_editor/javascript_glue/GenericResourceOpenAndSaveButtonsOnClickHandler.cpp
    src/note_editor/javascript_glue/GenericResourceImageJavaScriptHandler.cpp
    src/note_editor/javascript_glue/HyperlinkClickJavaScriptHandler.cpp
    src/note_editor/WebSocketTransport.cpp
    src/note_editor/WebSocketClientWrapper.cpp)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/windows/SysInfo.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/unix/linux/SysInfo.cpp)
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/unix/StackTrace.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/unix/darwin/SysInfo.cpp)
  list(APPEND ${PROJECT_NAME}_SOURCES src/utility/unix/StackTrace.cpp)
endif()

set(RESOURCES
    src/note_editor/images/underline.qrc
    src/note_editor/css/css.qrc
    src/note_editor/images/icons/checkbox_icons/checkbox_icons.qrc
    src/note_editor/images/icons/encrypted_area_icons/encrypted_area_icons.qrc
    src/note_editor/images/icons/generic_resource_icons/generic_resource_icons.qrc
    src/note_editor/javascript/jquery/jquery.qrc
    src/note_editor/javascript/colResizable/colResizable.qrc
    src/note_editor/javascript/debounce/debounce.qrc
    src/note_editor/javascript/scripts/scripts.qrc
    src/note_editor/javascript/hilitor/hilitor.qrc
    src/note_editor/javascript/rangy/rangy.qrc
    src/enml/enml.qrc)

if(USE_QT5)
  qt5_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
else()
  qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})
endif()

set(FORMS
    src/note_editor/dialogs/AttachmentStoragePathConfigDialog.ui
    src/note_editor/dialogs/DecryptionDialog.ui
    src/note_editor/dialogs/EncryptionDialog.ui
    src/note_editor/dialogs/EditHyperlinkDialog.ui
    src/note_editor/dialogs/RenameResourceDialog.ui)

if(NOT QUENTIER_USE_QT_WEB_ENGINE)
  list(APPEND FORMS
       src/note_editor/EncryptedAreaPlugin.ui
       src/note_editor/GenericResourceDisplayWidget.ui)
endif()

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
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES} ${THIRDPARTY_LIBS})

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
    src/tests/ResourceRecognitionIndicesParsingTest.h
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
    src/tests/ResourceRecognitionIndicesParsingTest.cpp
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

target_link_libraries(test_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES} ${THIRDPARTY_LIBS})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS "BUILDING_QUENTIER_DLL=1")
endif()

# set Doxygen documentation properties
set(DOXY_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/headers ${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(DOXY_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(DOXY_RECURSIVE "YES")
set(DOXY_PROJECT_NAME ${PROJECT_NAME})
set(DOXY_PROJECT_NUMBER "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(DOXY_PROJECT_BRIEF "\"The library for rich desktop clients of Evernote service\"")
set(DOXY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc")
ADD_DOCUMENTATION(doc Doxyfile)

# modifying sources list with absolute paths for the static analyzer
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})
prepend_path(TEST_SOURCES "${TEST_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# collect the list of sources to be checked by static analyzer
set(${PROJECT_NAME}_CHECKABLE_SOURCES ${${PROJECT_NAME}_SOURCES})
list(APPEND ${PROJECT_NAME}_CHECKABLE_SOURCES ${TEST_SOURCES})

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
foreach(ITEM ${NOTE_EDITOR_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/note_editor)
endforeach()

foreach(ITEM ${TYPES_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/types)
endforeach()

foreach(ITEM ${ENML_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/enml)
endforeach()

foreach(ITEM ${LOCAL_STORAGE_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/local_storage)
endforeach()

foreach(ITEM ${SYNCHRONIZATION_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/synchronization)
endforeach()

foreach(ITEM ${LOGGING_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/logging)
endforeach()

foreach(ITEM ${EXCEPTION_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/exception)
endforeach()

foreach(ITEM ${UTILITY_HEADERS})
  install(FILES ${ITEM} DESTINATION include/quentier/utility)
endforeach()
