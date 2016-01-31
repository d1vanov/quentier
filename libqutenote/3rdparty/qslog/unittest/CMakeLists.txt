set(PROJECTNAME QsLogUnitTest)
project(${PROJECTNAME})

include_directories(${QsLogUnitTest_SOURCE_DIR} ${QsLog_SOURCE_DIR})

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
