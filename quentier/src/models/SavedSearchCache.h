#ifndef QUENTIER_MODELS_SAVED_SEARCH_CACHE_H
#define QUENTIER_MODELS_SAVED_SEARCH_CACHE_H

#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/SavedSearch.h>

namespace quentier {

typedef LRUCache<QString, SavedSearch> SavedSearchCache;

} // namespace quentier

#endif // QUENTIER_MODELS_SAVED_SEARCH_CACHE_H
