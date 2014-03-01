# - Try to find Log4cplus
# Once done this will define
# LOG4CPLUS_FOUND - System has Log4cplus
# LOG4CPLUS_INCLUDE_DIRS - The Log4cplus include directories
# LOG4CPLUS_LIBRARIES - The libraries needed to use Log4cplus


find_path(LOG4CPLUS_INCLUDE_DIR log4cplus/config.hxx
          PATH_SUFFIXES log4cplus )

find_library(LOG4CPLUS_LIBRARY NAMES log4cplus
             PATHS /usr)

set(LOG4CPLUS_LIBRARIES ${LOG4CPLUS_LIBRARY} )
set(LOG4CPLUS_INCLUDE_DIRS ${LOG4CPLUS_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Log4cplus DEFAULT_MSG
                                  LOG4CPLUS_LIBRARY LOG4CPLUS_INCLUDE_DIR)

mark_as_advanced(LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY )
