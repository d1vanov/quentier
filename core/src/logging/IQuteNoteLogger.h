#ifndef __QUTE_NOTE__CORE__LOGGING__I_QUTE_NOTE_LOGGER_H
#define __QUTE_NOTE__CORE__LOGGING__I_QUTE_NOTE_LOGGER_H

#include "../tools/Linkage.h"
#include <QString>
#include <QTextStream>
#include <QScopedPointer>

namespace log4cplus {
QT_FORWARD_DECLARE_CLASS(Logger)
}

namespace qute_note {

class QUTE_NOTE_EXPORT IQuteNoteLogger
{
public:
    IQuteNoteLogger(const QString & name);
    virtual ~IQuteNoteLogger();

    enum class Level
    {
        LEVEL_CRITICAL,
        LEVEL_ERROR,
        LEVEL_WARNING,
        LEVEL_NOTICE,
        LEVEL_STATUS,
        LEVEL_DEBUG,
        LEVEL_DUMP,
        LEVEL_FULLDUMP
    };

    friend QUTE_NOTE_EXPORT QTextStream & operator<<(QTextStream & strm, const IQuteNoteLogger::Level level);

    void LogMessage(const QString & message, const IQuteNoteLogger::Level level);

private:
    IQuteNoteLogger() = delete;
    IQuteNoteLogger(const IQuteNoteLogger & other) = delete;
    IQuteNoteLogger & operator=(const IQuteNoteLogger & other) = delete;

    QString m_loggerName;
    QScopedPointer<log4cplus::Logger> m_pLogger;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__LOGGING__I_QUTE_NOTE_LOGGER_H
