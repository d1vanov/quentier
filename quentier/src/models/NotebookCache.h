#ifndef QUENTIER_MODELS_NOTEBOOK_CACHE_H
#define QUENTIER_MODELS_NOTEBOOK_CACHE_H

#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/Notebook.h>

namespace quentier {

typedef LRUCache<QString, Notebook> NotebookCache;

} // namespace quentier

#endif // QUENTIER_MODELS_NOTEBOOK_CACHE_H
