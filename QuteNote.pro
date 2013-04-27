#-------------------------------------------------
#
# Project created by QtCreator 2013-03-26T01:12:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QuteNote
TEMPLATE = app


SOURCES += main.cpp\
    NoteRichTextEditor.cpp \
    MainWindow.cpp

HEADERS  += \
    NoteRichTextEditor.h \
    MainWindow.h

FORMS    += \
    NoteRichTextEditor.ui \
    MainWindow.ui

RESOURCES += \
    resource/icons.qrc

linux-g++-64 {

QMAKE_CXXFLAGS += -std=c++11 -isystem/usr/incude/qt4/QtCore/ -isystem/usr/include/qt4/QtGui/

CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -isystem -W -Wall -Wextra -Werror -Wwrite-strings -Winit-self \
                      -Wcast-align -Wcast-qual -Wold-style-cast -Wpointer-arith \
                      -Wstrict-aliasing -Wformat=2 -Wuninitialized -Wno-unused-variable \
                      -Wno-missing-declarations -Woverloaded-virtual -Wnon-virtual-dtor \
                      -Wctor-dtor-privacy -Wno-long-long
}

}

OTHER_FILES += \
    TODO.txt
