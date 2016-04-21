#include <qute_note/exception/DatabaseLockedException.h>

namespace qute_note {

DatabaseLockedException::DatabaseLockedException(const QString & message) :
    IQuteNoteException(message)
{}

const QString DatabaseLockedException::exceptionDisplayName() const
{
    return QString("DatabaseLockedException");
}

} // namespace qute_note
