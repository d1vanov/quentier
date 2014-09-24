#include "DefaultLocalStorageCacheExpiryChecker.h"
#include "LocalStorageCacheManager.h"

#define MAX_NOTES_TO_STORE 100

namespace qute_note {

DefaultLocalStorageCacheExpiryChecker::DefaultLocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager) :
    ILocalStorageCacheExpiryChecker(cacheManager)
{}

DefaultLocalStorageCacheExpiryChecker::~DefaultLocalStorageCacheExpiryChecker()
{}

DefaultLocalStorageCacheExpiryChecker * DefaultLocalStorageCacheExpiryChecker::clone() const
{
    return new DefaultLocalStorageCacheExpiryChecker(m_localStorageCacheManager);
}

bool DefaultLocalStorageCacheExpiryChecker::checkNotes() const
{
    // Probably should not attempt to track memory using syscalls, too expensive
    size_t numNotes = m_localStorageCacheManager.numCachedNotes();
    return (numNotes < MAX_NOTES_TO_STORE);
}

} // namespace qute_note
