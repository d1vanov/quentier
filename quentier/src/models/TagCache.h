#ifndef QUENTIER_MODELS_TAG_CACHE_H
#define QUENTIER_MODELS_TAG_CACHE_H

#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/Tag.h>

namespace quentier {

typedef LRUCache<QString, Tag> TagCache;

} // namespace quentier

#endif // QUENTIER_MODELS_TAG_CACHE_H
