if(USE_QT5)
  find_package(QEverCloud-qt5 QUIET REQUIRED)
else()
  find_package(QEverCloud-qt4 QUIET REQUIRED)
endif()

include_directories(${QEVERCLOUD_INCLUDE_DIRS})

get_property(QEVERCLOUD_LIBRARY_LOCATION TARGET ${QEVERCLOUD_LIBRARIES} PROPERTY LOCATION)
message(STATUS "Found QEverCloud library: ${QEVERCLOUD_LIBRARY_LOCATION}")
