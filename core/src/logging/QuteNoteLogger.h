#ifndef __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
#define __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H

#include "../tools/Linkage.h"
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QApplication>

// TODO: add some macro for qInstallMessageHandler

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

void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString & message);

}

#define QNDEBUG(message) \
    qDebug() << message;

#define QNWARNING(message) \
    qWarning() << message;

#define QNCRITICAL(message) \
    qCritical() << message;

#define QNFATAL(message) \
    qFatal() << message;

#else

namespace qute_note_private {

void messageHandler(QtMsgType type, const char * msg);

}

#if defined ( WIN32 )
#define __func__ __FUNCTION__
#endif

#define QNDEBUG(message) \
    qDebug() << __FILE__ << ": " << __LINE__ << ": " __func__ ": " << message;

#define QNWARNING(message) \
    qWarning() << __FILE__ << ": " << __LINE__ << ": " __func__ ": " << message;

#define QNCRITICAL(message) \
    qCritical() << __FILE__ << ": " << __LINE__ << ": " __func__ ": " << message;

#define QNFATAL(message) \
    qFatal() << __FILE__ << ": " << __LINE__ << ": " __func__ ": " << message;

#endif

#endif // __QUTE_NOTE__CORE__LOGGING__QUTE_NOTE_LOGGER_H
