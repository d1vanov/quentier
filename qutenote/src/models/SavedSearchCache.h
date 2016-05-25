#ifndef QUTE_NOTE_MODELS_SAVED_SEARCH_CACHE_H
#define QUTE_NOTE_MODELS_SAVED_SEARCH_CACHE_H

#include <qute_note/utility/LRUCache.hpp>
#include <qute_note/types/SavedSearch.h>

namespace qute_note {

typedef LRUCache<QString, SavedSearch> SavedSearchCache;

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_SAVED_SEARCH_CACHE_H
