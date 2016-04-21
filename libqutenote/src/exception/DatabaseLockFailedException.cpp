#include <qute_note/exception/DatabaseLockFailedException.h>

namespace qute_note {

DatabaseLockFailedException::DatabaseLockFailedException(const QString & message) :
    IQuteNoteException(message)
{}

const QString DatabaseLockFailedException::exceptionDisplayName() const
{
    return QString("DatabaseLockFailedException");
}

} // namespace qute_note
