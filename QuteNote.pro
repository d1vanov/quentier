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
        mainwindow.cpp \
    NoteRichTextEditor.cpp

HEADERS  += mainwindow.h \
    NoteRichTextEditor.h

FORMS    += mainwindow.ui \
    NoteRichTextEditor.ui

RESOURCES += \
    resource/icons.qrc

linux-g++-64 {
QMAKE_CXXFLAGS += -std=c++11
}
