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
    QUTE_NOTE_SET_MIN_LOG_LEVEL(Warn);

    QsLogging::Logger & logger = QsLogging::Logger::instance();
    logger.addDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());

    QTest::qExec(new CoreTester(app));
    return 0;
}
