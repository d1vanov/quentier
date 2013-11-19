if(USE_QT5)
  find_package(Qt5Core REQUIRED)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Network REQUIRED)
  find_package(Qt5Webkit REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5WebkitWidgets REQUIRED)
  find_package(Qt5Xml REQUIRED)
  find_package(Qt5Sql REQUIRED)
  set(QT_INCLUDES "${Qt5Core_INCLUDES} ${Qt5Gui_INCLUDES} ${Qt5Network_INCLUDES}
                   ${Qt5Webkit_INCLUDES} ${Qt5Widgets_INCLUDES} ${Qt5WebkitWidgets_INCLUDES}
                   ${Qt5Xml_INCLUDES} ${Qt5Sql_INCLUDES}")
  set(QT_LIBRARIES "${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Network_LIBRARIES}
                    ${Qt5Webkit_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5WebkitWidgets_LIBRARIES}
                    ${Qt5Xml_LIBRARIES} ${Qt5Sql_LIBRARIES}")
  set(QT_DEFINITIONS "${Qt5Core_DEFINITIONS} ${Qt5Gui_DEFINITIONS} ${Qt5Network_DEFINITIONS}
                      ${Qt5Webkit_DEFINITIONS} ${Qt5Widgets_DEFINITIONS} ${Qt5WebkitWidgets_DEFINITIONS}
                      ${Qt5Xml_DEFINITIONS} ${Qt5Sql_DEFINITIONS}")
else()
  find_package(Qt4 COMPONENTS QTCORE QTGUI QTNETWORK QTWEBKIT QTXML QTSQL REQUIRED)
  include(${QT_USE_FILE})
endif()

include_directories(SYSTEM "${QT_INCLUDES} ${SYSTEM}")
add_definitions(${QT_DEFINITIONS})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR "ON")
