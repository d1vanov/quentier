cmake_minimum_required(VERSION 2.8)
project(thrift)

if(CMAKE_COMPILER_IS_GNUCXX)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  message(STATUS "Using GNU C++ compiler, version ${GCC_VERSION}.")
  if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
    message(STATUS "Your compiler supports C++11 standard.")
    add_definitions("-std=gnu++11")
  elseif(GCC_VERSION VERSION_GREATER 4.3 OR GCC_VERSION VERSION_EQUAL 4.3)
    message(WARNING "Your compiler is known to support C++11 standard only partially. If you get any errors update to a compiler which fully supports C++11.")
    add_definitions("-std=gnu++0x")
  else()
    message(FATAL_ERROR "Your compiler does not support C++11 standard. You need a GNU C++ compiler with version higher than 4.3.")   
  endif()
  add_definitions("-Wno-uninitialized")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE CLANG_VERSION)
  message(STATUS "Using LLVM/Clang C++ compiler, version info: ${CLANG_VERSION}")
  message(STATUS "You may have issues with compilation if your compiler is too old to support the C++11 features used in the project. In that case update your compiler. ")
  add_definitions("-std=c++11 -Wno-uninitialized")
  find_library(LIBCPP NAMES libc++.so libc++.so.1.0 libc++.dylib OPTIONAL)
  if(LIBCPP)
    message(STATUS "Using native clang C++ standard library: ${LIBCPP}")
    add_definitions("-stdlib=libc++")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC11")
  message(STATUS "Using MSVC C++ compiler, version 17")
  if(${CMAKE_MINOR_VERSION} EQUAL 8)
     if(${CMAKE_PATCH_VERSION} EQUAL 11 OR
        ${CMAKE_PATCH_VERSION} GREATER 11)
       cmake_policy(SET CMP0020 NEW)
	 endif()
  endif()
  add_definitions("-DNOMINMAX")
  set(USE_MSVC17 "Yes")
else()
  message(FATAL_ERROR "Unsupported C++ compiler")
endif(CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_AUTOMOC ON)

if(USE_MSVC17)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Network REQUIRED)
else()
  find_package(Qt4 COMPONENTS QTCORE QTNETWORK)
  include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
  include(${QT_USE_FILE})
  add_definitions(${QT_DEFINITIONS})
endif()

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

find_library(EVENT_LIBRARY NAMES libevent.so libevent.dylib libevent.dll event.dll libevent.lib REQUIRED)
message(STATUS "Event library found: ${EVENT_LIBRARY}")

find_package(Boost COMPONENTS thread system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${QT_INCLUDES})
add_definitions("-DUSE_BOOST_THREAD -DVERSION=0.9")

