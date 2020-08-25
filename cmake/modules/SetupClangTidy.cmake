if(NOT CLANG_TIDY_BINARY)
  find_program(CLANG_TIDY_BINARY clang-tidy)
endif()

if(CLANG_TIDY_BINARY)
  execute_process(COMMAND "${CLANG_TIDY_BINARY}" "--version"
    OUTPUT_VARIABLE CLANG_TIDY_BINARY_VERSION)
  string(STRIP ${CLANG_TIDY_BINARY_VERSION} CLANG_TIDY_BINARY_VERSION)
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_BINARY_VERSION}")

  function(build_clang_tidy_target WITH_AUTO_FIX)
    if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      string(APPEND CLANG_TIDY_SCRIPT "#!/bin/sh\n")
    endif()

    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      set(NEWLINE "\r\n")
    else()
      set(NEWLINE "\n")
    endif()

    foreach(SOURCE IN LISTS QUENTIER_ALL_HEADERS QUENTIER_ALL_SOURCES)
      string(APPEND CLANG_TIDY_SCRIPT "echo \"Running clang-tidy on file ${SOURCE}...\"")
      string(APPEND CLANG_TIDY_SCRIPT "${NEWLINE}")

      string(APPEND CLANG_TIDY_SCRIPT "${CLANG_TIDY_BINARY} ${SOURCE}")
      string(APPEND CLANG_TIDY_SCRIPT " -p ${PROJECT_BINARY_DIR} -quiet")
      if(WITH_AUTO_FIX)
        string(APPEND CLANG_TIDY_SCRIPT " -fix --format-style=file")
      endif()

      string(APPEND CLANG_TIDY_SCRIPT "${NEWLINE}")
    endforeach()

    set(CLANG_TIDY_SCRIPT_NAME "clang_tidy")
    set(CLANG_TIDY_BUILD_TARGET_NAME "clang-tidy")
    if(WITH_AUTO_FIX)
      string(APPEND CLANG_TIDY_SCRIPT_NAME "_fix")
      string(APPEND CLANG_TIDY_BUILD_TARGET_NAME "-fix")
    endif()

    if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      file(WRITE "${PROJECT_BINARY_DIR}/${CLANG_TIDY_SCRIPT_NAME}.sh" ${CLANG_TIDY_SCRIPT})
      add_custom_target(${CLANG_TIDY_BUILD_TARGET_NAME}
        COMMAND "sh" "${PROJECT_BINARY_DIR}/${CLANG_TIDY_SCRIPT_NAME}.sh")
    else()
      file(WRITE "${PROJECT_BINARY_DIR}/${CLANG_TIDY_SCRIPT_NAME}.bat" ${CLANG_TIDY_SCRIPT})
      add_custom_target(${CLANG_TIDY_BUILD_TARGET_NAME}
        COMMAND "${PROJECT_BINARY_DIR}/${CLANG_TIDY_SCRIPT_NAME}.bat")
    endif()
  endfunction()

  build_clang_tidy_target(FALSE)
  build_clang_tidy_target(TRUE)
endif()
