#-------------------------------------------------
#
# Project created by QtCreator 2013-03-26T01:12:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QuteNote
TEMPLATE = app

INCLUDEPATH += . src/note_editor src/gui

SOURCES += main.cpp\
    src/gui/MainWindow.cpp \
    src/gui/NoteEditorWidget.cpp \
    src/note_editor/ToDoCheckboxTextObject.cpp \
    src/note_editor/QuteNoteTextEdit.cpp \
    src/evernote_sdk/src/UserStore_types.cpp \
    src/evernote_sdk/src/UserStore_constants.cpp \
    src/evernote_sdk/src/UserStore.cpp \
    src/evernote_sdk/src/Types_types.cpp \
    src/evernote_sdk/src/Types_constants.cpp \
    src/evernote_sdk/src/NoteStore_types.cpp \
    src/evernote_sdk/src/NoteStore_constants.cpp \
    src/evernote_sdk/src/NoteStore.cpp \
    src/evernote_sdk/src/Limits_types.cpp \
    src/evernote_sdk/src/Limits_constants.cpp \
    src/evernote_sdk/src/Errors_types.cpp \
    src/evernote_sdk/src/Errors_constants.cpp

HEADERS  += \
    src/gui/MainWindow.h \
    src/gui/NoteEditorWidget.h \
    src/note_editor/ToDoCheckboxTextObject.h \
    src/note_editor/HorizontalLineExtraData.h \
    src/note_editor/QuteNoteTextEdit.h \
    src/evernote_sdk/src/UserStore_types.h \
    src/evernote_sdk/src/UserStore_constants.h \
    src/evernote_sdk/src/UserStore.h \
    src/evernote_sdk/src/Types_types.h \
    src/evernote_sdk/src/Types_constants.h \
    src/evernote_sdk/src/NoteStore_types.h \
    src/evernote_sdk/src/NoteStore_constants.h \
    src/evernote_sdk/src/NoteStore.h \
    src/evernote_sdk/src/Limits_types.h \
    src/evernote_sdk/src/Limits_constants.h \
    src/evernote_sdk/src/Errors_types.h \
    src/evernote_sdk/src/Errors_constants.h \
    src/service_frontend/INoteTakingService.h

FORMS    += \
    src/gui/MainWindow.ui \
    src/gui/NoteEditorWidget.ui

RESOURCES += \
    resource/icons.qrc

LIBS += -lthrift

linux-g++-64  {
QMAKE_CXXFLAGS += -isystem/usr/incude/qt4/QtCore/ -isystem/usr/include/qt4/QtGui/ \
                  -isystem$$PWD/src/evernote_sdk/src/
}

win32-g++ {
QMAKE_CXXFLAGS += -isystem${QTDIR}/include/QtCore/ -isystem${QTDIR}/include/QtGui/ \
                  -isystem${QTDIR}/include/QtWidgets/ -isystem$$PWD/src/evernote_sdk/src/
}

linux-g++-64 | win32-g++ {
QMAKE_CXXFLAGS += -std=c++11
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -Wwrite-strings -Winit-self -Wcast-align -Wcast-qual -Wold-style-cast \
                      -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wuninitialized \
                      -Wno-unused-variable -Wno-missing-declarations -Woverloaded-virtual \
                      -Wnon-virtual-dtor -Wctor-dtor-privacy -Wno-long-long -Wno-old-style-cast \
                      -Werror
}
}

OTHER_FILES += \
    TODO.txt
