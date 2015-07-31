#include <qute_note/exception/LocalStorageCacheManagerException.h>

namespace qute_note {

LocalStorageCacheManagerException::LocalStorageCacheManagerException(const QString & message) :
    IQuteNoteException(message)
{}

#ifdef _MSC_VER
LocalStorageCacheManagerException::~LocalStorageCacheManagerException()
#elif __cplusplus >= 201103L
LocalStorageCacheManagerException::~LocalStorageCacheManagerException() noexcept
#else
LocalStorageCacheManagerException::~LocalStorageCacheManagerException() throw()
#endif
{}

const QString LocalStorageCacheManagerException::exceptionDisplayName() const
{
    return "LocalStorageCacheManagerException";
}

} // namespace qute_note
