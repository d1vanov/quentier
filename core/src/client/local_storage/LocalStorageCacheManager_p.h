#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H

#include "LocalStorageCacheManager.h"
#include <client/types/Note.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

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

    void installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker);

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
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};
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
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NoteHolder::ByGuid>,
                boost::multi_index::const_mem_fun<NoteHolder,const QString,&NoteHolder::guid>
            >
        >
    > NotesCache;

private:
    QScopedPointer<ILocalStorageCacheExpiryChecker>   m_cacheExpiryChecker;
    NotesCache                  m_notesCache;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
