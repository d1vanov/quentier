if(NOT CREATE_BUNDLE)
  return()
endif()

find_program(LINUXDEPLOY NAMES linuxdeploy linuxdeploy-x86_64.AppImage linuxdeploy-i386.AppImage)
if(NOT LINUXDEPLOY)
  message(FATAL_ERROR "linuxdeploy executable was not found!")
endif()

find_program(LINUXDEPLOY_QT_PLUGIN NAMES linuxdeploy-plugin-qt linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-plugin-qt-i386.AppImage)
if(NOT LINUXDEPLOY_QT_PLUGIN)
  message(FATAL_ERROR "linuxdeploy-plugin-qt executable was not found!")
endif()

find_program(APPIMAGETOOL NAMES appimagetool appimagetool-x86_64.AppImage appimagetool-i686.AppImage)
if(NOT APPIMAGETOOL)
  message(FATAL_ERROR "appimagetool executable was not found!")
endif()
