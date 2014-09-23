#include "LocalStorageCacheManager_p.h"
#include "DefaultLocalStorageCacheExpiryFunction.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QDateTime>

namespace qute_note {

LocalStorageCacheManagerPrivate::LocalStorageCacheManagerPrivate(LocalStorageCacheManager & q) :
    q_ptr(&q),
    m_cacheExpiryFunction(defaultLocalStorageCacheExpiryFunction),
    m_notesCache()
{}

LocalStorageCacheManagerPrivate::~LocalStorageCacheManagerPrivate()
{}

size_t LocalStorageCacheManagerPrivate::numCachedNotes() const
{
    const auto & index = m_notesCache.get<NoteHolder::ByLocalGuid>();
    return index.size();
}

void LocalStorageCacheManagerPrivate::cacheNote(const Note & note)
{
    QUTE_NOTE_CHECK_PTR(m_cacheExpiryFunction);
    Q_Q(LocalStorageCacheManager);

    typedef boost::multi_index::index<NotesCache,NoteHolder::ByLastAccessTimestamp>::type LastAccessTimestampIndex;
    LastAccessTimestampIndex & latIndex = m_notesCache.get<NoteHolder::ByLastAccessTimestamp>();

    if (Q_LIKELY(m_cacheExpiryFunction))
    {
        bool res = false;
        while(!res && !latIndex.empty())
        {
            res = m_cacheExpiryFunction(*q);
            if (Q_UNLIKELY(!res)) {
                // FIXME: this may have nothing to do with notes, need to figure this out somehow
                Q_UNUSED(m_notesCache.erase(latIndex.rbegin().base()));
                continue;
            }
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
        lgIndex.replace(it, noteHolder);
        QNDEBUG("Updated note in the local storage cache: " << note);
        return;
    }

    // If got here, no existing note was found in the cache
    auto insertionResult = m_notesCache.insert(noteHolder);
    if (Q_UNLIKELY(!insertionResult.second)) {
        QNWARNING("Failed to insert note into the cache of local storage manager's notes: "
                  << note);
        // TODO: throw exception
        return;
    }

    QNDEBUG("Added note to the local storage cache: " << note);
}

const Note * LocalStorageCacheManagerPrivate::findNoteByLocalGuid(const QString & localGuid) const
{
    // TODO: implement
    Q_UNUSED(localGuid)
    return nullptr;
}

const Note * LocalStorageCacheManagerPrivate::findNoteByGuid(const QString & guid) const
{
    // TODO: implement
    Q_UNUSED(guid)
    return nullptr;
}

void LocalStorageCacheManagerPrivate::installCacheExpiryFunction(const LocalStorageCacheManager::CacheExpiryFunction & function)
{
    m_cacheExpiryFunction = function;
}

const QString LocalStorageCacheManagerPrivate::NoteHolder::guid() const
{
    // NOTE: This precaution is required for storage of local notes in the cache
    if (m_note.hasGuid()) {
        return m_note.guid();
    }
    else {
        return QString();
    }
}

} // namespace qute_note
