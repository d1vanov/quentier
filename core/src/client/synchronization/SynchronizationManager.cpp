#include "SynchronizationManager.h"
#include "SynchronizationManager_p.h"
#include <client/local_storage/LocalStorageManagerThreadWorker.h>

namespace qute_note {

SynchronizationManager::SynchronizationManager(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    d_ptr(new SynchronizationManagerPrivate(localStorageManagerThreadWorker))
{
    QObject::connect(d_ptr, SIGNAL(notifyError(QString)), this, SIGNAL(failed(QString)));
    QObject::connect(d_ptr, SIGNAL(notifyFinish()), this, SIGNAL(finished()));
    QObject::connect(d_ptr, SIGNAL(remoteToLocalSyncPaused(bool)), this, SIGNAL(remoteToLocalSyncPaused(bool)));
    QObject::connect(d_ptr, SIGNAL(remoteToLocalSyncStopped()), this, SIGNAL(remoteToLocalSyncStopped()));
    QObject::connect(d_ptr, SIGNAL(sendLocalChangesPaused(bool)), this, SIGNAL(sendLocalChangesPaused(bool)));
    QObject::connect(d_ptr, SIGNAL(sendLocalChangesStopped()), this, SIGNAL(sendLocalChangesStopped()));
    QObject::connect(d_ptr, SIGNAL(willRepeatRemoteToLocalSyncAfterSendingChanges()), this, SIGNAL(willRepeatRemoteToLocalSyncAfterSendingChanges()));
    QObject::connect(d_ptr, SIGNAL(detectedConflictDuringLocalChangesSending()), this, SIGNAL(detectedConflictDuringLocalChangesSending()));
    QObject::connect(d_ptr, SIGNAL(rateLimitExceeded(qint32)), this, SIGNAL(rateLimitExceeded(qint32)));
    QObject::connect(d_ptr, SIGNAL(notifyRemoteToLocalSyncDone()), this, SIGNAL(remoteToLocalSyncDone()));
    QObject::connect(d_ptr, SIGNAL(progress(QString,double)), this, SIGNAL(progress(QString,double)));
}

SynchronizationManager::~SynchronizationManager()
{
    delete d_ptr;
}

bool SynchronizationManager::active() const
{
    Q_D(const SynchronizationManager);
    return d->active();
}

bool SynchronizationManager::paused() const
{
    Q_D(const SynchronizationManager);
    return d->paused();
}

void SynchronizationManager::synchronize()
{
    Q_D(SynchronizationManager);
    d->synchronize();
}

void SynchronizationManager::pause()
{
    Q_D(SynchronizationManager);
    d->pause();
}

void SynchronizationManager::resume()
{
    Q_D(SynchronizationManager);
    d->resume();
}

void SynchronizationManager::stop()
{
    Q_D(SynchronizationManager);
    d->stop();
}

} // namespace qute_note
