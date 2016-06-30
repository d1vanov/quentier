#ifndef QUENTIER_MODELS_NOTE_CACHE_H
#define QUENTIER_MODELS_NOTE_CACHE_H

#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/Note.h>

namespace quentier {

typedef LRUCache<QString, Note> NoteCache;

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_CACHE_H
