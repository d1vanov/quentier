#include "QsLog.h"
#include "QsLogDest.h"
#include "QsLogWindow.h"
#include <QApplication>
#include <QTimer>
#include <QUuid>

class LogTimer : public QObject
{
    Q_OBJECT
public:
    LogTimer(QObject* parent = 0)
        : t(parent)
    {
        connect(&t, SIGNAL(timeout()), this, SLOT(onTick()));
        t.start(500);
    }

private slots:
    void onTick()
    {
        static int i = 1;
        const QUuid uuid = QUuid::createUuid();
        if (i % 5 == 0) {
            QLOG_ERROR() << "message" << uuid.toString();
        } else if (i % 4 == 0) {
            QLOG_WARN() << "message" << uuid.toString();
        } else if (i % 3 == 0) {
            QLOG_INFO() << "message" << uuid.toString();
        } else if (i % 2 == 0) {
            QLOG_DEBUG() << "message" << uuid.toString();
        } else {
            QLOG_TRACE() << "message" << uuid.toString();
        }
        ++i;
    }

private:
    QTimer t;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);

    using namespace QsLogging;

    // 1. init the logging mechanism
    Logger& logger = Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);

    // 2. add model destination
    DestinationPtr modelDestination(DestinationFactory::MakeModelDestination());
    logger.addDestination(modelDestination);

    LogTimer timer;
    Window w(modelDestination);
    w.show();

    QLOG_DEBUG() << "window was created";
    QLOG_TRACE() << "preparing to call exec...";

    return a.exec();
}

#include "log_example_window_main.moc"
