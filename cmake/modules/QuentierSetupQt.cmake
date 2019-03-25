if(NOT USE_QT5)
  find_package(Qt4 COMPONENTS QTCORE OPTIONAL QUIET)
  if(NOT QT_QTCORE_LIBRARY)
    message(STATUS "Qt4's core was not found, trying to find Qt5's libraries")
    set(USE_QT5 1)
  else()
    message(STATUS "Found Qt4 installation, version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
  endif()
endif()

if(USE_QT5)
  find_package(Qt5Core REQUIRED)
  message(STATUS "Found Qt5 installation, version ${Qt5Core_VERSION}")

  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5LinguistTools REQUIRED)
  find_package(Qt5Network REQUIRED)
  find_package(Qt5PrintSupport REQUIRED)

  list(APPEND QT_INCLUDES
       ${Qt5Core_INCLUDE_DIRS}
       ${Qt5Gui_INCLUDE_DIRS}
       ${Qt5Widgets_INCLUDE_DIRS}
       ${Qt5LinguistTools_INCLUDE_DIRS}
       ${Qt5Network_INCLUDE_DIRS}
       ${Qt5PrintSupport_INCLUDE_DIRS})
  list(APPEND QT_LIBRARIES
       ${Qt5Core_LIBRARIES}
       ${Qt5Gui_LIBRARIES}
       ${Qt5Widgets_LIBRARIES}
       ${Qt5LinguistTools_LIBRARIES}
       ${Qt5Network_LIBRARIES}
       ${Qt5PrintSupport_LIBRARIES})
  list(APPEND QT_DEFINITIONS
       ${Qt5Core_DEFINITIONS}
       ${Qt5Gui_DEFINITIONS}
       ${Qt5Widgets_DEFINITIONS}
       ${Qt5LinguistTools_DEFINITIONS}
       ${Qt5Network_DEFINITIONS}
       ${Qt5PrintSupport_DEFINITIONS})

  macro(qt_add_translation)
    qt5_add_translation(${ARGN})
  endmacro()
  macro(qt_create_translation)
    qt5_create_translation(${ARGN})
  endmacro()
  macro(qt_add_resources)
    qt5_add_resources(${ARGN})
  endmacro()
  macro(qt_wrap_ui)
    qt5_wrap_ui(${ARGN})
  endmacro()
else()
  find_package(Qt4 COMPONENTS QTCORE QTGUI QTNETWORK QTWEBKIT REQUIRED)
  include(${QT_USE_FILE})

  # Workaround CMake > 3.0.2 bug with Qt4 libraries
  list(FIND QT_LIBRARIES "${QT_QTGUI_LIBRARY}" HasGui)
  if(HasGui EQUAL -1)
    list(APPEND QT_LIBRARIES ${QT_QTGUI_LIBRARY})
  endif()

  macro(qt_add_translation)
    qt4_add_translation(${ARGN})
  endmacro()
  macro(qt_create_translation)
    qt4_create_translation(${ARGN})
  endmacro()
  macro(qt_add_resources)
    qt4_add_resources(${ARGN})
  endmacro()
  macro(qt_wrap_ui)
    qt4_wrap_ui(${ARGN})
  endmacro()
endif()

list(REMOVE_DUPLICATES QT_INCLUDES)
list(REMOVE_DUPLICATES QT_LIBRARIES)
list(REMOVE_DUPLICATES QT_DEFINITIONS)

include_directories(SYSTEM "${QT_INCLUDES}")
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")
