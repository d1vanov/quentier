#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include "ILocalStorageCacheExpiryChecker.h"

#define MAX_NOTES_TO_STORE 100
#define MAX_NOTEBOOKS_TO_STORE 20
#define MAX_TAGS_TO_STORE 100
#define MAX_LINKED_NOTEBOOKS_TO_STORE 8
#define MAX_SAVED_SEARCHES_TO_STORE 20

namespace qute_note {

class DefaultLocalStorageCacheExpiryChecker : public ILocalStorageCacheExpiryChecker
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
    virtual QTextStream & Print(QTextStream & strm) const override;

    DefaultLocalStorageCacheExpiryChecker() = delete;
    DefaultLocalStorageCacheExpiryChecker(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker(DefaultLocalStorageCacheExpiryChecker && other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(DefaultLocalStorageCacheExpiryChecker && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
