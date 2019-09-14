if(BUILD_WITH_QT4)
  return()
endif()

if(NOT CREATE_BUNDLE)
  return()
endif()

find_program(LINUXDEPLOYQT NAMES linuxdeployqt linuxdeployqt.AppImage PATHS ${Qt5Core_DIR}/../../../bin)
if(NOT LINUXDEPLOYQT)
  message(FATAL_ERROR "linuxdeployqt executable was not found!")
endif()

find_program(PATCHELF NAMES patchelf patchelf.AppImage)
if(NOT PATCHELF)
  message(FATAL_ERROR "patchelf executable was not found!")
endif()

find_program(APPIMAGETOOL NAMES appimagetool appimagetool.AppImage)
if(NOT APPIMAGETOOL)
  message(FATAL_ERROR "appimagetool executable was not found!")
endif()
