cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(QsLog)

set(CMAKE_AUTOMOC ON)

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS
    QsLog.h
    QsLogLevel.h
    QsLogDest.h
    QsLogDisableForThisFile.h
    QsLogDestFile.h
    QsLogDestFunctor.h
    QsLogDestConsole.h)
prepend_path(HEADERS "${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES
    QsLog.cpp
    QsLogDest.cpp
    QsLogDestFile.cpp
    QsLogDestFunctor.cpp
    QsLogDestConsole.cpp)
prepend_path(SOURCES "${SOURCES}" ${CMAKE_CURRENT_SOURCE_DIR})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows") 
  add_definitions("-DQSLOG_IS_SHARED_LIBRARY")
endif()

add_definitions("-DQS_LOG_LINE_NUMBERS -DQS_LOG_SEPARATE_THREAD -DQSLOG_IS_SHARED_LIBRARY")

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
else()
  qt5_use_modules(${PROJECT_NAME} Core)
endif()


if(USE_QT5)
  qt5_use_modules(${PROJECT_NAME} Core)
endif()

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
