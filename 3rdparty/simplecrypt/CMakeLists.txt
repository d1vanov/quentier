cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
project(simplecrypt)

add_definitions("-DBUILD_LIBSIMPLECRYPT")

set(${PROJECT_NAME}_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Linkage.h)

set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.cpp)

add_library(${PROJECT_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})

if(NOT USE_QT5)
  target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})
else()
  qt5_use_modules(${PROJECT_NAME} Core)
endif()

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")

# install shared library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# install headers
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Simplecrypt.h" DESTINATION include)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Linkage.h" DESTINATION include)
