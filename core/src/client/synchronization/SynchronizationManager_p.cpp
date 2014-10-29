#include "SynchronizationManager_p.h"
#include <keychain.h>
#include <client/local_storage/LocalStorageManagerThread.h>
#include <logging/QuteNoteLogger.h>
#include <QApplication>

namespace qute_note {

SynchronizationManagerPrivate::SynchronizationManagerPrivate(LocalStorageManagerThread & localStorageManagerThread) :
    m_pLastSyncState()
{
    connect(localStorageManagerThread);
    // TODO: implement
}

SynchronizationManagerPrivate::~SynchronizationManagerPrivate()
{}

void SynchronizationManagerPrivate::synchronize()
{
    // TODO: implement
}

void SynchronizationManagerPrivate::connect(LocalStorageManagerThread & localStorageManagerThread)
{
    // TODO: implement
    Q_UNUSED(localStorageManagerThread)
}

void SynchronizationManagerPrivate::authenticate()
{
    // TODO: implement
    // Plan:
    // 1) Check with qtkeychain whether the authentication token is available
    // 2) If not, manually call the slot which should continue the synchronization procedure
    // 3) Otherwise, initialize OAuth authentication using QEverCloud facilities

    QNDEBUG("Trying to restore the authentication token...");

    QKeychain::ReadPasswordJob readPasswordJob("GetAuthenticationToken");
    readPasswordJob.setAutoDelete(false);
    readPasswordJob.setKey(QApplication::applicationName());

    QEventLoop loop;
    readPasswordJob.connect(&readPasswordJob, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
    readPasswordJob.start();
    loop.exec();

    if (readPasswordJob.error()) {
        QNDEBUG("Failed to restore the authentication token, launching the OAuth authorization procedure");
        // TODO: launch the OAuth authorization procedure
    }
    else {
        QNDEBUG("Successfully restored the autnehtication token");
        // TODO: manually invoke the slot which should continue the synchronization process
    }
}

} // namespace qute_note
