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

    QTest::qExec(new CoreTester);
    return 0;
}
