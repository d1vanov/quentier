find_package(Libquentier-qt5 QUIET REQUIRED)

get_property(LIBQUENTIER_LIBRARY_LOCATION TARGET Libquentier::libqt5quentier PROPERTY LOCATION)
message(STATUS "Found libquentier library: ${LIBQUENTIER_LIBRARY_LOCATION}")

get_filename_component(LIBQUENTIER_LIB_DIR "${LIBQUENTIER_LIBRARY_LOCATION}" PATH)
