#include <quentier/exception/NoteEditorPluginInitializationException.h>

namespace quentier {

NoteEditorPluginInitializationException::NoteEditorPluginInitializationException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString NoteEditorPluginInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorPluginInitializationException");
}

} // namespace quentier
