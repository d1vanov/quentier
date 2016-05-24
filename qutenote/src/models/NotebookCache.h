#ifndef QUTE_NOTE_MODELS_NOTEBOOK_CACHE_H
#define QUTE_NOTE_MODELS_NOTEBOOK_CACHE_H

#include <qute_note/utility/LRUCache.hpp>
#include <qute_note/types/Notebook.h>

namespace qute_note {

typedef LRUCache<QString, Notebook> NotebookCache;

} // namespace qute_note

#endif // QUTE_NOTE_MODELS_NOTEBOOK_CACHE_H
