#include "MainWindow.h"
#include <logging/QuteNoteLogger.h>
#include <tools/QuteNoteApplication.h>

int main(int argc, char *argv[])
{
    qute_note::QuteNoteApplication app(argc, argv);
    app.setOrganizationName("d1vanov");
    app.setApplicationName("QuteNote");

    QUTE_NOTE_INITIALIZE_LOGGING();

    MainWindow w;
    w.show();

    return app.exec();
}
