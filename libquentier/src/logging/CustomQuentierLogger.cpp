#include "CustomQuentierLogger.h"
#include <quentier/exception/LoggerInitializationException.h>
#include <quentier/utility/DesktopServices.h>
#include <QApplication>
#include <QDir>

namespace quentier {

QuentierFileLogWriter::QuentierFileLogWriter(const QString & filename, const MaxSizeBytes & maxSizeBytes,
                                             const MaxOldLogFilesCount & maxOldLogFilesCount) :
    m_logFile(),
    m_maxSizeBytes(maxSizeBytes),
    m_maxOldLogFilesCount(maxOldLogFilesCount)
{
    QString appPersistentStoragePathString = applicationPersistentStoragePath();

    QDir appPersistentStoragePathDir(appPersistentStoragePathString);
    if (Q_UNLIKELY(!appPersistentStoragePathDir.exists()))
    {
        if (Q_UNLIKELY(!appPersistentStoragePathDir.mkpath("."))) {
            QNLocalizedString error = QT_TR_NOOP("Can't create the log file path");
            error += ": ";
            error += appPersistentStoragePathString;
            throw LoggerInitializationException(error);
        }
    }

    QString logFileName = appPersistentStoragePathString + QStringLiteral("/") + QApplication::applicationName() +
                          QStringLiteral("-log.txt");
    m_logFile.setFileName(logFileName);

    bool opened = m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
    if (Q_UNLIKELY(!opened)) {
        QNLocalizedString error = QT_TR_NOOP("Can't open the log file for writing/appending");
        error += ": ";
        error += m_logFile.errorString();
        error += " (";
        error += QT_TR_NOOP("error code");
        error += ": ";
        error += QString::number(m_logFile.error());
        throw LoggerInitializationException(error);
    }
}

QuentierFileLogWriter::~QuentierFileLogWriter()
{
    Q_UNUSED(m_logFile.flush())
    m_logFile.close();
}

void QuentierInitializeLogging()
{
    // TODO: implement
}

void QuentierAddLogEntry(const QString & message, const LogLevel::type logLevel)
{
    // TODO: implement
    Q_UNUSED(message)
    Q_UNUSED(logLevel)
}

void QuentierSetMinLogLevel(const LogLevel::type logLevel)
{
    // TODO: implement
    Q_UNUSED(logLevel)
}

bool QuentierIsLogLevelActive(const LogLevel::type logLevel)
{
    // TODO: implement
    Q_UNUSED(logLevel)
    return false;
}

void QuentierAddStdOutLogDestination()
{
    // TODO: implement
}

} // namespace quentier
