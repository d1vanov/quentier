#include <quentier/local_storage/ILocalStorageCacheExpiryChecker.h>
#include <quentier/local_storage/LocalStorageCacheManager.h>

namespace quentier {

ILocalStorageCacheExpiryChecker::ILocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager) :
    m_localStorageCacheManager(cacheManager)
{}

ILocalStorageCacheExpiryChecker::~ILocalStorageCacheExpiryChecker()
{}

} // namespace quentier
