set(PROJECTNAME QsLogUnitTest)
project(${PROJECTNAME})

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

include_directories(${QsLogUnitTest_SOURCE_DIR} ${QsLog_SOURCE_DIR})

if(QS_LOG_IS_SHARED_LIBRARY)
	# Ugh, ugly hack
	add_definitions(-UQSLOG_IS_SHARED_LIBRARY)
	add_definitions(-DQSLOG_IS_SHARED_LIBRARY_IMPORT)
endif()

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")

set(HEADER_FILES
	TestLog.h
	QtTestUtil/QtTestUtil.h
	QtTestUtil/TestRegistration.h
	QtTestUtil/TestRegistry.h
	)

set(SOURCE_FILES
	TestLog.cpp
	QtTestUtil/SimpleChecker.cpp
	QtTestUtil/TestRegistry.cpp
	)

add_executable(${PROJECTNAME} ${SOURCE_FILES} ${HEADER_FILES})

if(NOT USE_QT5)
    target_link_libraries(${PROJECTNAME} QsLog ${QT_LIBRARIES})
else()
    target_link_libraries(${PROJECTNAME} QsLog Qt5::Core Qt5::Widgets Qt5::Test)
endif()
