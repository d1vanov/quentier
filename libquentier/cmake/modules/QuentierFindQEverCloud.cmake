if(USE_QT5)
  find_package(QEverCloud-qt5 QUIET REQUIRED)
else()
  find_package(QEverCloud-qt4 QUIET REQUIRED)
endif()

include_directories(${QEVERCLOUD_INCLUDE_DIRS})

# Not sure why but sometimes CMake fails to recognize that ${QEVERCLOUD_LIBRARIES} is a target and issues a weird error message
if(TARGET ${QEVERCLOUD_LIBRARIES})
  get_property(QEVERCLOUD_LIBRARY_LOCATION TARGET ${QEVERCLOUD_LIBRARIES} PROPERTY LOCATION)
  message(STATUS "Found QEverCloud library: ${QEVERCLOUD_LIBRARY_LOCATION}")
else()
  message(STATUS "Found QEverCloud library")
endif()
