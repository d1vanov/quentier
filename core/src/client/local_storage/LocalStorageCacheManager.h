#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H

#include <tools/Printable.h>
#include <QScopedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

QT_FORWARD_DECLARE_CLASS(ILocalStorageCacheExpiryChecker)

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManagerPrivate)
class QUTE_NOTE_EXPORT LocalStorageCacheManager: public Printable
{
public:
    LocalStorageCacheManager();
    virtual ~LocalStorageCacheManager();

    enum WhichGuid
    {
        LocalGuid,
        Guid
    };

    void clear();
    bool empty() const;

    // Notes cache
    size_t numCachedNotes() const;
    void cacheNote(const Note & note);
    void expungeNote(const Note & note);
    const Note * findNote(const QString & guid, const WhichGuid wg) const;

    // Notebooks cache
    size_t numCachedNotebooks() const;
    void cacheNotebook(const Notebook & notebook);
    void expungeNotebook(const Notebook & notebook);
    const Notebook * findNotebook(const QString & guid, const WhichGuid wg) const;
    const Notebook * findNotebookByName(const QString & name) const;

    // Tags cache
    size_t numCachedTags() const;
    void cacheTag(const Tag & tag);
    void expungeTag(const Tag & tag);
    const Tag * findTag(const QString & guid, const WhichGuid wg) const;
    const Tag * findTagByName(const QString & name) const;

    // Linked notebooks cache
    size_t numCachedLinkedNotebooks() const;
    void cacheLinkedNotebook(const LinkedNotebook & linkedNotebook);
    void expungeLinkedNotebook(const LinkedNotebook & linkedNotebook);
    const LinkedNotebook * findLinkedNotebook(const QString & guid) const;

    // Saved searches cache
    size_t numCachedSavedSearches() const;
    void cacheSavedSearch(const SavedSearch & savedSearch);
    void expungeSavedSearch(const SavedSearch & savedSearch);
    const SavedSearch * findSavedSearch(const QString & guid, const WhichGuid wg) const;

    void installCacheExpiryFunction(const ILocalStorageCacheExpiryChecker & checker);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    LocalStorageCacheManager(const LocalStorageCacheManager & other) Q_DECL_DELETE;
    LocalStorageCacheManager(LocalStorageCacheManager && other) Q_DECL_DELETE;
    LocalStorageCacheManager & operator=(const LocalStorageCacheManager & other) Q_DECL_DELETE;
    LocalStorageCacheManager & operator=(LocalStorageCacheManager && other) Q_DECL_DELETE;

    LocalStorageCacheManagerPrivate *   d_ptr;
    Q_DECLARE_PRIVATE(LocalStorageCacheManager)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_H
