#include "QuteNoteNullPtrException.h"

namespace qute_note {

QuteNoteNullPtrException::QuteNoteNullPtrException(const QString & message) :
    IQuteNoteException(message)
{}

const QString QuteNoteNullPtrException::exceptionDisplayName() const
{
    return QString("QuteNoteNullPtrException");
}

} // namespace qute_note
