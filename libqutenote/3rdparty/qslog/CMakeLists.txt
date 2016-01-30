# CMake build support courtesy of Axel Gembe <axel@gembe.net>
# Modified for the purposes of libqutenote
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

project(QsLog)

# Add CMake modules
set(CMAKE_MODULE_PATH
    "${QsLog_SOURCE_DIR}/cmake"
    "${CMAKE_MODULE_PATH}"
)

include(QsLogConfigTargets)

# Add a _d to debug binaries
set(CMAKE_DEBUG_POSTFIX "_d")

set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell to always look for includes there
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Specify build paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${QsLog_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${QsLog_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${QsLog_BINARY_DIR}/bin)

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADER_FILES
	QsLog.h
	QsLogDest.h
	QsLogDestConsole.h
	QsLogDestFile.h
	QsLogDestFunctor.h
	QsLogDisableForThisFile.h
	QsLogLevel.h
	QsLogMessage.h
    )

set(SOURCE_FILES
	QsLog.cpp
	QsLogDest.cpp
	QsLogDestConsole.cpp
	QsLogDestFile.cpp
	QsLogDestFunctor.cpp
	QsLogMessage.cpp
	QsLogLevel.cpp
    )

if(APPLE)
	# Apple's compiler will not find standard includes like <thread> or <mutex> with 10.7 target otherwise
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

# Use 10.7 OSX SDK
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")

option(QS_LOG_LINE_NUMBERS "Automatically writes the file and line for each log message" ON)
if(QS_LOG_LINE_NUMBERS)
	add_definitions(-DQS_LOG_LINE_NUMBERS)
endif()

option(QS_LOG_DISABLE "Logging code is replaced with a no-op" OFF)
if(QS_LOG_DISABLE)
	add_definitions(-DQS_LOG_DISABLE)
endif()

option(QS_LOG_SEPARATE_THREAD "Messages are queued and written from a separate thread" OFF)
if(QS_LOG_SEPARATE_THREAD)
	add_definitions(-DQS_LOG_SEPARATE_THREAD)
endif()

option(QS_LOG_WIN_PRINTF_CONSOLE "Use fprintf instead of OutputDebugString on Windows" OFF)
if(QS_LOG_WIN_PRINTF_CONSOLE)
	add_definitions(-DQS_LOG_WIN_PRINTF_CONSOLE)
endif()

option(QS_LOG_IS_SHARED_LIBRARY "Build shared library" ON)
if(QS_LOG_IS_SHARED_LIBRARY)
	set(QS_LOG_LIBRARY_TYPE SHARED)
	add_definitions(-DQSLOG_IS_SHARED_LIBRARY)
else(QS_LOG_IS_SHARED_LIBRARY)
	set(QS_LOG_LIBRARY_TYPE STATIC)
endif(QS_LOG_IS_SHARED_LIBRARY)

option(QS_LOG_BUILD_WINDOW "Build log window, depends on Qt Widgets" ON)
if(QS_LOG_BUILD_WINDOW)
	list(APPEND HEADER_FILES QsLogDestModel.h QsLogWindow.h)
	list(APPEND SOURCE_FILES QsLogDestModel.cpp QsLogWindow.cpp)
	list(APPEND UI_FILES QsLogWindow.ui)
	list(APPEND RES_FILES QsLogWindow.qrc)

    if(NOT USE_QT5)
	    qt4_wrap_ui(UI_SOURCE_FILES ${UI_FILES})
	    qt4_add_resources(RES_SOURCE_FILES ${RES_FILES})
    else()
	    qt5_wrap_ui(UI_SOURCE_FILES ${UI_FILES})
	    qt5_add_resources(RES_SOURCE_FILES ${RES_FILES})
    endif()
    add_definitions(-DQS_LOG_WINDOW)
endif()

add_library(QsLog ${QS_LOG_LIBRARY_TYPE} ${HEADER_FILES} ${SOURCE_FILES} ${UI_SOURCE_FILES} ${RES_SOURCE_FILES})

if(NOT USE_QT5)
    target_link_libraries(QsLog ${QT_LIBRARIES})
else()
    target_link_libraries(QsLog Qt5::Core Qt5::Widgets)
endif()

install(FILES ${HEADER_FILES} DESTINATION include/QsLog)
QsLog_install_target(QsLog "")

option(QS_LOG_BUILD_TESTS "Build unit tests" ON)
if(QS_LOG_BUILD_TESTS)
	add_subdirectory(unittest)
    get_directory_property(UNITTEST_BINARY DIRECTORY unittest DEFINITION PROJECTNAME)
    add_test(${UNITTEST_BINARY} ${CMAKE_CURRENT_BINARY_DIR}/bin/${UNITTEST_BINARY})
endif()
