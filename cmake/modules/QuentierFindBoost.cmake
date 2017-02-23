find_package(Boost 1.38.0 REQUIRED COMPONENTS program_options)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()
