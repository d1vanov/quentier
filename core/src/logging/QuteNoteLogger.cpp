#include "QuteNoteLogger.h"
#include "LoggerInitializationException.h"
#include "../tools/ApplicationStoragePersistencePath.h"
#include <QDateTime>
#include <QtCore>

namespace qute_note {

QuteNoteLogger::QuteNoteLogger(const QString & name, const Level level) :
    m_loggerName(name),
    m_logFile(),
    m_logLevel(level)
{
    QString appPersistentStoragePathString = GetApplicationPersistentStoragePath();
    QDir appPersistentStoragePathFolder(appPersistentStoragePathString);
    if (!appPersistentStoragePathFolder.exists()) {
        if (!appPersistentStoragePathFolder.mkpath(".")) {
            throw LoggerInitializationException(QString("Can't create path for log file: ") + appPersistentStoragePathString);
        }
    }

    QString logFileName = appPersistentStoragePathString + QString("/") + m_loggerName +
                          QString("-qn.log");
    m_logFile.setFileName(logFileName);

    m_logFile.open(QFile::ReadWrite);
    if (!m_logFile.isOpen()) {
        QString error = QT_TR_NOOP("Can't open log file for writing: ");
        error += m_logFile.errorString();
        throw LoggerInitializationException(error);
    }
}

QuteNoteLogger::~QuteNoteLogger()
{
    m_logFile.close();
}

QTextStream & operator <<(QTextStream & strm, const QuteNoteLogger::Level level)
{
    switch(level)
    {
    case QuteNoteLogger::LEVEL_FATAL:
        strm << "<FATAL>";
        break;
    case QuteNoteLogger::LEVEL_CRITICAL:
        strm << "<CRITICAL>";
        break;
    case QuteNoteLogger::LEVEL_WARNING:
        strm << "<WARNING>";
        break;
    case QuteNoteLogger::LEVEL_DEBUG:
        strm << "<DEBUG>";
        break;
    default:
        strm << "<UNKNOWN>";
        break;
    }

    return strm;
}

QuteNoteLogger & QuteNoteLogger::instance(const QString & name)
{
    static QuteNoteLogger logger(name);
    return logger;
}

void QuteNoteLogger::setLogLevel(const QuteNoteLogger::Level level)
{
    m_logLevel = level;
}

void QuteNoteLogger::logMessage(const QString & message,
                                const QuteNoteLogger::Level level)
{
    if (level >= m_logLevel)
    {
        QTextStream strm(&m_logFile);

        strm << " Logger: " << m_loggerName;
        strm << ", timestamp: " << QDateTime::currentDateTime().toString(Qt::ISODate);
        strm << ", level: " << level;
        strm << message;
        strm << "\n";

        m_logFile.flush();
    }
}

} // namespace qute_note

namespace qute_note_private {

#if QT_VERSION >= 0x050000

void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString & message)
{
    using namespace qute_note;

    QuteNoteLogger & logger = QuteNoteLogger::instance("main");

    QString contextMessage = ", file: ";
    contextMessage += context.file;
    contextMessage += ", line: ";
    contextMessage += context.line;
    contextMessage += ", message: ";
    contextMessage += message;

    switch (type) {
    case QtDebugMsg:
        logger.logMessage(contextMessage, QuteNoteLogger::LEVEL_DEBUG);
        break;
    case QtWarningMsg:
        logger.logMessage(contextMessage, QuteNoteLogger::LEVEL_WARNING);
        break;
    case QtCriticalMsg:
        logger.logMessage(contextMessage, QuteNoteLogger::LEVEL_CRITICAL);
        break;
    case QtFatalMsg:
        logger.logMessage(contextMessage, QuteNoteLogger::LEVEL_FATAL);
        abort();
    }
}

#else

void messageHandler(QtMsgType type, const char * msg)
{
    using namespace qute_note;

    QuteNoteLogger & logger = __QNMAINLOGGER;

    switch (type) {
    case QtDebugMsg:
        logger.logMessage(QString(msg), QuteNoteLogger::Level::LEVEL_DEBUG);
        break;
    case QtWarningMsg:
        logger.logMessage(QString(msg), QuteNoteLogger::Level::LEVEL_WARNING);
        break;
    case QtCriticalMsg:
        logger.logMessage(QString(msg), QuteNoteLogger::Level::LEVEL_CRITICAL);
        break;
    case QtFatalMsg:
    default:
        logger.logMessage(QString(msg), QuteNoteLogger::Level::LEVEL_FATAL);
        abort();
    }
}

#endif

} // namespace qute_note_private
