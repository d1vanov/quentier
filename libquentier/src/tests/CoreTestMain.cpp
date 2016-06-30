#include "CoreTester.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QuentierApplication.h>
#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlDatabase>

using namespace quentier::test;

int main(int argc, char *argv[])
{
    quentier::QuentierApplication app(argc, argv);
    app.setOrganizationName("d1vanov");
    app.setApplicationName("QuentierCoreTests");

    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Warn);

    QsLogging::Logger & logger = QsLogging::Logger::instance();
    logger.addDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());

    return QTest::qExec(new CoreTester);
}
