#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H

#include "LocalStorageCacheManager.h"
#include <client/types/Note.h>
#include <client/types/Notebook.h>
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
    void expungeNote(const Note & note);

    const Note * findNoteByLocalGuid(const QString & localGuid) const;
    const Note * findNoteByGuid(const QString & guid) const;


    size_t numCachedNotebooks() const;
    void cacheNotebook(const Notebook & notebook);
    void expungeNotebook(const Notebook & notebook);

    const Notebook * findNotebookByLocalGuid(const QString & localGuid) const;
    const Notebook * findNotebookByGuid(const QString & guid) const;

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
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NoteHolder::ByGuid>,
                boost::multi_index::const_mem_fun<NoteHolder,const QString,&NoteHolder::guid>
            >
        >
    > NotesCache;

    struct NotebookHolder
    {
        Notebook    m_notebook;
        qint64      m_lastAccessTimestamp;

        const QString localGuid() const { return m_notebook.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};
    };

    typedef boost::multi_index_container<
        NotebookHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NotebookHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<NotebookHolder,qint64,&NotebookHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NotebookHolder::ByLocalGuid>,
                boost::multi_index::const_mem_fun<NotebookHolder,const QString,&NotebookHolder::localGuid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NotebookHolder::ByGuid>,
                boost::multi_index::const_mem_fun<NotebookHolder,const QString,&NotebookHolder::guid>
            >
        >
    > NotebooksCache;

private:
    QScopedPointer<ILocalStorageCacheExpiryChecker>   m_cacheExpiryChecker;
    NotesCache      m_notesCache;
    NotebooksCache  m_notebooksCache;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
