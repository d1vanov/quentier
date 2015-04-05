include(../../../mimetypes-nolibs.pri)
LIBS += -L$$OUT_PWD/../../../src/mimetypes -lQtMimeTypes

TEMPLATE = app

TARGET   = tst_qmimetype
CONFIG   += qtestlib
CONFIG   -= app_bundle

DEPENDPATH += .

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wshadow -Wnon-virtual-dtor

CONFIG += depend_includepath


SOURCES += tst_qmimetype.cpp

HEADERS += tst_qmimetype.h


QMAKE_EXTRA_TARGETS += check
check.depends = $$TARGET
check.commands = LD_LIBRARY_PATH=$$(LD_LIBRARY_PATH):$$OUT_PWD/../../../src/mimetypes ./$$TARGET
