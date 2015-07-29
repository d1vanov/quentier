#ifndef __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
#define __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H

#include "LoggerInitializationException.h"
#include <qute_note/utility/DesktopServices.h>
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

#define QUTE_NOTE_SET_MIN_LOG_LEVEL(level) \
    QsLogging::Logger::instance().setLoggingLevel(QsLogging::level##Level);

#define QUTE_NOTE_INITIALIZE_LOGGING() \
{ \
    using namespace QsLogging; \
    \
    Logger & logger = Logger::instance(); \
    logger.setLoggingLevel(InfoLevel); \
    \
    QString appPersistentStoragePathString = qute_note::applicationPersistentStoragePath(); \
    QDir appPersistentStoragePathFolder(appPersistentStoragePathString); \
    if (!appPersistentStoragePathFolder.exists()) { \
        if (!appPersistentStoragePathFolder.mkpath(".")) { \
            throw qute_note::LoggerInitializationException(QString("Can't create path for log file: ") + appPersistentStoragePathString); \
        } \
    } \
    \
    QString logFileName = appPersistentStoragePathString + QString("/") + QApplication::applicationName() + \
                          QString("-log.txt"); \
    DestinationPtr fileDest(DestinationFactory::MakeFileDestination(logFileName, EnableLogRotation, \
                                                                    MaxSizeBytes(104857600), MaxOldLogCount(2))); \
    logger.addDestination(fileDest); \
}

#endif // __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
