#include "TestLog.h"
#include "QtTestUtil/QtTestUtil.h"
#include "QsLog.h"
#include "QsLogDest.h"
#include "QsLogDestFile.h"
#include "QsLogDestConsole.h"
#include "QsLogDestFile.h"
#include "QsLogDestFunctor.h"
#include <QTest>
#include <QSharedPointer>
#include <QtGlobal>

void DummyLogFunction(const QsLogging::LogMessage&)
{

}

// Autotests for QsLog
class TestLog : public QObject
{
    Q_OBJECT
public:
    TestLog()
        : mockDest1(new MockDestination)
        , mockDest2(new MockDestination)
    {
    }

private slots:
    void initTestCase();
    void testAllLevels();
    void testMessageText();
    void testLevelChanges();
    void testLevelParsing();
    void testDestinationRemove();
    void testWillRotate();
    void testRotation_data();
    void testRotation();
    void testRotationNoBackup();
    void testDestinationType();

private:
    QSharedPointer<MockDestination> mockDest1;
    QSharedPointer<MockDestination> mockDest2;
};

void TestLog::initTestCase()
{
    using namespace QsLogging;
    QCOMPARE(Logger::instance().loggingLevel(), InfoLevel);
    Logger::instance().setLoggingLevel(TraceLevel);
    QCOMPARE(Logger::instance().loggingLevel(), TraceLevel);
    Logger::instance().addDestination(mockDest1);
    Logger::instance().addDestination(mockDest2);
}

