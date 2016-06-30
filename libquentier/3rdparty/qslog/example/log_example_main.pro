#This project links with QsLog dynamically and outputs an executable file.

QT -= gui
TARGET = log_example
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += log_example_main.cpp
DEFINES += QSLOG_IS_SHARED_LIBRARY_IMPORT
INCLUDEPATH += ../
QSLOG_DESTDIR=$$(QSLOG_DESTDIR)
!isEmpty(QSLOG_DESTDIR) {
    DESTDIR = $${QSLOG_DESTDIR}/bin
    OBJECTS_DIR = $${QSLOG_DESTDIR}
    MOC_DIR = $${QSLOG_DESTDIR}
} else {
    error(Please set the QSLOG_DESTDIR environment variable)
}

LIBS += -L$${QSLOG_DESTDIR}/bin -llog_example_shared
win32 {
    LIBS += -lQsLog2
} else {
    LIBS += -lQsLog
}
