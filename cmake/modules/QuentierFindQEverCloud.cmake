find_package(QEverCloud-qt5 QUIET REQUIRED)

include_directories(${QEVERCLOUD_INCLUDE_DIRS})

get_target_property(QEVERCLOUD_LIBRARY_LOCATION QEverCloud::qt5qevercloud LOCATION)
message(STATUS "Found QEverCloud library: ${QEVERCLOUD_LIBRARY_LOCATION}")

get_filename_component(QEVERCLOUD_LIB_DIR "${QEVERCLOUD_LIBRARY_LOCATION}" PATH)
