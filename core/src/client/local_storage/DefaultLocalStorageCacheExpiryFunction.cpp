#include "DefaultLocalStorageCacheExpiryFunction.h"
#include "LocalStorageCacheManager.h"

#define MAX_NOTES_TO_STORE 100

namespace qute_note {

bool defaultLocalStorageCacheExpiryFunction(const LocalStorageCacheManager & cacheManager)
{
    // Something simple for now, probably should not attempt to track memory using syscalls, too expensive

    size_t numNotes = cacheManager.numCachedNotes();
    return (numNotes < MAX_NOTES_TO_STORE);
}

} // namespace qute_note
