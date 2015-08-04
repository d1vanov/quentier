#include <qute_note/exception/ApplicationSettingsInitializationException.h>

namespace qute_note {

ApplicationSettingsInitializationException::ApplicationSettingsInitializationException(const QString & message) :
    IQuteNoteException(message)
{}

const QString ApplicationSettingsInitializationException::exceptionDisplayName() const
{
    return QString("ApplicationSettingsInitializationException");
}

} // namespace qute_note
