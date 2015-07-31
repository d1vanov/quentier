#include <qute_note/exception/NoteEditorPluginInitializationException.h>

namespace qute_note {

NoteEditorPluginInitializationException::NoteEditorPluginInitializationException(const QString & message) :
    IQuteNoteException(message)
{}

const QString NoteEditorPluginInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorPluginInitializationException");
}

} // namespace qute_note
