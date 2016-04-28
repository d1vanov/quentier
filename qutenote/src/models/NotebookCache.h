#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_CACHE_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_CACHE_H

#include <qute_note/utility/LRUCache.hpp>
#include <qute_note/types/Notebook.h>

namespace qute_note {

typedef LRUCache<QString, Notebook> NotebookCache;

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_CACHE_H
