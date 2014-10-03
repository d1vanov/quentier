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

    virtual QTextStream & Print(QTextStream & strm) const override;

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

        friend QTextStream & operator<<(QTextStream & strm, const NoteHolder & noteHolder)
        {
            strm << "NoteHolder: note = " << noteHolder.m_note << "last access timestamp = " << noteHolder.m_lastAccessTimestamp
                 << " (" << QDateTime::fromMSecsSinceEpoch(noteHolder.m_lastAccessTimestamp).toString(Qt::ISODate) << "); \n";
            return strm;
        }
    };

    typedef boost::multi_index_container<
        NoteHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
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

        friend QTextStream & operator<<(QTextStream & strm, const NotebookHolder & notebookHolder)
        {
            strm << "NotebookHolder: notebook = " << notebookHolder.m_notebook
                 << "last access timestamp = " << notebookHolder.m_lastAccessTimestamp
                 << " (" << QDateTime::fromMSecsSinceEpoch(notebookHolder.m_lastAccessTimestamp).toString(Qt::ISODate) << "); \n";
            return strm;
        }
    };

    typedef boost::multi_index_container<
        NotebookHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
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

    struct TagHolder
    {
        Tag     m_tag;
        qint64  m_lastAccessTimestamp;

        const QString localGuid() const { return m_tag.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        friend QTextStream & operator<<(QTextStream & strm, const TagHolder & tagHolder)
        {
            strm << "TagHolder: tag = " << tagHolder.m_tag << "last access timestamp = " << tagHolder.m_lastAccessTimestamp
                 << " (" << QDateTime::fromMSecsSinceEpoch(tagHolder.m_lastAccessTimestamp).toString(Qt::ISODate) << "); \n";
            return strm;
        }
    };

    typedef boost::multi_index_container<
        TagHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
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

    struct LinkedNotebookHolder
    {
        LinkedNotebook  m_linkedNotebook;
        qint64          m_lastAccessTimestamp;

        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByGuid{};

        friend QTextStream & operator<<(QTextStream & strm, const LinkedNotebookHolder & linkedNotebookHolder)
        {
            strm << "LinkedNotebookHolder: linked notebook = " << linkedNotebookHolder.m_linkedNotebook
                 << "last access timestamp = " << linkedNotebookHolder.m_lastAccessTimestamp << " ("
                 << QDateTime::fromMSecsSinceEpoch(linkedNotebookHolder.m_lastAccessTimestamp).toString(Qt::ISODate) << "); \n";
            return strm;
        }
    };

    typedef boost::multi_index_container<
        LinkedNotebookHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
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

    struct SavedSearchHolder
    {
        SavedSearch     m_savedSearch;
        qint64          m_lastAccessTimestamp;

        const QString localGuid() const { return m_savedSearch.localGuid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalGuid{};
        struct ByGuid{};

        friend QTextStream & operator<<(QTextStream & strm, const SavedSearchHolder & savedSearchHolder)
        {
            strm << "SavedSearchHolder: saved search = " << savedSearchHolder.m_savedSearch
                 << "last access timestamp = " << savedSearchHolder.m_lastAccessTimestamp
                 << "( " << QDateTime::fromMSecsSinceEpoch(savedSearchHolder.m_lastAccessTimestamp).toString(Qt::ISODate) << "); \n";
            return strm;
        }
    };

    typedef boost::multi_index_container<
        SavedSearchHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
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
