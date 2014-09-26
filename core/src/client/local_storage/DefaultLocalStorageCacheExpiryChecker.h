#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include "ILocalStorageCacheExpiryChecker.h"

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

private:
    DefaultLocalStorageCacheExpiryChecker() = delete;
    DefaultLocalStorageCacheExpiryChecker(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker(DefaultLocalStorageCacheExpiryChecker && other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(const DefaultLocalStorageCacheExpiryChecker & other) = delete;
    DefaultLocalStorageCacheExpiryChecker & operator=(DefaultLocalStorageCacheExpiryChecker && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__DEFAULT_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
