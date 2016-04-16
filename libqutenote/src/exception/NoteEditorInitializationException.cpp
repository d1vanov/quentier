#include <qute_note/exception/NoteEditorInitializationException.h>

namespace qute_note {

NoteEditorInitializationException::NoteEditorInitializationException(const QString & message) :
    IQuteNoteException(message)
{}

const QString NoteEditorInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorInitializationException");
}

} // namespace qute_note
