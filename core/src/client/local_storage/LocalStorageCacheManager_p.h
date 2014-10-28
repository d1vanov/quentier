#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H

#include "LocalStorageCacheManager.h"
#include <client/types/Note.h>
#include <client/types/Notebook.h>
#include <client/types/Tag.h>
#include <client/types/LinkedNotebook.h>
#include <client/types/SavedSearch.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace qute_note {

class LocalStorageCacheManagerPrivate: public Printable
{
    Q_DECLARE_PUBLIC(LocalStorageCacheManager)
public:
    LocalStorageCacheManagerPrivate(LocalStorageCacheManager & q);
    virtual ~LocalStorageCacheManagerPrivate();

    void clear();
    bool empty() const;

    // Notes cache
    size_t numCachedNotes() const;
    void cacheNote(const Note & note);
    void expungeNote(const Note & note);

    const Note * findNoteByLocalGuid(const QString & localGuid) const;
    const Note * findNoteByGuid(const QString & guid) const;

    // Notebooks cache
    size_t numCachedNotebooks() const;
    void cacheNotebook(const Notebook & notebook);
    void expungeNotebook(const Notebook & notebook);

    const Notebook * findNotebookByLocalGuid(const QString & localGuid) const;
    const Notebook * findNotebookByGuid(const QString & guid) const;

    // Tags cache
    size_t numCachedTags() const;
    void cacheTag(const Tag & tag);
    void expungeTag(const Tag & tag);

    const Tag * findTagByLocalGuid(const QString & localGuid) const;
    const Tag * findTagByGuid(const QString & guid) const;

    // Linked notebooks cache
    size_t numCachedLinkedNotebooks() const;
    void cacheLinkedNotebook(const LinkedNotebook & linkedNotebook);
    void expungeLinkedNotebook(const LinkedNotebook & linkedNotebook);

    const LinkedNotebook * findLinkedNotebookByGuid(const QString & guid) const;

    // Saved searches cache
    size_t numCachedSavedSearches() const;
    void cacheSavedSearch(const SavedSearch & savedSearch);
    void expungeSavedSearch(const SavedSearch & savedSearch);

    const SavedSearch * findSavedSearchByLocalGuid(const QString & localGuid) const;
    const SavedSearch * findSavedSearchByGuid(const QString & guid) const;

    void installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker);

    LocalStorageCacheManager *  q_ptr;

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    LocalStorageCacheManagerPrivate() Q_DECL_DELETE;
    LocalStorageCacheManagerPrivate(const LocalStorageCacheManagerPrivate & other) Q_DECL_DELETE;
    LocalStorageCacheManagerPrivate(LocalStorageCacheManagerPrivate && other) Q_DECL_DELETE;
    LocalStorageCacheManagerPrivate & operator=(const LocalStorageCacheManagerPrivate & other) Q_DECL_DELETE;
    LocalStorageCacheManagerPrivate & operator=(LocalStorageCacheManagerPrivate && other) Q_DECL_DELETE;

private:
    class NoteHolder: public Printable
    {
    public:
        NoteHolder & operator=(const NoteHolder & other);

        Note    m_note;
        qint64  m_lastAccessTimestamp;

        const QString localGuid() const { return m_note.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;
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

    class NotebookHolder: public Printable
    {
    public:
        NotebookHolder & operator=(const NotebookHolder & other);

        Notebook    m_notebook;
        qint64      m_lastAccessTimestamp;

        const QString localGuid() const { return m_notebook.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;
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

    class TagHolder: public Printable
    {
    public:
        TagHolder & operator=(const TagHolder & other);

        Tag     m_tag;
        qint64  m_lastAccessTimestamp;

        const QString localGuid() const { return m_tag.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        TagHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<TagHolder,qint64,&TagHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagHolder::ByLocalGuid>,
                boost::multi_index::const_mem_fun<TagHolder,const QString,&TagHolder::localGuid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder::ByGuid>,
                boost::multi_index::const_mem_fun<TagHolder,const QString,&TagHolder::guid>
            >
        >
    > TagsCache;

    class LinkedNotebookHolder: public Printable
    {
    public:
        LinkedNotebookHolder & operator=(const LinkedNotebookHolder & other);

        LinkedNotebook  m_linkedNotebook;
        qint64          m_lastAccessTimestamp;

        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByGuid{};

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        LinkedNotebookHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<LinkedNotebookHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<LinkedNotebookHolder,qint64,&LinkedNotebookHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LinkedNotebookHolder::ByGuid>,
                boost::multi_index::const_mem_fun<LinkedNotebookHolder,const QString,&LinkedNotebookHolder::guid>
            >
        >
    > LinkedNotebooksCache;

    class SavedSearchHolder: public Printable
    {
    public:
        SavedSearchHolder & operator=(const SavedSearchHolder & other);

        SavedSearch     m_savedSearch;
        qint64          m_lastAccessTimestamp;

        const QString localGuid() const { return m_savedSearch.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        SavedSearchHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<SavedSearchHolder,qint64,&SavedSearchHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<SavedSearchHolder::ByLocalGuid>,
                boost::multi_index::const_mem_fun<SavedSearchHolder,const QString,&SavedSearchHolder::localGuid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchHolder::ByGuid>,
                boost::multi_index::const_mem_fun<SavedSearchHolder,const QString,&SavedSearchHolder::guid>
            >
        >
    > SavedSearchesCache;

private:
    QScopedPointer<ILocalStorageCacheExpiryChecker>   m_cacheExpiryChecker;
    NotesCache              m_notesCache;
    NotebooksCache          m_notebooksCache;
    TagsCache               m_tagsCache;
    LinkedNotebooksCache    m_linkedNotebooksCache;
    SavedSearchesCache      m_savedSearchesCache;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
