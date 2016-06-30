#This project file links with QsLog dynamically and outputs a shared object.

QT -= gui

TARGET = log_example_shared
TEMPLATE = lib

DEFINES += EXAMPLE_IS_SHARED_LIBRARY QSLOG_IS_SHARED_LIBRARY_IMPORT
SOURCES += log_example_shared.cpp
HEADERS += log_example_shared.h
INCLUDEPATH += ../
QSLOG_DESTDIR=$$(QSLOG_DESTDIR)
!isEmpty(QSLOG_DESTDIR) {
    DESTDIR = $${QSLOG_DESTDIR}/bin
    OBJECTS_DIR = $${QSLOG_DESTDIR}
    MOC_DIR = $${QSLOG_DESTDIR}
} else {
    error(Please set the QSLOG_DESTDIR environment variable)
}

LIBS += -L$${QSLOG_DESTDIR}/bin
win32 {
    LIBS += -lQsLog2
} else {
    LIBS += -lQsLog
}


