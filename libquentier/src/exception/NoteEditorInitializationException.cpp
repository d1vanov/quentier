#include <quentier/exception/NoteEditorInitializationException.h>

namespace quentier {

NoteEditorInitializationException::NoteEditorInitializationException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString NoteEditorInitializationException::exceptionDisplayName() const
{
    return QString("NoteEditorInitializationException");
}

} // namespace quentier
