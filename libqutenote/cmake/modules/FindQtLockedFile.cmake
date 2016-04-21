message(STATUS "Looking for system wide installation of QtLockedFile")
find_path(QT_LOCKED_FILE_INCLUDE_PATH QtLockedFile SILENT REQUIRED)
if(NOT QT_LOCKED_FILE_INCLUDE_PATH)
  message(FATAL_ERROR "Can't find development header QtLockedFile")
else()
  message(STATUS "Found development header for QtLockedFile in folder ${QT_LOCKED_FILE_INCLUDE_PATH}")
  find_library(QT_LOCKED_FILE_LIB
               NAMES libQtSolutions_LockedFile_head.1.0.dylib libQtSolutions_LockedFile_head.1.0.dll libQtSolutions_LockedFile_head.1.0.lib 
                     libQtSolutions_LockedFile_head.so libQtSolutions_LockedFile_head.so.1 libQtSolutions_LockedFile_head.so.1.0 
                     libQtSolutions_LockedFile_head.so.1.0.0 QtSolutions_LockedFile_head.1.0.dll QtSolutions_LockedFile_head.1.0.lib
                     libQt5Solutions_LockedFile_head.1.0.dylib libQt5Solutions_LockedFile_head.1.0.dll libQt5Solutions_LockedFile_head.1.0.lib
                     Qt5Solutions_LockedFile_head.1.0.dll Qt5Solutions_LockedFile_head.1.0.lib libQt5Solutions_LockedFile_head.so
                     libQt5Solutions_LockedFile_head.so.1 libQt5Solutions_LockedFile_head.so.1.0 libQt5Solutions_LockedFile_head.so.1.0.0
               SILENT REQUIRED)
  if(NOT QT_LOCKED_FILE_LIB)
    message(FATAL_ERROR "Can't find QtLockedFile library")
  else()
    message(STATUS "Found QtLockedFile library: ${QT_LOCKED_FILE_LIB}")
  endif()
endif()
            
