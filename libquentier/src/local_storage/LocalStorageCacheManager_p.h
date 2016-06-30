#ifndef LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
#define LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H

#include <quentier/local_storage/LocalStorageCacheManager.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Tag.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/SavedSearch.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace quentier {

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

    const Note * findNoteByLocalUid(const QString & localUid) const;
    const Note * findNoteByGuid(const QString & guid) const;

    // Notebooks cache
    size_t numCachedNotebooks() const;
    void cacheNotebook(const Notebook & notebook);
    void expungeNotebook(const Notebook & notebook);

    const Notebook * findNotebookByLocalUid(const QString & localUid) const;
    const Notebook * findNotebookByGuid(const QString & guid) const;
    const Notebook * findNotebookByName(const QString & name) const;

    // Tags cache
    size_t numCachedTags() const;
    void cacheTag(const Tag & tag);
    void expungeTag(const Tag & tag);

    const Tag * findTagByLocalUid(const QString & localUid) const;
    const Tag * findTagByGuid(const QString & guid) const;
    const Tag * findTagByName(const QString & name) const;

    // Linked notebooks cache
    size_t numCachedLinkedNotebooks() const;
    void cacheLinkedNotebook(const LinkedNotebook & linkedNotebook);
    void expungeLinkedNotebook(const LinkedNotebook & linkedNotebook);

    const LinkedNotebook * findLinkedNotebookByGuid(const QString & guid) const;

    // Saved searches cache
    size_t numCachedSavedSearches() const;
    void cacheSavedSearch(const SavedSearch & savedSearch);
    void expungeSavedSearch(const SavedSearch & savedSearch);

    const SavedSearch * findSavedSearchByLocalUid(const QString & localUid) const;
    const SavedSearch * findSavedSearchByGuid(const QString & guid) const;
    const SavedSearch * findSavedSearchByName(const QString & name) const;

    void installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker);

    LocalStorageCacheManager *  q_ptr;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

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

        const QString localUid() const { return m_note.localUid(); }
        const QString guid() const;

        struct ByLastAccessTimestamp{};
        struct ByLocalUid{};
        struct ByGuid{};

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        NoteHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NoteHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<NoteHolder,qint64,&NoteHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NoteHolder::ByLocalUid>,
                boost::multi_index::const_mem_fun<NoteHolder,const QString,&NoteHolder::localUid>
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

        const QString localUid() const { return m_notebook.localUid(); }
        const QString guid() const;
        const QString nameUpper() const { return (m_notebook.hasName()
                                                  ? m_notebook.name().toUpper()
                                                  : QString()); }

        struct ByLastAccessTimestamp{};
        struct ByLocalUid{};
        struct ByGuid{};
        struct ByName{};

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        NotebookHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NotebookHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<NotebookHolder,qint64,&NotebookHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<NotebookHolder::ByLocalUid>,
                boost::multi_index::const_mem_fun<NotebookHolder,const QString,&NotebookHolder::localUid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NotebookHolder::ByGuid>,
                boost::multi_index::const_mem_fun<NotebookHolder,const QString,&NotebookHolder::guid>
            >,
            /* NOTE: non-unique for proper support of empty names */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<NotebookHolder::ByName>,
                boost::multi_index::const_mem_fun<NotebookHolder,const QString,&NotebookHolder::nameUpper>
            >
        >
    > NotebooksCache;

    class TagHolder: public Printable
    {
    public:
        TagHolder & operator=(const TagHolder & other);

        Tag     m_tag;
        qint64  m_lastAccessTimestamp;

        const QString localUid() const { return m_tag.localUid(); }
        const QString guid() const;
        const QString nameUpper() const { return (m_tag.hasName()
                                                  ? m_tag.name().toUpper()
                                                  : QString()); }

        struct ByLastAccessTimestamp{};
        struct ByLocalUid{};
        struct ByGuid{};
        struct ByName{};

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        TagHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<TagHolder,qint64,&TagHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagHolder::ByLocalUid>,
                boost::multi_index::const_mem_fun<TagHolder,const QString,&TagHolder::localUid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder::ByGuid>,
                boost::multi_index::const_mem_fun<TagHolder,const QString,&TagHolder::guid>
            >,
            /* NOTE: non-unique for proper support of empty names */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder::ByName>,
                boost::multi_index::const_mem_fun<TagHolder,const QString,&TagHolder::nameUpper>
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

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
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

        const QString localUid() const { return m_savedSearch.localUid(); }
        const QString guid() const;
        const QString nameUpper() const { return (m_savedSearch.hasName()
                                                  ? m_savedSearch.name().toUpper()
                                                  : QString()); }

        struct ByLastAccessTimestamp{};
        struct ByLocalUid{};
        struct ByGuid{};
        struct ByName{};

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;
    };

    typedef boost::multi_index_container<
        SavedSearchHolder,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchHolder::ByLastAccessTimestamp>,
                boost::multi_index::member<SavedSearchHolder,qint64,&SavedSearchHolder::m_lastAccessTimestamp>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<SavedSearchHolder::ByLocalUid>,
                boost::multi_index::const_mem_fun<SavedSearchHolder,const QString,&SavedSearchHolder::localUid>
            >,
            /* NOTE: non-unique for proper support of empty guids */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchHolder::ByGuid>,
                boost::multi_index::const_mem_fun<SavedSearchHolder,const QString,&SavedSearchHolder::guid>
            >,
            /* NOTE: non-unique for proper support of empty names */
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<SavedSearchHolder::ByName>,
                boost::multi_index::const_mem_fun<SavedSearchHolder,const QString,&SavedSearchHolder::nameUpper>
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

} // namespace quentier

#endif // LIB_QUENTIER_LOCAL_STORAGE_LOCAL_STORAGE_CACHE_MANAGER_PRIVATE_H
