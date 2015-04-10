cmake_minimum_required(VERSION 2.6)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

if(USE_QT5)
  message(STATUS "Building with Qt5's native mimetypes database")
  return()
endif()

set(CMAKE_AUTOMOC ON)

project(mimetypes-qt4)

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/QtMimeTypes)

add_definitions("-DBUILD_QT4_MIMETYPES")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/QtMimeTypes
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes/mime
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/mimetypes/inqt5)

# shared library
aux_source_directory(src/mimetypes MIMETYPES_SOURCES)
aux_source_directory(src/mimetypes/mime MIME_SOURCES)

set(SOURCES ${MIMETYPES_SOURCES} ${MIME_SOURCES})
prepend_path(SOURCES "${SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_win.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_mac.cpp)
else()
  list(APPEND SOURCES src/mimetypes/inqt5/qstandardpaths_unix.cpp)
endif()

set(RESOURCES src/mimetypes/mimetypes.qrc)
qt4_add_resources(${PROJECT_NAME}_RESOURCES_RCC ${RESOURCES})

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${${PROJECT_NAME}_RESOURCES_RCC})
target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES}) 

if(MSVC)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/Zc:wchar_t- ${SHARED_FLAGS} -DQT_BUILD_INTERNAL -DQT_MAKEDLL")
elseif(MINGW)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_MAKEDLL ${SHARED_FLAGS}")
endif()

if(APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation -framework Carbon")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL QMIME_LIBRARY)

# qmimetype test
aux_source_directory(tests/auto/qmimetype test_qmimetype_${PROJECT_NAME}_SOURCES)
add_executable(test_qmimetype_${PROJECT_NAME} ${test_qmimetype_${PROJECT_NAME}_SOURCES})
target_link_libraries(test_qmimetype_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
add_test(test_qmimetype_${PROJECT_NAME} test_qmimetype_${PROJECT_NAME})

# qmimedatabase-xml test
add_definitions("-DCORE_SOURCES=\"${CMAKE_CURRENT_SOURCE_DIR}/src/\"")
add_definitions("-DSRCDIR=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/auto/qmimedatabase/\"")
aux_source_directory(tests/auto/qmimedatabase/qmimedatabase-xml test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES)
list(APPEND test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES tests/auto/qmimedatabase/tst_qmimedatabase.h)
add_executable(test_qmimedatabase_xml_${PROJECT_NAME} ${test_qmimedatabase_xml_${PROJECT_NAME}_SOURCES})
target_link_libraries(test_qmimedatabase_xml_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
add_test(test_qmimedatabase_xml_${PROJECT_NAME} test_qmimedatabase_xml_${PROJECT_NAME}) 

if(MSVC OR MINGW)
  set_target_properties(test_qmimetype_${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_DLL")
  set_target_properties(test_qmimedatabase_xml_${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-DQT_BUILD_INTERNAL -DQT_DLL")
endif()

# qmimedatabase-cache test
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  aux_source_directory(tests/auto/qmimedatabase/qmimedatabase-cache test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES)
  list(APPEND test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES tests/auto/qmimedatabase/tst_qmimedatabase.h)
  add_executable(test_qmimedatabase_cache_${PROJECT_NAME} ${test_qmimedatabase_cache_${PROJECT_NAME}_SOURCES})
  target_link_libraries(test_qmimedatabase_cache_${PROJECT_NAME} ${PROJECT_NAME} ${QT_LIBRARIES})
  add_test(test_qmimedatabase_cache_${PROJECT_NAME} test_qmimedatabase_cache_${PROJECT_NAME})
endif()

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/QtMimeTypes/QMimeDatabase ${CMAKE_CURRENT_SOURCE_DIR}/include/QtMimeTypes/QMimeType")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()

