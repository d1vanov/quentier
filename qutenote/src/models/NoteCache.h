#ifndef QUTE_NOTE_MODELS_NOTE_CACHE_H
#define QUTE_NOTE_MODELS_NOTE_CACHE_H

#include <qute_note/utility/LRUCache.hpp>
#include <qute_note/types/Note.h>

namespace qute_note {

typedef LRUCache<QString, Note> NoteCache;

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_NOTE_CACHE_H
