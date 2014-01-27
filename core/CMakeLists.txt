cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
project(qutenotecore)

include_directories(${THIRDPARTY_INCLUDE_DIRS})

set(CMAKE_AUTOMOC ON)

set(${PROJECT_NAME}_QT_HEADERS
    src/note_editor/HorizontalLineExtraData.h
    src/note_editor/MediaResourceTextObject.h
    src/note_editor/QuteNoteTextEdit.h
    src/note_editor/ResourceTextObject.h
    src/note_editor/ToDoCheckboxTextObject.h)

set(${PROJECT_NAME}_HEADERS
    src/client/EnWrappers.h
    src/client/enml/ENMLConverter.h
    src/client/local_storage/DatabaseOpeningException.h
    src/client/local_storage/DatabaseSqlErrorException.h
    src/client/local_storage/LocalStorageManager.h
    src/tools/EmptyDataElementException.h
    src/tools/IQuteNoteException.h
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
    src/note_editor/QuteNoteTextEdit.cpp
    src/note_editor/ResourceTextObject.cpp
    src/note_editor/ToDoCheckboxTextObject.cpp
    src/client/EnWrappers.cpp
    src/client/enml/ENMLConverter.cpp
    src/client/local_storage/DatabaseOpeningException.cpp
    src/client/local_storage/DatabaseSqlErrorException.cpp
    src/client/local_storage/LocalStorageManager.cpp
    src/tools/EmptyDataElementException.cpp
    src/tools/IQuteNoteException.cpp
    src/tools/Printable.cpp
    src/tools/QuteNoteNullPtrException.cpp
    src/tools/TypeWithError.cpp)

add_library(${PROJECT_NAME} SHARED
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

target_link_libraries(${PROJECT_NAME} ${EVERNOTE_SDK_LIB} ${THRIFT_LIB} ${Boost_LIBRARIES}
                      ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES})

# modifying sources list with absolute paths for cppcheck
prepend_path(${PROJECT_NAME}_SOURCES "${${PROJECT_NAME}_SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
