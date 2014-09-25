#include "DefaultLocalStorageCacheExpiryChecker.h"
#include "LocalStorageCacheManager.h"

#define MAX_NOTES_TO_STORE 100
#define MAX_NOTEBOOKS_TO_STORE 20

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

// Probably should not attempt to track memory using syscalls for the following methods, too expensive

bool DefaultLocalStorageCacheExpiryChecker::checkNotes() const
{
    size_t numNotes = m_localStorageCacheManager.numCachedNotes();
    return (numNotes < MAX_NOTES_TO_STORE);
}

bool DefaultLocalStorageCacheExpiryChecker::checkNotebooks() const
{
    size_t numNotebooks = m_localStorageCacheManager.numCachedNotebooks();
    return (numNotebooks < MAX_NOTEBOOKS_TO_STORE);
}

} // namespace qute_note
