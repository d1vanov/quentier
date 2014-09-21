find_path(Boost_MULTI_INDEX_INCLUDE_PATH boost/multi_index_container.hpp REQUIRED)
if(Boost_MULTI_INDEX_INCLUDE_PATH_FOUND)
  include_directories(SYSTEM ${Boost_MULTI_INDEX_INCLUDE_PATH})
endif()
