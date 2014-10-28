#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include "ILocalStorageCacheExpiryChecker.h"

#define MAX_NOTES_TO_STORE 100
#define MAX_NOTEBOOKS_TO_STORE 20

// The following condition workarounds strange crash of clang build on Linux
// which is most likely me doing something wrong but can also be a bug
// in either Qt or clang (or both). Anyway, not having the resources to deal with it
// right now, I apply the following workaround
#if defined(__clang__) & defined(__linux__)
#define MAX_TAGS_TO_STORE 40
#define MAX_LINKED_NOTEBOOKS_TO_STORE 5
#define MAX_SAVED_SEARCHES_TO_STORE 5
#else
#define MAX_TAGS_TO_STORE 200
#define MAX_LINKED_NOTEBOOKS_TO_STORE 20
#define MAX_SAVED_SEARCHES_TO_STORE 20
#endif

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
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    DefaultLocalStorageCacheExpiryChecker() = delete;
    DefaultLocalStorageCacheExpiryChecker(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker(DefaultLocalStorageCacheExpiryChecker && other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(DefaultLocalStorageCacheExpiryChecker && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
