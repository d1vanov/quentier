cmake_minimum_required(VERSION 2.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
project(evernote_sdk)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../thrift/src"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../thrift/src/thrift")

set(${PROJECT_NAME}_HEADERS
    sdk/src/UserStore_types.h                                                                              
    sdk/src/UserStore_constants.h                                                                          
    sdk/src/UserStore.h                                                                                    
    sdk/src/Types_types.h                                                                                  
    sdk/src/Types_constants.h                                                                              
    sdk/src/NoteStore_types.h                                                                              
    sdk/src/NoteStore_constants.h                                                                          
    sdk/src/NoteStore.h                                                                                    
    sdk/src/Limits_types.h                                                                                 
    sdk/src/Limits_constants.h                                                                             
    sdk/src/Errors_types.h                                                                                 
    sdk/src/Errors_constants.h)
    
set(${PROJECT_NAME}_SOURCES
    sdk/src/UserStore_types.cpp                                                                            
    sdk/src/UserStore_constants.cpp                                                                        
    sdk/src/UserStore.cpp                                                                                  
    sdk/src/Types_types.cpp                                                                                
    sdk/src/Types_constants.cpp                                                                            
    sdk/src/NoteStore_types.cpp                                                                            
    sdk/src/NoteStore_constants.cpp                                                                        
    sdk/src/NoteStore.cpp                                                                                  
    sdk/src/Limits_types.cpp                                                                               
    sdk/src/Limits_constants.cpp                                                                           
    sdk/src/Errors_types.cpp                                                                               
    sdk/src/Errors_constants.cpp)
    
add_library(${PROJECT_NAME} STATIC 
            ${${PROJECT_NAME}_HEADERS} 
            ${${PROJECT_NAME}_SOURCES})

# include dirs for cppcheck
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/src)

# install headers
file(GLOB_RECURSE DEPLOY_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/*.h")
foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
  install(FILES ${ITEM} DESTINATION include)
endforeach()
