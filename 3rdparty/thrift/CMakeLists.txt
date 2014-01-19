cmake_minimum_required(VERSION 2.8)
project(thrift)

add_definitions("-DBUILD_LIBTHRIFT")

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

include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/src ${SYSTEM}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${QT_INCLUDES})
add_definitions("-DUSE_BOOST_THREAD -DVERSION=0.9")

set(HEADERS
    src/thrift/Linkage.h
    src/thrift/TProcessor.h
    src/thrift/TLogging.h
    src/thrift/Thrift.h
    src/thrift/TApplicationException.h
    src/thrift/transport/TVirtualTransport.h
    src/thrift/transport/TTransportException.h
    src/thrift/transport/TTransport.h
    src/thrift/transport/TSocket.h
    src/thrift/transport/TSSLSocket.h
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

set(${PROJECT_NAME}_SOURCES
    src/thrift/Thrift.cpp
    src/thrift/TProcessor.cpp
    src/thrift/TApplicationException.cpp
    src/thrift/transport/TTransportException.cpp
    src/thrift/transport/TSocket.cpp
    src/thrift/transport/TSSLSocket.cpp
    src/thrift/transport/TServerSocket.cpp
    src/thrift/transport/THttpTransport.cpp
    src/thrift/transport/THttpClient.cpp
    src/thrift/transport/TBufferTransports.cpp
    src/thrift/protocol/TBinaryProtocol.tcc
    src/thrift/protocol/TVirtualProtocol.cpp
    src/thrift/concurrency/Util.cpp
    src/thrift/concurrency/BoostThreadFactory.cpp
    src/thrift/concurrency/BoostMutex.cpp
    src/thrift/concurrency/BoostMonitor.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/config.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/force_inc.h")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/GetTimeOfDay.h")
    list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/GetTimeOfDay.cpp")
  endif()
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/Operators.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/SocketPair.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/SocketPair.cpp")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/StdAfx.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/StdAfx.cpp")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/TargetVersion.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/TWinsockSingleton.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/TWinsockSingleton.cpp")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/WinFcntl.h")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/WinFcntl.cpp")
  list(APPEND ${PROJECT_NAME}_SOURCES "src/thrift/windows/tr1/functional")
endif()

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

# winsock2
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
  endif()

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
endif()

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "src/thrift")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "src/thrift/concurrency")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "src/thrift/protocol")
list(APPEND ${PROJECT_NAME}_INCLUDE_DIRS "src/thrift/transport")
