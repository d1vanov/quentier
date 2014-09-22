#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H

#include "LocalStorageCacheManager.h"
#include <client/types/Note.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace std {

template<>
struct less<const QString>
{
   bool operator()(const QString lhs, const QString rhs) const
   {
       return (lhs < rhs);
   }
};

} // namespace std

namespace qute_note {

class LocalStorageCacheManagerPrivate
{
    Q_DECLARE_PUBLIC(LocalStorageCacheManager)
public:
    LocalStorageCacheManagerPrivate(LocalStorageCacheManager & q);
    virtual ~LocalStorageCacheManagerPrivate();

    size_t numCachedNotes() const;
    void cacheNote(const Note & note);

    const Note * findNoteByLocalGuid(const QString & localGuid) const;
    const Note * findNoteByGuid(const QString & guid) const;

    void installCacheExpiryFunction(const LocalStorageCacheManager::CacheExpiryFunction & function);

    LocalStorageCacheManager *  q_ptr;

private:
    LocalStorageCacheManagerPrivate() = delete;
    LocalStorageCacheManagerPrivate(const LocalStorageCacheManagerPrivate & other) = delete;
    LocalStorageCacheManagerPrivate(LocalStorageCacheManagerPrivate && other) = delete;
    LocalStorageCacheManagerPrivate & operator=(const LocalStorageCacheManagerPrivate & other) = delete;
    LocalStorageCacheManagerPrivate & operator=(LocalStorageCacheManagerPrivate && other) = delete;

private:
    struct NoteHolder
    {
        Note    m_note;
        qint64  m_lastAccessTimestamp;

        const QString localGuid() const { return m_note.localGuid(); }

        class Change
        {
        public:
            Change(const NoteHolder & noteHolder) :
                m_noteHolder(noteHolder)
            {}

            Change(const Change & other) :
                m_noteHolder(other.m_noteHolder)
            {}

            Change(Change && other) :
                m_noteHolder(std::move(other.m_noteHolder))
            {}

            ~Change() {}

            void operator()(NoteHolder & noteHolder)
            {
                noteHolder.m_note = m_noteHolder.m_note;
                noteHolder.m_lastAccessTimestamp = m_noteHolder.m_lastAccessTimestamp;
            }

        private:
            Change() = delete;
            Change & operator=(const Change & other) = delete;
            Change & operator=(Change && other) = delete;

        private:
            const NoteHolder &    m_noteHolder;
        };

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};
        struct Linear {};
    };

    typedef boost::multi_index_container<
        NoteHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NoteHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<NoteHolder,qint64,&NoteHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NoteHolder::ByLocalGuid>,
                boost::multi_index::const_mem_fun<NoteHolder,const QString,&NoteHolder::localGuid>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NoteHolder::ByGuid>,
                boost::multi_index::const_mem_fun<Note,const QString &,&Note::guid>
            >,
            boost::multi_index::sequenced<
                boost::multi_index::tag<NoteHolder::Linear>
            >
        >
    > NotesCache;

    typedef NotesCache::index<NoteHolder::Linear>::type LinearIndex;

    typedef LocalStorageCacheManager::CacheExpiryFunction CacheExpiryFunction;

private:
    CacheExpiryFunction *       m_pCacheExpiryFunction;
    NotesCache                  m_notesCache;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
