#ifndef __LIB_QUTE_NOTE__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __LIB_QUTE_NOTE__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <qute_note/local_storage/ILocalStorageCacheExpiryChecker.h>

#define MAX_NOTES_TO_STORE 100
#define MAX_NOTEBOOKS_TO_STORE 20
#define MAX_TAGS_TO_STORE 200
#define MAX_LINKED_NOTEBOOKS_TO_STORE 20
#define MAX_SAVED_SEARCHES_TO_STORE 20

namespace qute_note {

class QUTE_NOTE_EXPORT DefaultLocalStorageCacheExpiryChecker: public ILocalStorageCacheExpiryChecker
{
public:
    DefaultLocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager);
    virtual ~DefaultLocalStorageCacheExpiryChecker();

    virtual DefaultLocalStorageCacheExpiryChecker * clone() const;

    virtual bool checkNotes() const;
    virtual bool checkNotebooks() const;
    virtual bool checkTags() const;
    virtual bool checkLinkedNotebooks() const;
    virtual bool checkSavedSearches() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    DefaultLocalStorageCacheExpiryChecker() Q_DECL_DELETE;
    DefaultLocalStorageCacheExpiryChecker(const DefaultLocalStorageCacheExpiryChecker & other) Q_DECL_DELETE;
    DefaultLocalStorageCacheExpiryChecker(DefaultLocalStorageCacheExpiryChecker && other) Q_DECL_DELETE;
    DefaultLocalStorageCacheExpiryChecker & operator=(const DefaultLocalStorageCacheExpiryChecker & other) Q_DECL_DELETE;
    DefaultLocalStorageCacheExpiryChecker & operator=(DefaultLocalStorageCacheExpiryChecker && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
