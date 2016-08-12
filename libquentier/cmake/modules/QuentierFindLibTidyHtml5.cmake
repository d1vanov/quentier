function(FIND_TIDY_HTML5)
  find_path(TIDY_HTML5_INCLUDE_PATH tidy.h PATHS /usr/include)
  if(NOT TIDY_HTML5_INCLUDE_PATH)
    return()
  endif()

  if(NOT EXISTS ${TIDY_HTML5_INCLUDE_PATH}/tidyenum.h)
    return()
  endif()

  if(NOT EXISTS ${TIDY_HTML5_INCLUDE_PATH}/tidyplatform.h)
    return()
  endif()

  find_library(TIDY_HTML5_LIB
               NAMES libtidy5.so libtidy5.dylib libtidy5.a libtidy5.dll.a libtidy5.lib libtidy5s.lib
                     tidy5.so tidy5.dylib tidy5.a tidy5.dll.a tidy5.lib tidy5s.lib
                     libtidy.so libtidy.dylib libtidy.a libtidy.dll.a libtidy.lib libtidys.lib
                     tidy.so tidy.dylib tidy.a tidy.dll.a tidy.lib tidys.lib)
  if(NOT TIDY_HTML5_LIB)
    return()
  endif()

  set(LIB_TIDY_FOUND TRUE PARENT_SCOPE)
  set(LIB_TIDY_INCLUDE_DIRS ${TIDY_HTML5_INCLUDE_PATH} PARENT_SCOPE)
  set(LIB_TIDY_LIBRARIES ${TIDY_HTML5_LIB} PARENT_SCOPE)
endfunction()

FIND_TIDY_HTML5()

if(NOT LIB_TIDY_FOUND)
  message(FATAL_ERROR "Can't find tidy-html5 library and/or development headers")
endif()

message(STATUS "Found tidy-html5 library: ${LIB_TIDY_LIBRARIES}")
include_directories(${LIB_TIDY_INCLUDE_DIRS})
