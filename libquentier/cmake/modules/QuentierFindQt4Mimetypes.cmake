if(NOT USE_QT5)
  find_package(qt4-mimetypes REQUIRED)
  include_directories(${QT4-MIMETYPES_INCLUDE_DIRS}/qt4-mimetypes)
endif()
