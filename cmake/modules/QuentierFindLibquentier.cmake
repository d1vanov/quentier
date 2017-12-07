if(USE_QT5)
  find_package(Libquentier-qt5 QUIET REQUIRED)
else()
  find_package(Libquentier-qt4 QUIET REQUIRED)
endif()

include_directories(${LIBQUENTIER_INCLUDE_DIRS})

get_property(LIBQUENTIER_LIBRARY_LOCATION TARGET ${LIBQUENTIER_LIBRARIES} PROPERTY LOCATION)
message(STATUS "Found libquentier library: ${LIBQUENTIER_LIBRARY_LOCATION}")

get_filename_component(LIBQUENTIER_LIB_DIR "${LIBQUENTIER_LIBRARY_LOCATION}" PATH)
