if(NOT CLANG_FORMAT_BINARY)
  find_program(CLANG_FORMAT_BINARY clang-format)
endif()

if(CLANG_FORMAT_BINARY)
  execute_process(COMMAND "${CLANG_FORMAT_BINARY}" "--version"
    OUTPUT_VARIABLE CLANG_FORMAT_BINARY_VERSION)
  string(STRIP ${CLANG_FORMAT_BINARY_VERSION} CLANG_FORMAT_BINARY_VERSION)
  message(STATUS "Found clang-format: ${CLANG_FORMAT_BINARY_VERSION}")

  if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    string(APPEND CLANG_FORMAT_SCRIPT "#!/bin/sh\n")
  endif()

  foreach(SOURCE IN LISTS QUENTIER_ALL_HEADERS QUENTIER_ALL_SOURCES)
    string(APPEND CLANG_FORMAT_SCRIPT "${CLANG_FORMAT_BINARY} -style=file -i ${SOURCE}")
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      string(APPEND CLANG_FORMAT_SCRIPT "\r\n")
    else()
      string(APPEND CLANG_FORMAT_SCRIPT "\n")
    endif()
  endforeach()

  if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    file(WRITE "${PROJECT_BINARY_DIR}/clang_format.sh" ${CLANG_FORMAT_SCRIPT})
    add_custom_target(clang-format
      COMMAND "sh" "${PROJECT_BINARY_DIR}/clang_format.sh")
  else()
    file(WRITE "${PROJECT_BINARY_DIR}/clang_format.bat" ${CLANG_FORMAT_SCRIPT})
    add_custom_target(clang-format
      COMMAND "${PROJECT_BINARY_DIR}/clang_format.bat")
  endif()
endif()
