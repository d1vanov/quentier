include(../../mimetypes-nolibs.pri)

TARGET = QtMimeTypes
TEMPLATE = lib

win32: DESTDIR = ./

DEFINES += QMIME_LIBRARY

DEPENDPATH  *= $$PWD

CONFIG += depend_includepath create_pc create_prl no_install_prl

unix {
    CONFIG += create_pc create_prl no_install_prl
    QMAKE_PKGCONFIG_DESTDIR = pkgconfig

    QMAKE_PKGCONFIG_NAME = QtMimeTypes
    QMAKE_PKGCONFIG_DESCRIPTION = A Qt4 backport of the Qt5 mimetypes API
    QMAKE_PKGCONFIG_PREFIX = $${PREFIX}
    QMAKE_PKGCONFIG_LIBDIR = $${LIBDIR}
    QMAKE_PKGCONFIG_INCDIR = $${INCLUDEDIR}/QtMimeTypes
    QMAKE_PKGCONFIG_REQUIRES = QtCore
}

QT     = core

QMAKE_CXXFLAGS += -W -Wall -Wextra -Wshadow -Wnon-virtual-dtor

SOURCES += qmimedatabase.cpp \
           qmimetype.cpp \
           qmimemagicrulematcher.cpp \
           qmimetypeparser.cpp \
           qmimemagicrule.cpp \
           qmimeglobpattern.cpp \
           qmimeprovider.cpp

the_includes.files += qmime_global.h \
                      qmimedatabase.h \
                      qmimetype.h \

HEADERS += $$the_includes.files \
           qmimemagicrulematcher_p.h \
           qmimetype_p.h \
           qmimetypeparser_p.h \
           qmimedatabase_p.h \
           qmimemagicrule_p.h \
           qmimeglobpattern_p.h \
           qmimeprovider_p.h

SOURCES += inqt5/qstandardpaths.cpp
win32: SOURCES += inqt5/qstandardpaths_win.cpp
unix: {
    macx-*: {
        SOURCES += inqt5/qstandardpaths_mac.cpp
        LIBS += -framework Carbon
    } else {
        SOURCES += inqt5/qstandardpaths_unix.cpp
    }
}

RESOURCES += \
    mimetypes.qrc

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xEA6A790B
    TARGET.CAPABILITY =
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = lib.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    target.path = $${LIBDIR}
    the_includes.path = $${INCLUDEDIR}/QtMimeTypes
    INSTALLS += target the_includes
}
