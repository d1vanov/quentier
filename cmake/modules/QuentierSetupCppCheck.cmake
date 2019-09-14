if(NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")    # clang is not supported by cppcheck
  include(CppcheckTargets)

  ## using only source files for cppcheck as a first attempt
  set(SOURCE_FILE_REGEX "\\.cpp")

  MACRO(ADD_CPPCHECK_TEST FILE_TO_TEST)
    string( REGEX MATCH ${SOURCE_FILE_REGEX} is_source_file ${FILE_TO_TEST} )
    if(is_source_file)
      add_cppcheck_sources(${FILE_TO_TEST} ${FILE_TO_TEST} FAIL_ON_WARNINGS)
    endif(is_source_file)
  ENDMACRO()

  get_property(QUENTIER_CPPCHECKABLE_INCLUDE_DIRS GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_INCLUDE_DIRS)
  set(CPPCHECK_INCLUDEPATH_ARG ${QUENTIER_CPPCHECKABLE_INCLUDE_DIRS})

  get_property(QUENTIER_CPPCHECKABLE_SOURCES GLOBAL PROPERTY QUENTIER_CPPCHECKABLE_SOURCES)
  foreach(i ${QUENTIER_CPPCHECKABLE_SOURCES})
    add_cppcheck_test(${i})
  endforeach()
endif()
