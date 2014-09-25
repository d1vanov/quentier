#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <QtGlobal>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

class ILocalStorageCacheExpiryChecker
{
protected:
    ILocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager);
    const LocalStorageCacheManager &    m_localStorageCacheManager;

public:
    virtual ~ILocalStorageCacheExpiryChecker();

    virtual ILocalStorageCacheExpiryChecker * clone() const = 0;

    virtual bool checkNotes() const = 0;
    virtual bool checkNotebooks() const = 0;

private:
    ILocalStorageCacheExpiryChecker() = delete;
    ILocalStorageCacheExpiryChecker(const ILocalStorageCacheExpiryChecker & other) = delete;
    ILocalStorageCacheExpiryChecker(ILocalStorageCacheExpiryChecker && other) = delete;
    ILocalStorageCacheExpiryChecker & operator=(const ILocalStorageCacheExpiryChecker & other) = delete;
    ILocalStorageCacheExpiryChecker & operator=(ILocalStorageCacheExpiryChecker && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
