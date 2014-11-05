#include "SynchronizationManager_p.h"
#include <tools/QuteNoteCheckPtr.h>
#include <tools/Printable.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {

void SynchronizationManagerPrivate::launchFullSync()
{
    QNDEBUG("SynchronizationManagerPrivate::launchFullSync");

    QUTE_NOTE_CHECK_PTR(m_pNoteStore.data());
    QUTE_NOTE_CHECK_PTR(m_pOAuthResult.data());

    qint32 afterUsn = 0;
    std::vector<qevercloud::SyncChunk> syncChunks;
    qevercloud::SyncChunk * pSyncChunk = nullptr;

    while(!pSyncChunk || (pSyncChunk->chunkHighUSN < pSyncChunk->updateCount))
    {
        if (pSyncChunk) {
            afterUsn = pSyncChunk->chunkHighUSN;
        }

        syncChunks.push_back(qevercloud::SyncChunk());
        pSyncChunk = &(syncChunks.back());
        *pSyncChunk = m_pNoteStore->getSyncChunk(afterUsn, m_maxSyncChunkEntries, true,
                                                 m_pOAuthResult->authenticationToken);
        QNDEBUG("Received sync chunk: " << *pSyncChunk);
    }

    // TODO: continue from here
}

} // namespace qute_note
