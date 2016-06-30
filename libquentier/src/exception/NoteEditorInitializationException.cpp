#include <quentier/exception/NoteEditorInitializationException.h>

namespace quentier {

NoteEditorInitializationException::NoteEditorInitializationException(const QString & message) :
    IQuentierException(message)
{}

const QString NoteEditorInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorInitializationException");
}

} // namespace quentier
