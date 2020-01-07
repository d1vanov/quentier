find_package(Qt5Core 5.5 REQUIRED)
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

list(REMOVE_DUPLICATES QT_INCLUDES)
list(REMOVE_DUPLICATES QT_LIBRARIES)
list(REMOVE_DUPLICATES QT_DEFINITIONS)

include_directories(SYSTEM "${QT_INCLUDES}")
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")
