#include "CoreTester.h"
#include <logging/QuteNoteLogger.h>
#include <tools/QuteNoteApplication.h>
#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlDatabase>

using namespace qute_note::test;

int main(int argc, char *argv[])
{
    qute_note::QuteNoteApplication app(argc, argv);
    app.setApplicationName("QuteNoteCoreTests");

    QUTE_NOTE_INITIALIZE_LOGGING();
    QUTE_NOTE_SET_MIN_LOG_LEVEL(qute_note::QuteNoteLogger::Level::LEVEL_WARNING);

    QTest::qExec(new CoreTester);
    return 0;
}
