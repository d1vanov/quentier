#ifndef QUTE_NOTE_MODELS_TAG_CACHE_H
#define QUTE_NOTE_MODELS_TAG_CACHE_H

#include <qute_note/utility/LRUCache.hpp>
#include <qute_note/types/Tag.h>

namespace qute_note {

typedef LRUCache<QString, Tag> TagCache;

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_TAG_CACHE_H
