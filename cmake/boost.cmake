# require boost
SET(Boost_USE_STATIC_LIBS OFF)
SET(Boost_USE_MULTITHREAD OFF)
#make sure that at least version 1.46.0 is used
SET(BOOST_MIN_VERSION "1.46.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS thread system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
