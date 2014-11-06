#include "LocalStorageCacheManager.h"
#include "LocalStorageCacheManager_p.h"
#include "ILocalStorageCacheExpiryChecker.h"
#include <logging/QuteNoteLogger.h>

namespace qute_note {

LocalStorageCacheManager::LocalStorageCacheManager() :
    d_ptr(new LocalStorageCacheManagerPrivate(*this))
{}

LocalStorageCacheManager::~LocalStorageCacheManager()
{}

void LocalStorageCacheManager::clear()
{
    Q_D(LocalStorageCacheManager);
    d->clear();
}

bool LocalStorageCacheManager::empty() const
{
    Q_D(const LocalStorageCacheManager);
    return d->empty();
}

size_t LocalStorageCacheManager::numCachedNotes() const
{
    Q_D(const LocalStorageCacheManager);
    return d->numCachedNotes();
}

void LocalStorageCacheManager::cacheNote(const Note & note)
{
    Q_D(LocalStorageCacheManager);
    d->cacheNote(note);
}

void LocalStorageCacheManager::expungeNote(const Note & note)
{
    Q_D(LocalStorageCacheManager);
    d->expungeNote(note);
}

#define FIND_OBJECT(Type) \
const Type * LocalStorageCacheManager::find##Type(const QString & guid, const LocalStorageCacheManager::WhichGuid wg) const \
{ \
    Q_D(const LocalStorageCacheManager); \
    \
    switch(wg) {    \
    case LocalGuid: \
        return d->find##Type##ByLocalGuid(guid); \
    case Guid: \
        return d->find##Type##ByGuid(guid); \
    default: \
    { \
        QNCRITICAL("Detected incorrect local/remote guid qualifier in local storage cache manager"); \
        return nullptr; \
    } \
    } \
}

FIND_OBJECT(Note)
FIND_OBJECT(Notebook)
FIND_OBJECT(Tag)
FIND_OBJECT(SavedSearch)

#undef FIND_OBJECT

const Tag * LocalStorageCacheManager::findTagByName(const QString & name) const
{
    Q_D(const LocalStorageCacheManager);
    return d->findTagByName(name.toUpper());
}

size_t LocalStorageCacheManager::numCachedNotebooks() const
{
    Q_D(const LocalStorageCacheManager);
    return d->numCachedNotebooks();
}

void LocalStorageCacheManager::cacheNotebook(const Notebook & notebook)
{
    Q_D(LocalStorageCacheManager);
    d->cacheNotebook(notebook);
}

void LocalStorageCacheManager::expungeNotebook(const Notebook & notebook)
{
    Q_D(LocalStorageCacheManager);
    d->expungeNotebook(notebook);
}

size_t LocalStorageCacheManager::numCachedTags() const
{
    Q_D(const LocalStorageCacheManager);
    return d->numCachedTags();
}

void LocalStorageCacheManager::cacheTag(const Tag & tag)
{
    Q_D(LocalStorageCacheManager);
    d->cacheTag(tag);
}

void LocalStorageCacheManager::expungeTag(const Tag &tag)
{
    Q_D(LocalStorageCacheManager);
    d->expungeTag(tag);
}

size_t LocalStorageCacheManager::numCachedLinkedNotebooks() const
{
    Q_D(const LocalStorageCacheManager);
    return d->numCachedLinkedNotebooks();
}

void LocalStorageCacheManager::cacheLinkedNotebook(const LinkedNotebook & linkedNotebook)
{
    Q_D(LocalStorageCacheManager);
    d->cacheLinkedNotebook(linkedNotebook);
}

void LocalStorageCacheManager::expungeLinkedNotebook(const LinkedNotebook & linkedNotebook)
{
    Q_D(LocalStorageCacheManager);
    d->expungeLinkedNotebook(linkedNotebook);
}

const LinkedNotebook * LocalStorageCacheManager::findLinkedNotebook(const QString & guid) const
{
    Q_D(const LocalStorageCacheManager);
    return d->findLinkedNotebookByGuid(guid);
}

size_t LocalStorageCacheManager::numCachedSavedSearches() const
{
    Q_D(const LocalStorageCacheManager);
    return d->numCachedSavedSearches();
}

void LocalStorageCacheManager::cacheSavedSearch(const SavedSearch & savedSearch)
{
    Q_D(LocalStorageCacheManager);
    d->cacheSavedSearch(savedSearch);
}

void LocalStorageCacheManager::expungeSavedSearch(const SavedSearch & savedSearch)
{
    Q_D(LocalStorageCacheManager);
    d->expungeSavedSearch(savedSearch);
}

void LocalStorageCacheManager::installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker)
{
    Q_D(LocalStorageCacheManager);
    d->installCacheExpiryFunction(checker);
}

QTextStream & LocalStorageCacheManager::Print(QTextStream & strm) const
{
    Q_D(const LocalStorageCacheManager);
    return d->Print(strm);
}

} // namespace qute_note
