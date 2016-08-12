if(USE_QT5)
  find_package(QEverCloud-qt5 REQUIRED)
else()
  find_package(QEverCloud-qt4 REQUIRED)
endif()

include_directories(${QEVERCLOUD_INCLUDE_DIRS})
