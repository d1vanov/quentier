#include <quentier/exception/NoteEditorPluginInitializationException.h>

namespace quentier {

NoteEditorPluginInitializationException::NoteEditorPluginInitializationException(const QString & message) :
    IQuentierException(message)
{}

const QString NoteEditorPluginInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorPluginInitializationException");
}

} // namespace quentier
