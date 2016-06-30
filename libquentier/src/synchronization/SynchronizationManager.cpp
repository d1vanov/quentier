#include <quentier/synchronization/SynchronizationManager.h>
#include "SynchronizationManager_p.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>

namespace quentier {

SynchronizationManager::SynchronizationManager(const QString & consumerKey, const QString & consumerSecret,
                                               LocalStorageManagerThreadWorker & localStorageManagerThreadWorker) :
    d_ptr(new SynchronizationManagerPrivate(consumerKey, consumerSecret, localStorageManagerThreadWorker))
{
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, notifyError, QString), this, QNSIGNAL(SynchronizationManager, failed, QString));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, notifyFinish), this, QNSIGNAL(SynchronizationManager, finished));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, remoteToLocalSyncPaused, bool), this, QNSIGNAL(SynchronizationManager, remoteToLocalSyncPaused, bool));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, remoteToLocalSyncStopped), this, QNSIGNAL(SynchronizationManager, remoteToLocalSyncStopped));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, sendLocalChangesPaused, bool), this, QNSIGNAL(SynchronizationManager, sendLocalChangesPaused, bool));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, sendLocalChangesStopped), this, QNSIGNAL(SynchronizationManager, sendLocalChangesStopped));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, willRepeatRemoteToLocalSyncAfterSendingChanges), this, QNSIGNAL(SynchronizationManager, willRepeatRemoteToLocalSyncAfterSendingChanges));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, detectedConflictDuringLocalChangesSending), this, QNSIGNAL(SynchronizationManager, detectedConflictDuringLocalChangesSending));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, rateLimitExceeded, qint32), this, QNSIGNAL(SynchronizationManager, rateLimitExceeded, qint32));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, notifyRemoteToLocalSyncDone), this, QNSIGNAL(SynchronizationManager, remoteToLocalSyncDone));
    QObject::connect(d_ptr, QNSIGNAL(SynchronizationManagerPrivate, progress,QString,double), this, QNSIGNAL(SynchronizationManager, progress,QString,double));
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

} // namespace quentier
