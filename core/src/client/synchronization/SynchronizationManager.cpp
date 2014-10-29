#include "SynchronizationManager.h"
#include "SynchronizationManager_p.h"
#include <client/local_storage/LocalStorageManagerThread.h>

namespace qute_note {

SynchronizationManager::SynchronizationManager(LocalStorageManagerThread & localStorageManagerThread) :
    d_ptr(new SynchronizationManagerPrivate(localStorageManagerThread))
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
