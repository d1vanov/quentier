#include "NullPtrException.h"

namespace qute_note {

NullPtrException::NullPtrException(const QString & message) :
    IQuteNoteException(message)
{}

const QString NullPtrException::exceptionDisplayName() const
{
    return QString("NullPtrException");
}

} // namespace qute_note
