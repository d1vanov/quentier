set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# require log4cplus
find_package(log4cplus REQUIRED)
if(LOG4CPLUS_FOUND)
  include_directories(${LOG4CPLUS_INCLUDE_DIRS})
endif()
