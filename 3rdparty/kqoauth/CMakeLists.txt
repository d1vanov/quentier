cmake_minimum_required(VERSION 2.8)
project(kqoauth)

add_definitions("-DKQOAUTH")

set(${PROJECT_NAME}_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthauthreplyserver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthauthreplyserver_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthmanager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_xauth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthglobals.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_1.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthutils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_xauth_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthmanager_p.h)

set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthauthreplyserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthrequest_xauth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kqoauthutils.cpp)

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
else()
  qt5_use_modules(${PROJECT_NAME} Core)
  qt5_use_modules(${PROJECT_NAME} Gui)
  qt5_use_modules(${PROJECT_NAME} Network)
  qt5_use_modules(${PROJECT_NAME} Webkit)
endif()

if(LIBCPP)
  target_link_libraries(${PROJECT_NAME} ${LIBCPP})
endif()

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
