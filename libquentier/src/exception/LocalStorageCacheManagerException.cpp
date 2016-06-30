#include <quentier/exception/LocalStorageCacheManagerException.h>

namespace quentier {

LocalStorageCacheManagerException::LocalStorageCacheManagerException(const QString & message) :
    IQuentierException(message)
{}

const QString LocalStorageCacheManagerException::exceptionDisplayName() const
{
    return "LocalStorageCacheManagerException";
}

} // namespace quentier
