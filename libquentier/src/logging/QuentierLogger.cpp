#include <quentier/logging/QuentierLogger.h>
#include <quentier/exception/LoggerInitializationException.h>
#include <quentier/utility/DesktopServices.h>
#include <QApplication>

#ifndef QS_LOG_SEPARATE_THREAD
#define QS_LOG_SEPARATE_THREAD
#endif

#include <QsLog/QsLog.h>
#include <QsLog/QsLogLevel.h>
#include <QsLog/QsLogDest.h>
#include <QsLog/QsLogDestFile.h>

namespace quentier {

void QuentierInitializeLogging()
{
    using namespace QsLogging;

    Logger & logger = Logger::instance();
    logger.setLoggingLevel(InfoLevel);

    QString appPersistentStoragePathString = quentier::applicationPersistentStoragePath();
    QDir appPersistentStoragePathFolder(appPersistentStoragePathString);
    if (!appPersistentStoragePathFolder.exists())
    {
        if (!appPersistentStoragePathFolder.mkpath(".")) {
            quentier::QNLocalizedString error = QT_TR_NOOP("Can't create path for log file");
            error += ": ";
            error += appPersistentStoragePathString;
            throw quentier::LoggerInitializationException(error);
        }
    }

    QString logFileName = appPersistentStoragePathString + QString("/") + QApplication::applicationName() +
                          QString("-log.txt");
    DestinationPtr fileDest(DestinationFactory::MakeFileDestination(logFileName, EnableLogRotation,
                                                                    MaxSizeBytes(104857600), MaxOldLogCount(2)));
    logger.addDestination(fileDest);
}

QDebug QuentierGetLoggerHelper(const LogLevel::type logLevel)
{
#define CONVERT_TO_QSLOG(level) \
    case LogLevel::level: \
        { \
            return QsLogging::Logger::Helper(QsLogging::level).stream(); \
        }

    switch(logLevel)
    {
        CONVERT_TO_QSLOG(TraceLevel)
        CONVERT_TO_QSLOG(DebugLevel)
        CONVERT_TO_QSLOG(InfoLevel)
        CONVERT_TO_QSLOG(WarnLevel)
        CONVERT_TO_QSLOG(ErrorLevel)
        CONVERT_TO_QSLOG(FatalLevel)
    default:
        {
            qWarning() << "Unidentified logging level: " << logLevel;
            return QsLogging::Logger::Helper(QsLogging::FatalLevel).stream();
        }
    }

#undef CONVERT_TO_QSLOG
}

void QuentierSetMinLogLevel(const LogLevel::type logLevel)
{
    switch(logLevel)
    {
    case LogLevel::TraceLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::TraceLevel);
        break;
    case LogLevel::DebugLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::DebugLevel);
        break;
    case LogLevel::InfoLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::DebugLevel);
        break;
    case LogLevel::WarnLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::WarnLevel);
        break;
    case LogLevel::ErrorLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::ErrorLevel);
        break;
    case LogLevel::FatalLevel:
        QsLogging::Logger::instance().setLoggingLevel(QsLogging::FatalLevel);
        break;
    default:
        {
            qWarning() << "Unidentified minimal logging level: " << logLevel;
            break;
        }
    }
}

bool QuentierIsLogLevelActive(const LogLevel::type logLevel)
{
#define CONVERT_TO_QSLOG(level) \
    case LogLevel::level: \
        { \
            return (QsLogging::Logger::instance().loggingLevel() <= QsLogging::level); \
        }

    switch(logLevel)
    {
        CONVERT_TO_QSLOG(TraceLevel)
        CONVERT_TO_QSLOG(DebugLevel)
        CONVERT_TO_QSLOG(InfoLevel)
        CONVERT_TO_QSLOG(WarnLevel)
        CONVERT_TO_QSLOG(ErrorLevel)
        CONVERT_TO_QSLOG(FatalLevel)
    default:
        {
            qWarning() << "Unidentified tested logging level: " << logLevel;
            break;
        }
    }

    return false;

#undef CONVERT_TO_QSLOG
}

void QuentierAddStdOutLogDestination()
{
    QsLogging::Logger & logger = QsLogging::Logger::instance();
    logger.addDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
}

} // namespace quentier
