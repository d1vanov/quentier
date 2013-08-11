cmake_minimum_required(VERSION 2.8)
project(thrift)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/compiler.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/qt.cmake)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  # Include files required by thrift
  find_path(NETINET_IN_INCLUDE_DIR netinet/in.h)
  if(NETINET_IN_INCLUDE_DIR)
    message(STATUS "Include file netinet/in.h found in ${NETINET_IN_INCLUDE_DIR}")
    add_definitions(-DHAVE_NETINET_IN_H)
  else()
    message(FATAL_ERROR "Include file netinet.h not found")
  endif()

  find_path(INTTYPES_INCLUDE_DIR inttypes.h)
  if(INTTYPES_INCLUDE_DIR)
    message(STATUS "Include file inttypes.h found in ${INTTYPES_INCLUDE_DIR}")
    add_definitions(-DHAVE_INTTYPES_H)
  else()
    message(FATAL_ERROR "Include file inttypes.h not found")
  endif()

  find_path(SYS_TIME_INCLUDE_DIR sys/time.h)
  if(SYS_TIME_INCLUDE_DIR)
    message(STATUS "Include file sys/time.h found in ${SYS_TIME_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_TIME_H)
  else()
    message(FATAL_ERROR "Include file sys/time.h not found")
  endif()
  
  find_path(NETDB_INCLUDE_DIR netdb.h)
  if(NETDB_INCLUDE_DIR)
    message(STATUS "Include file netdb.h found in ${NETDB_INCLUDE_DIR}")
    add_definitions(-DHAVE_NETDB_H)
  else()
    message(FATAL_ERROR "Include file netdb.h not found")
  endif()
  
  find_path(SYS_SOCKET_INCLUDE_DIR sys/socket.h)
  if(SYS_SOCKET_INCLUDE_DIR)
    message(STATUS "Include file sys/socket.h found in ${SYS_SOCKET_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_SOCKET_H)
  else()
    message(FATAL_ERROR "Include file sys/socket.h not found")
  endif()
  
  find_path(SYS_UN_INCLUDE_DIR sys/un.h)
  if(SYS_UN_INCLUDE_DIR)
    message(STATUS "Include file sys/un.h found in ${SYS_UN_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_UN_H)
  else()
    message(FATAL_ERROR "Include file sys/un.h not found")
  endif()
  
  find_path(SYS_POLL_INCLUDE_DIR sys/poll.h)
  if(SYS_POLL_INCLUDE_DIR)
    message(STATUS "Include file sys/poll.h found in ${SYS_POLL_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_POLL_H)
  else()
    message(FATAL_ERROR "Include file sys/poll.h not found")
  endif()

  find_path(ARPA_INET_INCLUDE_DIR arpa/inet.h)
  if(ARPA_INET_INCLUDE_DIR)
    message(STATUS "Include file arpa/inet.h found in ${ARPA_INET_INCLUDE_DIR}")
    add_definitions(-DHAVE_APRA_INET_H)
  else()
    message(FATAL_ERROR "Include file arpa/inet.h not found")
  endif()

  find_path(UNISTD_INCLUDE_DIR unistd.h)
  if(UNISTD_INCLUDE_DIR)
    message(STATUS "Include file unistd.h found in ${UNISTD_INCLUDE_DIR}")
    add_definitions(-DHAVE_UNISTD_H)
  else()
    message(FATAL_ERROR "Include file unitstd.h not found")
  endif()

  find_path(PTHREAD_INCLUDE_DIR pthread.h)
  if(PTHREAD_INCLUDE_DIR)
    message(STATUS "Include file pthread.h found in ${PTHREAD_INCLUDE_DIR}")
    add_definitions(-DHAVE_PTHREAD_H)
  else()
    message(FATAL_ERROR "Include file pthread.h not found")
  endif()

  find_path(STRINGS_INCLUDE_DIR strings.h)
  if(STRINGS_INCLUDE_DIR)
    message(STATUS "Include file strings.h found in ${STRINGS_INCLUDE_DIR}")
    add_definitions(-DHAVE_STRINGS_H)
  else()
    message(FATAL_ERROR "Include file strings.h not found")
  endif()

  find_path(SYS_STAT_INCLUDE_DIR sys/stat.h)
  if(SYS_STAT_INCLUDE_DIR)
    message(STATUS "Include file sys/stat.h found in ${SYS_STAT_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_STAT_H)
  else()
    message(FATAL_ERROR "Include file sys/stat.h not found")
  endif()

  find_path(SYS_RESOURCE_INCLUDE_DIR sys/resource.h)
  if(SYS_RESOURCE_INCLUDE_DIR)
    message(STATUS "Include file sys/resource.h found in ${SYS_RESOURCE_INCLUDE_DIR}")
    add_definitions(-DHAVE_SYS_RESOURCE_H)
  else()
    message(FATAL_ERROR "Include file sys/resource.h")
  endif()

  find_path(FCNTL_INCLUDE_DIR fcntl.h)
  if(FCNTL_INCLUDE_DIR)
    message(STATUS "Include file fcntl.h found in ${FCNTL_INCLUDE_DIR}")
    add_definitions(-DHAVE_FCNTL_H)
  else()
    message(FATAL_ERROR "Include file fcntl.h not found")
  endif()

  find_path(SCHED_INCLUDE_DIR sched.h)
  if(SCHED_INCLUDE_DIR)
    message(STATUS "Include file sched.h found in ${SCHED_INCLUDE_DIR}")
    add_definitions(-DHAVE_SCHED_H)
  else()
    message(FATAL_ERROR "Include file sched.h not found")
  endif()

  find_path(TIME_INCLUDE_DIR time.h)
  if(TIME_INCLUDE_DIR)
    message(STATUS "Include file time.h found in ${TIME_INCLUDE_DIR}")
    add_definitions(-DHAVE_CLOCK_GETTIME)
  else()
    message(FATAL_ERROR "Include file time.h not found")
  endif()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  find_path(EVENT_CONFIG_INCLUDE_DIR event2/event-config.h REQUIRED)
  include_directories(${EVENT_CONFIG_INCLUDE_DIR})
  message(STATUS "Include file event2/event-config.h found in ${EVENT_CONFIG_INCLUDE_DIR}")

  find_path(EVHTTP_INCLUDE_DIR evhttp.h REQUIRED)
  include_directories(${EVHTTP_INCLUDE_DIR})
  message(STATUS "Include file evhttp.h found in ${EVHTTP_INCLUDE_DIR}")

  find_path(EVENT_INCLUDE_DIR event2/event.h REQUIRED)
  include_directories(${EVENT_INCLUDE_DIR})
  include_directories("${EVENT_INCLUDE_DIR}/event2/")
  add_definitions("-D_EVENT_HAVE_UINT8_T")
  message(STATUS "Include file event.h found in ${EVENT_INCLUDE_DIR}/event2/")
else()
  find_path(EVENT_INCLUDE_DIR event.h REQUIRED)
  include_directories(${EVENT_INCLUDE_DIR})
  message(STATUS "Include file event.h found in ${EVENT_INCLUDE_DIR}")
endif()

find_package(Boost COMPONENTS thread system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/src ${SYSTEM}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${QT_INCLUDES})
add_definitions("-DUSE_BOOST_THREAD -DVERSION=0.9")

set(HEADERS
    src/thrift/TProcessor.h
    src/thrift/TLogging.h
    src/thrift/Thrift.h
    src/thrift/TApplicationException.h
    src/thrift/transport/TVirtualTransport.h
    src/thrift/transport/TTransportException.h
    src/thrift/transport/TTransport.h
    src/thrift/transport/TSSLSocket.h
    src/thrift/transport/TSocket.h
    src/thrift/transport/TServerSocket.h
    src/thrift/transport/TServerTransport.h
    src/thrift/transport/THttpTransport.h
    src/thrift/transport/THttpClient.h
    src/thrift/transport/TBufferTransports.h
    src/thrift/protocol/TVirtualProtocol.h
    src/thrift/protocol/TProtocolException.h
    src/thrift/protocol/TProtocol.h
    src/thrift/protocol/TBinaryProtocol.h
    src/thrift/concurrency/Mutex.h
    src/thrift/concurrency/Monitor.h
    src/thrift/concurrency/Util.h
    src/thrift/concurrency/Thread.h
    src/thrift/concurrency/Exception.h
    src/thrift/concurrency/BoostThreadFactory.h)

set(SOURCES
    src/thrift/Thrift.cpp
    src/thrift/TApplicationException.cpp
    src/thrift/transport/TTransportException.cpp
    src/thrift/transport/TSocket.cpp
    src/thrift/transport/TServerSocket.cpp
    src/thrift/transport/THttpTransport.cpp
    src/thrift/transport/THttpClient.cpp
    src/thrift/transport/TBufferTransports.cpp
    src/thrift/protocol/TBinaryProtocol.tcc
    src/thrift/concurrency/Util.cpp
    src/thrift/concurrency/BoostThreadFactory.cpp
    src/thrift/concurrency/BoostMutex.cpp
    src/thrift/concurrency/BoostMonitor.cpp)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND SOURCES  "src/thrift/transport/TSSLSocket.cpp")
endif()
	
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${QT_LIBRARIES})

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
endif()
