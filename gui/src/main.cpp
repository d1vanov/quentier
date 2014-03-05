#include "MainWindow.h"
#include <logging/QuteNoteLogger.h>
#include <QApplication>

// TODO: install exception handler

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QuteNote");

    QUTE_NOTE_INITIALIZE_LOGGING();

    MainWindow w;
    w.show();

    return app.exec();
}