set(HEADERS
    src/thrift/TReflectionLocal.h
    src/thrift/TProcessor.h
    src/thrift/TLogging.h
    src/thrift/Thrift.h
    src/thrift/TDispatchProcessor.h
    src/thrift/TApplicationException.h
    src/thrift/transport/TZlibTransport.h
    src/thrift/transport/TVirtualTransport.h
    src/thrift/transport/TTransportUtils.h
    src/thrift/transport/TTransportException.h
    src/thrift/transport/TTransport.h
    src/thrift/transport/TSSLSocket.h
    src/thrift/transport/TSSLServerSocket.h
    src/thrift/transport/TSocketPool.h
    src/thrift/transport/TSocket.h
    src/thrift/transport/TSimpleFileTransport.h
    src/thrift/transport/TShortReadTransport.h
    src/thrift/transport/TServerTransport.h
    src/thrift/transport/TServerSocket.h
    src/thrift/transport/TPipeServer.h
    src/thrift/transport/TPipe.h
    src/thrift/transport/THttpTransport.h
    src/thrift/transport/THttpServer.h
    src/thrift/transport/THttpClient.h
    src/thrift/transport/TFileTransport.h
    src/thrift/transport/TFDTransport.h
    src/thrift/transport/TBufferTransports.h
    src/thrift/server/TThreadPoolServer.h
    src/thrift/server/TThreadedServer.h
    src/thrift/server/TSimpleServer.h
    src/thrift/server/TServer.h
    src/thrift/server/TNonblockingServer.h
    src/thrift/qt/TQTcpServer.h
    src/thrift/qt/TQIODeviceTransport.h
    src/thrift/protocol/TVirtualProtocol.h
    src/thrift/protocol/TProtocolTap.h
    src/thrift/protocol/TProtocolException.h
    src/thrift/protocol/TProtocol.h
    src/thrift/protocol/TJSONProtocol.h
    src/thrift/protocol/TDenseProtocol.h
    src/thrift/protocol/TDebugProtocol.h
    src/thrift/protocol/TCompactProtocol.h
    src/thrift/protocol/TBinaryProtocol.h
    src/thrift/protocol/TBase64Utils.h
    src/thrift/processor/StatsProcessor.h
    src/thrift/processor/PeekProcessor.h
    src/thrift/concurrency/Mutex.h
    src/thrift/concurrency/Monitor.h
    src/thrift/concurrency/PosixThreadFactory.h
    src/thrift/concurrency/Util.h
    src/thrift/concurrency/TimerManager.h
    src/thrift/concurrency/ThreadManager.h
    src/thrift/concurrency/Thread.h
    src/thrift/concurrency/PlatformThreadFactory.h
    src/thrift/concurrency/FunctionRunner.h
    src/thrift/concurrency/Exception.h
    src/thrift/concurrency/BoostThreadFactory.h
    src/thrift/async/TEvhttpServer.h
    src/thrift/async/TEvhttpClientChannel.h
    src/thrift/async/TAsyncProtocolProcessor.h
    src/thrift/async/TAsyncProcessor.h
    src/thrift/async/TAsyncDispatchProcessor.h
    src/thrift/async/TAsyncChannel.h
    src/thrift/async/TAsyncBufferProcessor.h)

set(SOURCES
    src/thrift/VirtualProfiling.cpp
    src/thrift/Thrift.cpp
    src/thrift/TApplicationException.cpp
    src/thrift/transport/TZlibTransport.cpp
    src/thrift/transport/TTransportUtils.cpp
    src/thrift/transport/TTransportException.cpp
    src/thrift/transport/TSSLServerSocket.cpp
    src/thrift/transport/TSocketPool.cpp
    src/thrift/transport/TSocket.cpp
    src/thrift/transport/TSimpleFileTransport.cpp
    src/thrift/transport/TServerSocket.cpp
    src/thrift/transport/TPipeServer.cpp
    src/thrift/transport/TPipe.cpp
    src/thrift/transport/THttpTransport.cpp
    src/thrift/transport/THttpServer.cpp
    src/thrift/transport/THttpClient.cpp
    src/thrift/transport/TFileTransport.cpp
    src/thrift/transport/TFDTransport.cpp
    src/thrift/transport/TBufferTransports.cpp
    src/thrift/server/TThreadPoolServer.cpp
    src/thrift/server/TThreadedServer.cpp
    src/thrift/server/TSimpleServer.cpp
    src/thrift/server/TNonblockingServer.cpp
    src/thrift/qt/TQTcpServer.cpp
    src/thrift/qt/TQIODeviceTransport.cpp
    src/thrift/protocol/TJSONProtocol.cpp
    src/thrift/protocol/TDenseProtocol.cpp
    src/thrift/protocol/TDebugProtocol.cpp
    src/thrift/protocol/TCompactProtocol.tcc
    src/thrift/protocol/TBinaryProtocol.tcc
    src/thrift/protocol/TBase64Utils.cpp
    src/thrift/processor/PeekProcessor.cpp
    src/thrift/concurrency/Util.cpp
    src/thrift/concurrency/TimerManager.cpp
    src/thrift/concurrency/ThreadManager.cpp
    src/thrift/concurrency/BoostThreadFactory.cpp
    src/thrift/concurrency/BoostMutex.cpp
    src/thrift/concurrency/BoostMonitor.cpp
    src/thrift/async/TEvhttpServer.cpp
    src/thrift/async/TEvhttpClientChannel.cpp
    src/thrift/async/TAsyncProtocolProcessor.cpp
    src/thrift/async/TAsyncChannel.cpp)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(SOURCES "${SOURCES} 
      src/thrift/transport/TSSLSocket.cpp
      src/thrift/server/TServer.cpp")
endif()
	
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${EVENT_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})

if(USE_MSVC17)
  target_link_libraries(${PROJECT_NAME} ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES})
else()
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
endif()