void TestLog::testAllLevels()
{
    mockDest1->clear();
    mockDest2->clear();

    QLOG_TRACE() << "trace level";
    QLOG_DEBUG() << "debug level";
    QLOG_INFO() << "info level";
    QLOG_WARN() << "warn level";
    QLOG_ERROR() << "error level";
    QLOG_FATAL() << "fatal level";

    using namespace QsLogging;
    QCOMPARE(mockDest1->messageCount(), 6);
    QCOMPARE(mockDest1->messageCountForLevel(TraceLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(DebugLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(InfoLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(WarnLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(ErrorLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(FatalLevel), 1);
}

void TestLog::testMessageText()
{
    mockDest1->clear();

    QLOG_DEBUG() << "foobar";
    QLOG_WARN() << "eiszeit";
    QLOG_FATAL() << "ruh-roh!";
    using namespace QsLogging;
    QVERIFY(mockDest1->hasMessage("foobar", DebugLevel));
    QVERIFY(mockDest1->hasMessage("eiszeit", WarnLevel));
    QVERIFY(mockDest1->hasMessage("ruh-roh!", FatalLevel));
    QCOMPARE(mockDest1->messageCount(), 3);
}

void TestLog::testLevelChanges()
{
    mockDest1->clear();
    mockDest2->clear();

    using namespace QsLogging;
    Logger::instance().setLoggingLevel(WarnLevel);
    QCOMPARE(Logger::instance().loggingLevel(), WarnLevel);

    QLOG_TRACE() << "one";
    QLOG_DEBUG() << "two";
    QLOG_INFO() << "three";
    QCOMPARE(mockDest1->messageCount(), 0);
    QCOMPARE(mockDest2->messageCount(), 0);

    QLOG_WARN() << "warning";
    QLOG_ERROR() << "error";
    QLOG_FATAL() << "fatal";
    QCOMPARE(mockDest1->messageCountForLevel(WarnLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(ErrorLevel), 1);
    QCOMPARE(mockDest1->messageCountForLevel(FatalLevel), 1);
    QCOMPARE(mockDest1->messageCount(), 3);
    QCOMPARE(mockDest2->messageCountForLevel(WarnLevel), 1);
    QCOMPARE(mockDest2->messageCountForLevel(ErrorLevel), 1);
    QCOMPARE(mockDest2->messageCountForLevel(FatalLevel), 1);
    QCOMPARE(mockDest2->messageCount(), 3);
}

void TestLog::testLevelParsing()
{
    mockDest1->clear();

    QLOG_TRACE() << "one";
    QLOG_DEBUG() << "two";
    QLOG_INFO() << "three";
    QLOG_WARN() << "warning";
    QLOG_ERROR() << "error";
    QLOG_FATAL() << "fatal";

    using namespace QsLogging;
    for(int i = 0;i < mockDest1->messageCount();++i) {
        bool conversionOk = false;
        const MockDestination::Message& m = mockDest1->messageAt(i);
        QCOMPARE(Logger::levelFromLogMessage(m.text, &conversionOk), m.level);
        QCOMPARE(Logger::levelFromLogMessage(m.text), m.level);
        QCOMPARE(conversionOk, true);
    }
}

void TestLog::testDestinationRemove()
{
    using namespace QsLogging;
    mockDest1->clear();
    mockDest2->clear();
    Logger::instance().setLoggingLevel(DebugLevel);
    QSharedPointer<MockDestination> toAddAndRemove(new MockDestination);
    Logger::instance().addDestination(toAddAndRemove);

    QLOG_INFO() << "one for all";
    QCOMPARE(mockDest1->messageCount(), 1);
    QCOMPARE(mockDest2->messageCount(), 1);
    QCOMPARE(toAddAndRemove->messageCount(), 1);

    Logger::instance().removeDestination(toAddAndRemove);
    QLOG_INFO() << "one for (almost) all";
    QCOMPARE(mockDest1->messageCount(), 2);
    QCOMPARE(mockDest2->messageCount(), 2);
    QCOMPARE(toAddAndRemove->messageCount(), 1);
    QCOMPARE(toAddAndRemove->messageCountForLevel(InfoLevel), 1);

    Logger::instance().addDestination(toAddAndRemove);
    QLOG_INFO() << "another one for all";
    QCOMPARE(mockDest1->messageCount(), 3);
    QCOMPARE(mockDest2->messageCount(), 3);
    QCOMPARE(toAddAndRemove->messageCount(), 2);
    QCOMPARE(toAddAndRemove->messageCountForLevel(InfoLevel), 2);
}

void TestLog::testWillRotate()
{
    using namespace QsLogging;
    MockSizeRotationStrategy rotationStrategy;
    rotationStrategy.setBackupCount(1);
    rotationStrategy.setMaximumSizeInBytes(10);
    QCOMPARE(rotationStrategy.shouldRotate(), false);

    rotationStrategy.includeMessageInCalculation(QLatin1String("12345"));
    QCOMPARE(rotationStrategy.shouldRotate(), false);

    rotationStrategy.includeMessageInCalculation(QLatin1String("12345"));
    QCOMPARE(rotationStrategy.shouldRotate(), false);

    rotationStrategy.includeMessageInCalculation(QLatin1String("1"));
    QCOMPARE(rotationStrategy.shouldRotate(), true);
}

void TestLog::testRotation_data()
{
    QTest::addColumn<int>("backupCount");

    QTest::newRow("one backup") << 1;
    QTest::newRow("three backups") << 3;
    QTest::newRow("five backups") << 5;
    QTest::newRow("ten backups") << 10;
}

void TestLog::testRotation()
{
    using namespace QsLogging;

    QFETCH(int, backupCount);
    MockSizeRotationStrategy rotationStrategy;
    rotationStrategy.setBackupCount(backupCount);
    for (int i = 0;i < backupCount;++i) {
        rotationStrategy.rotate();
        QCOMPARE(rotationStrategy.filesList().size(), 1 + i + 1); // 1 log + "rotation count" backups
    }

    rotationStrategy.rotate();
    QCOMPARE(rotationStrategy.filesList().size(), backupCount + 1); // 1 log + backup count
}

void TestLog::testRotationNoBackup()
{
    using namespace QsLogging;
    MockSizeRotationStrategy rotationStrategy;
    rotationStrategy.setBackupCount(0);
    rotationStrategy.setMaximumSizeInBytes(10);

    rotationStrategy.rotate();
    QCOMPARE(rotationStrategy.filesList().size(), 1); // log
}



void TestLog::testDestinationType()
{
    using namespace QsLogging;

    DestinationPtr console = DestinationFactory::MakeDebugOutputDestination();
    DestinationPtr file = DestinationFactory::MakeFileDestination("test.log",
                           DisableLogRotation, MaxSizeBytes(5000), MaxOldLogCount(1));
    DestinationPtr func = DestinationFactory::MakeFunctorDestination(&DummyLogFunction);

    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), false);

    Logger::instance().addDestination(console);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), false);

    Logger::instance().addDestination(file);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), false);

    Logger::instance().addDestination(func);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), true);

    Logger::instance().removeDestination(console);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), true);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), true);

    Logger::instance().removeDestination(file);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), true);

    Logger::instance().removeDestination(func);
    QCOMPARE(Logger::instance().hasDestinationOfType(DebugOutputDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FileDestination::Type), false);
    QCOMPARE(Logger::instance().hasDestinationOfType(FunctorDestination::Type), false);
}

QTTESTUTIL_REGISTER_TEST(TestLog);
#include "TestLog.moc"
