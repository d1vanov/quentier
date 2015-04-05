cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(simplecrypt)

add_definitions("-DBUILD_LIBSIMPLECRYPT")

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Linkage.h)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
else()
  qt5_use_modules(${PROJECT_NAME} Core)
endif()

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

# install headers
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.h" DESTINATION include)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Linkage.h" DESTINATION include)
