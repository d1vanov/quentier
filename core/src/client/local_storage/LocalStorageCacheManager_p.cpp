#include "LocalStorageCacheManager_p.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QDateTime>

namespace qute_note {

LocalStorageCacheManagerPrivate::LocalStorageCacheManagerPrivate(LocalStorageCacheManager & q) :
    q_ptr(&q),
    m_pCacheExpiryFunction(nullptr),
    m_notesCache()
{
    // FIXME: install default cache expiry function
}

LocalStorageCacheManagerPrivate::~LocalStorageCacheManagerPrivate()
{}

size_t LocalStorageCacheManagerPrivate::numCachedNotes() const
{
    const auto & index = m_notesCache.get<NoteHolder::ByLocalGuid>();
    return index.size();
}

void LocalStorageCacheManagerPrivate::cacheNote(const Note & note)
{
    QUTE_NOTE_CHECK_PTR(m_pCacheExpiryFunction);
    Q_Q(LocalStorageCacheManager);

    typedef boost::multi_index::index<NotesCache,NoteHolder::ByLastAccessTimestamp>::type LastAccessTimestampIndex;
    LastAccessTimestampIndex & latIndex = m_notesCache.get<NoteHolder::ByLastAccessTimestamp>();

    bool res = false;
    while(!res && !latIndex.empty())
    {
        res = (*m_pCacheExpiryFunction)(*q);
        if (!res) {
            Q_UNUSED(m_notesCache.erase(latIndex.rbegin().base()));
            continue;
        }
    }

    NoteHolder noteHolder;
    noteHolder.m_note = note;
    noteHolder.m_lastAccessTimestamp = QDateTime::currentMSecsSinceEpoch();

    // See whether the note is already in the cache
    typedef boost::multi_index::index<NotesCache,NoteHolder::ByLocalGuid>::type LocalGuidIndex;
    LocalGuidIndex & lgIndex = m_notesCache.get<NoteHolder::ByLocalGuid>();
    LocalGuidIndex::iterator it = lgIndex.find(note.localGuid());
    if (it != lgIndex.end()) {
        lgIndex.modify(it, NoteHolder::Modifier(noteHolder));
        QNDEBUG("Updated note in the local storage cache: " << note);
        return;
    }

    // If got here, no existing note was found in the cache
    auto insertionResult = m_notesCache.insert(noteHolder);
    if (!insertionResult.second) {
        QNWARNING("Failed to insert note into the cache of local storage manager's notes: "
                  << note);
        // TODO: throw exception
        return;
    }

    QNDEBUG("Added note to the local storage cache: " << note);
}

} // namespace qute_note
