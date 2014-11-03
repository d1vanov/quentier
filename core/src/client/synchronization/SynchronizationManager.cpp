#include "SynchronizationManager.h"
#include "SynchronizationManager_p.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

SynchronizationManager::SynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    d_ptr(new SynchronizationManagerPrivate(localStorageManagerThreadWorker))
{}

SynchronizationManager::~SynchronizationManager()
{
    if (d_ptr) {
        delete d_ptr;
    }
}

void SynchronizationManager::synchronize()
{
    Q_D(SynchronizationManager);
    d->synchronize();
}

} // namespace qute_note
