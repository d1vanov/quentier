#include "LocalStorageCacheManagerException.h"

namespace qute_note {

LocalStorageCacheManagerException::LocalStorageCacheManagerException(const QString & message) :
    IQuteNoteException(message)
{}

LocalStorageCacheManagerException::~LocalStorageCacheManagerException()
{}

const QString LocalStorageCacheManagerException::exceptionDisplayName() const
{
    return "LocalStorageCacheManagerException";
}

} // namespace qute_note
