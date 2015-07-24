include(../../mimetypes-nolibs.pri)

contains($$list($$[QT_VERSION]),4.[6-9].*) {
    TEMPLATE = subdirs
} else {
    TEMPLATE = aux
}

the_includes.files += QMimeDatabase \
                      QMimeType \

unix:!symbian {
    the_includes.path = $${INCLUDEDIR}/QtMimeTypes
    INSTALLS += the_includes
}
