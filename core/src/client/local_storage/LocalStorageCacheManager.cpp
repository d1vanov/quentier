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

const Note * LocalStorageCacheManager::findNote(const QString & guid, const LocalStorageCacheManager::WhichGuid wg) const
{
    Q_D(const LocalStorageCacheManager);

    switch(wg) {
    case LocalGuid:
        return d->findNoteByLocalGuid(guid);
    case Guid:
        return d->findNoteByGuid(guid);
    default:
    {
        QNCRITICAL("Detected incorrect local/remote guid qualifier in local storage cache manager");
        return nullptr;
    }
    }
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

const Notebook * LocalStorageCacheManager::findNotebook(const QString & guid, const LocalStorageCacheManager::WhichGuid wg) const
{
    Q_D(const LocalStorageCacheManager);

    switch(wg) {
    case LocalGuid:
        return d->findNotebookByLocalGuid(guid);
    case Guid:
        return d->findNotebookByGuid(guid);
    default:
    {
        QNCRITICAL("Detected incorrect local/remote guid qualifier in local storage cache manager");
        return nullptr;
    }
    }
}

void LocalStorageCacheManager::installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker)
{
    Q_D(LocalStorageCacheManager);
    d->installCacheExpiryFunction(checker);
}

} // namespace qute_note
