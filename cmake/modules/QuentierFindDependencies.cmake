if(_QUENTIER_USE_QT6)
  find_package(Qt6 6.4 COMPONENTS Core Gui Widgets LinguistTools Network PrintSupport ${_QUENTIER_FIND_DEPS_ARGS})
  find_package(QEverCloud-qt6 ${_QUENTIER_FIND_DEPS_ARGS})
  find_package(Libquentier-qt6 ${_QUENTIER_FIND_DEPS_ARGS})
else()
  find_package(Qt5 5.12 COMPONENTS Core Gui Widgets LinguistTools Network PrintSupport ${_QUENTIER_FIND_DEPS_ARGS})
  find_package(QEverCloud-qt5 ${_QUENTIER_FIND_DEPS_ARGS})
  find_package(Libquentier-qt5 ${_QUENTIER_FIND_DEPS_ARGS})
endif()

find_package(Boost 1.71.0 ${_QUENTIER_FIND_DEPS_ARGS})

if(NOT APPLE)
  find_package(Breakpad)
  if(NOT BREAKPAD_FOUND)
    message(STATUS "Google Breakpad was not found, will build without it")
  endif()
endif()

if(WIN32 AND CREATE_BUNDLE)
  find_package(NSIS)
  if(NOT NSIS_FOUND)
    message(STATUS "NSIS was not found, won't create the installer")
  else()
    message(STATUS "NSIS was found: ${NSIS_MAKE}; will create the installer")
  endif()
endif()

find_package(Sanitizers)
