#include <qute_note/logging/LoggerInitializationException.h>

namespace qute_note {

LoggerInitializationException::LoggerInitializationException(const QString & message) :
    IQuteNoteException(message)
{}

const QString LoggerInitializationException::exceptionDisplayName() const
{
    return "LoggerInitializationException";
}

}
