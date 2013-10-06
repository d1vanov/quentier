cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
project(kqoauth)

add_definitions("-DKQOAUTH")

set(${PROJECT_NAME}_HEADERS
    src/kqoauthauthreplyserver.h
    src/kqoauthauthreplyserver_p.h
    src/kqoauthmanager.h
    src/kqoauthrequest.h
    src/kqoauthrequest_xauth.h
    src/kqoauthglobals.h
    src/kqoauthrequest_1.h
    src/kqoauthutils.h
    src/kqoauthrequest_xauth_p.h
    src/kqoauthrequest_p.h
    src/kqoauthmanager_p.h)

set(${PROJECT_NAME}_SOURCES
    src/kqoauthauthreplyserver.cpp
    src/kqoauthmanager.cpp
    src/kqoauthrequest.cpp
    src/kqoauthrequest_1.cpp
    src/kqoauthrequest_xauth.cpp
    src/kqoauthutils.cpp)

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

