#include <qute_note/local_storage/ILocalStorageCacheExpiryChecker.h>
#include "LocalStorageCacheManager.h"

namespace qute_note {

ILocalStorageCacheExpiryChecker::ILocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager) :
    m_localStorageCacheManager(cacheManager)
{}

ILocalStorageCacheExpiryChecker::~ILocalStorageCacheExpiryChecker()
{}

} // namespace qute_note
