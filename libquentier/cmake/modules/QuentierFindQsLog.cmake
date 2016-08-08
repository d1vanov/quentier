find_path(QSLOG_INCLUDE_DIR QsLog/QsLog.h)
if(${QSLOG_INCLUDE_DIR} STREQUAL "QSLOG_INCLUDE_DIR-NOTFOUND")
  message(FATAL_ERROR "Can't find development headers for QsLog library")
else()
  include_directories(${QSLOG_INCLUDE_DIR})
endif()

find_library(QSLOG_LIBRARIES
             NAMES
             libQsLog.so libQsLog.dylib libQsLog.a libQsLog.dll.a libQsLog.lib)
if(${QSLOG_LIBRARIES} STREQUAL "QSLOG_LIBRARIES-NOTFOUND")
  message(FATAL_ERROR "Can't find QsLog library")
else()
  message(STATUS "Found QsLog library: ${QSLOG_LIBRARIES}")
endif()
