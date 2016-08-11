#include "QuentierLogger_p.h"
#include <quentier/exception/LoggerInitializationException.h>
#include <quentier/utility/DesktopServices.h>
#include <QApplication>
#include <QFileInfo>
#include <QTextCodec>
#include <QDir>
#include <QDateTime>
#include <iostream>

#if __cplusplus < 201103L
#include <QMutexLocker>
#endif

namespace quentier {

QuentierFileLogWriter::QuentierFileLogWriter(const MaxSizeBytes & maxSizeBytes,
                                             const MaxOldLogFilesCount & maxOldLogFilesCount,
                                             QObject * parent) :
    IQuentierLogWriter(parent),
    m_logFile(),
    m_stream(),
    m_maxSizeBytes(maxSizeBytes.size()),
    m_maxOldLogFilesCount(maxOldLogFilesCount.count()),
    m_currentLogFileSize(0),
    m_currentOldLogFilesCount(0)
{
    QString logFileDirPath = applicationPersistentStoragePath() + QStringLiteral("/logs-quentier");

    QDir logFileDir(logFileDirPath);
    if (Q_UNLIKELY(!logFileDir.exists()))
    {
        if (Q_UNLIKELY(!logFileDir.mkpath("."))) {
            QNLocalizedString error = QT_TR_NOOP("Can't create the log file path");
            error += ": ";
            error += logFileDirPath;
            throw LoggerInitializationException(error);
        }
    }

    QString logFileName = logFileDirPath + QStringLiteral("/") + QApplication::applicationName() +
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

    m_stream.setDevice(&m_logFile);
    m_stream.setCodec(QTextCodec::codecForName("UTF-8"));

    m_currentLogFileSize = m_logFile.size();

    // Seek for old log files with indices from 1 to m_maxOldLogFilesCount, count the existing ones
    for(int i = 1; i < m_maxOldLogFilesCount; ++i)
    {
        const QString previousLogFilePath = logFileDirPath + QStringLiteral("/") + QApplication::applicationName() +
                                            QStringLiteral("-log.") + QString::number(i) + QStringLiteral(".txt");
        if (QFile::exists(previousLogFilePath)) {
            ++m_currentOldLogFilesCount;
        }
    }
}

QuentierFileLogWriter::~QuentierFileLogWriter()
{
    Q_UNUSED(m_logFile.flush())
    m_logFile.close();
}

void QuentierFileLogWriter::write(QString message)
{
    message.prepend(QDateTime::currentDateTime().toString(Qt::ISODate) + QStringLiteral(" "));

    qint64 messageSize = message.toUtf8().size();
    m_currentLogFileSize += messageSize;

    if (Q_UNLIKELY(m_currentLogFileSize > m_maxSizeBytes)) {
        rotate();
    }

    m_stream << message << "\n";
}

void QuentierFileLogWriter::rotate()
{
    QString logFileDirPath = QFileInfo(m_logFile).absolutePath();

    // 1) Rename all the existing old log files
    for(int i = m_currentOldLogFilesCount; i >= 1; --i)
    {
        const QString previousLogFilePath = logFileDirPath + QStringLiteral("/") + QApplication::applicationName() +
                                            QStringLiteral("-log.") + QString::number(i) + QStringLiteral(".txt");
        QFile previousLogFile(previousLogFilePath);
        if (Q_UNLIKELY(!previousLogFile.exists())) {
            continue;
        }

        const QString newLogFilePath = logFileDirPath + QStringLiteral("/") + QApplication::applicationName() +
                                       QStringLiteral("-log.") + QString::number(i+1) + QStringLiteral(".txt");

        // Just-in-case check, shouldn't really do anything in normal circumstances
        Q_UNUSED(QFile::remove(newLogFilePath))

        bool res = previousLogFile.rename(newLogFilePath);
        if (Q_UNLIKELY(!res)) {
            std::cerr << "Can't rename one of previous libquentier log files for log file rotation: attempted to rename from "
                      << qPrintable(previousLogFilePath) << " to " << qPrintable(newLogFilePath) << ", error: " << qPrintable(previousLogFile.errorString())
                      << " (error code " << qPrintable(QString::number(previousLogFile.error())) << ")\n";
        }
    }

    // 2) Rename the current log file
    m_stream.setDevice(Q_NULLPTR);
    m_logFile.close();
    bool res = m_logFile.rename(logFileDirPath + QStringLiteral("/") + QApplication::applicationName() + QStringLiteral("-log.1.txt"));
    if (Q_UNLIKELY(!res)) {
        std::cerr << "Can't rename the current libquentier log file for log file rotation, error: " << qPrintable(m_logFile.errorString())
                      << " (error code " << qPrintable(QString::number(m_logFile.error())) << ")\n";
        return;
    }

    // 3) Open the renamed file
    bool opened = m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
    if (Q_UNLIKELY(!opened)) {
        std::cerr << "Can't open the renamed/rotated libquentier log file, error: " << qPrintable(m_logFile.errorString())
                  << " (error code " << qPrintable(QString::number(m_logFile.error())) << ")\n";
        return;
    }

    m_currentLogFileSize = m_logFile.size();

    m_stream.setDevice(&m_logFile);
    m_stream.setCodec(QTextCodec::codecForName("UTF-8"));

    // 4) Increase the current count of old log files
    ++m_currentOldLogFilesCount;

    if (Q_LIKELY(m_currentOldLogFilesCount < m_maxOldLogFilesCount)) {
        return;
    }

    // 5) If got here, there are too many old log files, need to remove the oldest one
    QString oldestLogFilePath = logFileDirPath + QStringLiteral("/") + QApplication::applicationName() + QStringLiteral("-log.") +
                                QString::number(m_currentOldLogFilesCount) + QStringLiteral(".txt");
    res = QFile::remove(oldestLogFilePath);
    if (Q_UNLIKELY(!res)) {
        std::cerr << "Can't remove the oldest previous libquentier log file: " << qPrintable(oldestLogFilePath) << "\n";
        return;
    }

    // 6) Decrement the current count of old log files
    --m_currentOldLogFilesCount;
}

QuentierConsoleLogWriter::QuentierConsoleLogWriter(QObject * parent) :
    IQuentierLogWriter(parent)
{}

void QuentierConsoleLogWriter::write(QString message)
{
#if defined Q_OS_WIN
#include "Windows.h"
    OutputDebugStringW(reinterpret_cast<const WCHAR*>(message.utf16()));
    OutputDebugStringW(L"\n");
#else
    fprintf(stderr, "%s\n", qPrintable(message));
    fflush(stderr);
#endif
}

QuentierLogger & QuentierLogger::instance()
{
    static QuentierLogger instance;
    return instance;
}

QuentierLogger::QuentierLogger(QObject * parent) :
    QObject(parent),
    m_pImpl(Q_NULLPTR)
{
    // NOTE: since C++11 static construction is thread-safe so if we use C++11, we don't need to lock anything ourselves
#if __cplusplus < 201103L
    QMutexLocker locker(&m_constructionMutex);
#endif

    if (!m_pImpl) {
        m_pImpl = new QuentierLoggerImpl(this);
        addLogWriter(new QuentierFileLogWriter(MaxSizeBytes(104857600), MaxOldLogFilesCount(2)));
    }
}

void QuentierLogger::addLogWriter(IQuentierLogWriter * pLogWriter)
{
    for(auto it = m_pImpl->m_logWriterPtrs.begin(), end = m_pImpl->m_logWriterPtrs.end(); it != end; ++it)
    {
        if (Q_UNLIKELY(it->data() == pLogWriter)) {
            return;
        }
    }

    m_pImpl->m_logWriterPtrs << QPointer<IQuentierLogWriter>(pLogWriter);

    QObject::connect(this, QNSIGNAL(QuentierLogger,sendLogMessage,QString),
                     pLogWriter, QNSLOT(IQuentierLogWriter,write,QString),
                     Qt::QueuedConnection);

    pLogWriter->setParent(Q_NULLPTR);
    pLogWriter->moveToThread(m_pImpl->m_pLogWriteThread);
}

void QuentierLogger::removeLogWriter(IQuentierLogWriter * pLogWriter)
{
    bool found = false;
    for(auto it = m_pImpl->m_logWriterPtrs.begin(), end = m_pImpl->m_logWriterPtrs.end(); it != end; ++it)
    {
        if (Q_LIKELY(it->data() != pLogWriter)) {
            continue;
        }

        found = true;
        Q_UNUSED(m_pImpl->m_logWriterPtrs.erase(it));
        break;
    }

    if (Q_UNLIKELY(!found)) {
        return;
    }

    QObject::disconnect(this, QNSIGNAL(QuentierLogger,sendLogMessage,QString),
                        pLogWriter, QNSLOT(IQuentierLogWriter,write,QString));

    pLogWriter->moveToThread(thread());
    pLogWriter->deleteLater();
}

void QuentierLogger::write(QString message)
{
    emit sendLogMessage(message);
}

void QuentierLogger::setMinLogLevel(const LogLevel::type minLogLevel)
{
    Q_UNUSED(m_pImpl->m_minLogLevel.fetchAndStoreOrdered(static_cast<int>(minLogLevel)))
}

LogLevel::type QuentierLogger::minLogLevel() const
{
    return static_cast<LogLevel::type>(static_cast<int>(m_pImpl->m_minLogLevel));
}

QuentierLoggerImpl::QuentierLoggerImpl(QObject * parent) :
    QObject(parent),
    m_logWriterPtrs(),
    m_minLogLevel(static_cast<int>(LogLevel::InfoLevel)),
    m_pLogWriteThread(new QThread)
{
    QObject::connect(m_pLogWriteThread, QNSIGNAL(QThread,finished), m_pLogWriteThread, QNSLOT(QThread,deleteLater));
    QObject::connect(this, QNSIGNAL(QuentierLoggerImpl,destroyed), m_pLogWriteThread, QNSLOT(QThread,quit));
    m_pLogWriteThread->start(QThread::LowPriority);
}

} // namespace quentier
