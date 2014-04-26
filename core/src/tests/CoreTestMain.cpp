#include "CoreTester.h"
#include <logging/QuteNoteLogger.h>
#include <QApplication>
#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlDatabase>

using namespace qute_note::test;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QuteNoteCoreTests");

    QUTE_NOTE_INITIALIZE_LOGGING();
    QUTE_NOTE_SET_MIN_LOG_LEVEL(qute_note::QuteNoteLogger::Level::LEVEL_WARNING);

    QTest::qExec(new CoreTester);
    return 0;
}
