#ifndef __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
#define __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H

#include "../tools/Linkage.h"
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QApplication>

namespace qute_note {

class QUTE_NOTE_EXPORT QuteNoteLogger
{
public:
    enum class Level
    {
        LEVEL_FATAL,
        LEVEL_CRITICAL,
        LEVEL_WARNING,
        LEVEL_DEBUG
    };

    friend QUTE_NOTE_EXPORT QTextStream & operator<<(QTextStream & strm, const QuteNoteLogger::Level level);

    static QuteNoteLogger & instance(const QString & name);

    void setLogLevel(const Level level);
    void logMessage(const QString & message, const QuteNoteLogger::Level level);

private:
    QuteNoteLogger(const QString & name, const Level level = QuteNoteLogger::Level::LEVEL_WARNING);
    virtual ~QuteNoteLogger();

    QuteNoteLogger() = delete;
    QuteNoteLogger(const QuteNoteLogger & other) = delete;
    QuteNoteLogger & operator=(const QuteNoteLogger & other) = delete;

    QString m_loggerName;
    QFile   m_logFile;
    Level   m_logLevel;

};

} // namespace qute_note

#if QT_VERSION >= 0x050000

namespace qute_note_private {

void QUTE_NOTE_EXPORT messageHandler(QtMsgType type, const QMessageLogContext & context, const QString & message);

}

#define QNDEBUG(message) \
    qDebug() << message;

#define QNWARNING(message) \
    qWarning() << message;

#define QNCRITICAL(message) \
    qCritical() << message;

#define QNFATAL(message) \
    qFatal() << message;

#define QUTE_NOTE_INITIALIZE_LOGGING() \
    qInstallMessageHandler(qute_note_private::messageHandler);

#else

namespace qute_note_private {

void QUTE_NOTE_EXPORT messageHandler(QtMsgType type, const char * msg);

}

#if defined ( WIN32 )
#define __func__ __FUNCTION__
#endif

#define __QNMESSAGE(message) \
    ", file: "<< __FILE__ << ", line: " << __LINE__ << ", function: " << __func__ << ", message: " << message;

#define QNDEBUG(message) \
    qDebug() << __QNMESSAGE(message)

#define QNWARNING(message) \
    qWarning() << __QNMESSAGE(message)

#define QNCRITICAL(message) \
    qCritical() << __QNMESSAGE(message)

#define QNFATAL(message) \
    qFatal() << __QNMESSAGE(message)

#define QUTE_NOTE_INITIALIZE_LOGGING() \
    qInstallMsgHandler(qute_note_private::messageHandler);

#endif

#define __QNMAINLOGGER \
    qute_note::QuteNoteLogger::instance("main")

#define QUTE_NOTE_SET_MIN_LOG_LEVEL(level) \
    __QNMAINLOGGER.setLogLevel(level);

#endif // __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
