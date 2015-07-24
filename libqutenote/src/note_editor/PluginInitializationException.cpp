#include "PluginInitializationException.h"

namespace qute_note {

PluginInitializationException::PluginInitializationException(const QString & message) :
    IQuteNoteException(message)
{}

const QString PluginInitializationException::exceptionDisplayName() const
{
    return QString("PluginInitializationException");
}

} // namespace qute_note
