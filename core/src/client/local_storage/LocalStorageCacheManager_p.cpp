#include "LocalStorageCacheManager_p.h"
#include "DefaultLocalStorageCacheExpiryChecker.h"
#include "LocalStorageCacheManagerException.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QDateTime>

namespace qute_note {

LocalStorageCacheManagerPrivate::LocalStorageCacheManagerPrivate(LocalStorageCacheManager & q) :
    q_ptr(&q),
    m_cacheExpiryChecker(new DefaultLocalStorageCacheExpiryChecker(q)),
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
    QUTE_NOTE_CHECK_PTR(m_cacheExpiryChecker.data());

    typedef boost::multi_index::index<NotesCache,NoteHolder::ByLastAccessTimestamp>::type LastAccessTimestampIndex;
    LastAccessTimestampIndex & latIndex = m_notesCache.get<NoteHolder::ByLastAccessTimestamp>();

    if (Q_LIKELY(!m_cacheExpiryChecker.isNull()))
    {
        bool res = false;
        while(!res && !latIndex.empty())
        {
            res = m_cacheExpiryChecker->checkNotes();
            if (Q_UNLIKELY(!res)) {
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
        throw LocalStorageCacheManagerException("Unable to insert note into the local storage cache");
    }

    QNDEBUG("Added note to the local storage cache: " << note);
}

void LocalStorageCacheManagerPrivate::expungeNote(const Note & note)
{
    bool noteHasGuid = note.hasGuid();
    const QString guid = (noteHasGuid ? note.guid() : note.localGuid());

#define CHECK_AND_EXPUNGE_NOTE(guidType) \
    typedef boost::multi_index::index<NotesCache,NoteHolder::guidType>::type GuidIndex; \
    GuidIndex & index = m_notesCache.get<NoteHolder::guidType>(); \
    GuidIndex::iterator it = index.find(guid); \
    if (it != index.end()) { \
        index.erase(it); \
        QNDEBUG("Expunged note from the local storage cache: " << note); \
    }

    if (noteHasGuid)  {
        CHECK_AND_EXPUNGE_NOTE(ByGuid)
    }
    else {
        CHECK_AND_EXPUNGE_NOTE(ByLocalGuid)
    }

#undef CHECK_AND_EXPUNGE_NOTE
}

const Note * LocalStorageCacheManagerPrivate::findNoteByLocalGuid(const QString & localGuid) const
{
    const auto & index = m_notesCache.get<NoteHolder::ByLocalGuid>();
    auto it = index.find(localGuid);
    if (it == index.end()) {
        return nullptr;
    }

    return &(it->m_note);
}

const Note * LocalStorageCacheManagerPrivate::findNoteByGuid(const QString & guid) const
{
    const auto & index = m_notesCache.get<NoteHolder::ByGuid>();
    auto it = index.find(guid);
    if (it == index.end()) {
        return nullptr;
    }

    return &(it->m_note);
}

void LocalStorageCacheManagerPrivate::installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker)
{
    m_cacheExpiryChecker.reset(checker.clone());
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
