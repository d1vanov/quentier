#include "IQuteNoteLogger.h"
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

namespace qute_note {

IQuteNoteLogger::IQuteNoteLogger(const QString & name) :
    m_loggerName(name),
    m_pLogger(new log4cplus::Logger(log4cplus::Logger::getInstance(name.toStdString())))
{
    // TODO: figure out how to set this thing up... it looks like a tricky business :(
}

IQuteNoteLogger::~IQuteNoteLogger()
{}

QTextStream & operator <<(QTextStream & strm, const IQuteNoteLogger::Level level)
{
    // TODO: implement
    return strm;
}

void IQuteNoteLogger::LogMessage(const QString & /* message */,
                                 const IQuteNoteLogger::Level /* level */)
{
    // TODO: implement
}



} // namespace qute_note
