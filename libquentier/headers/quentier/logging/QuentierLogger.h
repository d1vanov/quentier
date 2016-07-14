#ifndef LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H
#define LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H

#include <quentier/exception/LoggerInitializationException.h>
#include <quentier/utility/DesktopServices.h>

#ifndef QS_LOG_LINE_NUMBERS
#define QS_LOG_LINE_NUMBERS
#endif

#ifndef QS_LOG_SEPARATE_THREAD
#define QS_LOG_SEPARATE_THREAD
#endif

#include <QsLog.h>
#include <QsLogLevel.h>
#include <QsLogDest.h>
#include <QsLogDestFile.h>

#define QNTRACE(message) \
    QLOG_TRACE() << message;

#define QNDEBUG(message) \
    QLOG_DEBUG() << message;

#define QNINFO(message) \
    QLOG_INFO() << message;

#define QNWARNING(message) \
    QLOG_WARN() << message;

#define QNCRITICAL(message) \
    QLOG_ERROR() << message;

#define QNFATAL(message) \
    QLOG_FATAL() << message;

#define QUENTIER_SET_MIN_LOG_LEVEL(level) \
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::level##Level);

#define QUENTIER_INITIALIZE_LOGGING() \
{ \
    using namespace QsLogging; \
    \
    Logger & logger = Logger::instance(); \
    logger.setLoggingLevel(InfoLevel); \
    \
    QString appPersistentStoragePathString = quentier::applicationPersistentStoragePath(); \
    QDir appPersistentStoragePathFolder(appPersistentStoragePathString); \
    if (!appPersistentStoragePathFolder.exists()) { \
        if (!appPersistentStoragePathFolder.mkpath(".")) { \
            quentier::QNLocalizedString error = QT_TR_NOOP("Can't create path for log file"); \
            error += ": "; \
            error += appPersistentStoragePathString; \
            throw quentier::LoggerInitializationException(error); \
        } \
    } \
    \
    QString logFileName = appPersistentStoragePathString + QString("/") + QApplication::applicationName() + \
                          QString("-log.txt"); \
    DestinationPtr fileDest(DestinationFactory::MakeFileDestination(logFileName, EnableLogRotation, \
                                                                    MaxSizeBytes(104857600), MaxOldLogCount(2))); \
    logger.addDestination(fileDest); \
}

#endif // LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H
