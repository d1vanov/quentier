#include <qute_note/exception/LocalStorageCacheManagerException.h>

namespace qute_note {

LocalStorageCacheManagerException::LocalStorageCacheManagerException(const QString & message) :
    IQuteNoteException(message)
{}

const QString LocalStorageCacheManagerException::exceptionDisplayName() const
{
    return "LocalStorageCacheManagerException";
}

} // namespace qute_note
