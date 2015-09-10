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

    virtual DefaultLocalStorageCacheExpiryChecker * clone() const Q_DECL_OVERRIDE;

    virtual bool checkNotes() const Q_DECL_OVERRIDE;
    virtual bool checkNotebooks() const Q_DECL_OVERRIDE;
    virtual bool checkTags() const Q_DECL_OVERRIDE;
    virtual bool checkLinkedNotebooks() const Q_DECL_OVERRIDE;
    virtual bool checkSavedSearches() const Q_DECL_OVERRIDE;

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    DefaultLocalStorageCacheExpiryChecker() Q_DECL_DELETE;
    Q_DISABLE_COPY(DefaultLocalStorageCacheExpiryChecker)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
