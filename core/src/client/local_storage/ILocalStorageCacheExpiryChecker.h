#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <tools/Printable.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

class ILocalStorageCacheExpiryChecker: public Printable
{
protected:
    ILocalStorageCacheExpiryChecker(const LocalStorageCacheManager & cacheManager);
    const LocalStorageCacheManager &    m_localStorageCacheManager;

public:
    virtual ~ILocalStorageCacheExpiryChecker();

    virtual ILocalStorageCacheExpiryChecker * clone() const = 0;

    virtual bool checkNotes() const = 0;
    virtual bool checkNotebooks() const = 0;
    virtual bool checkTags() const = 0;
    virtual bool checkLinkedNotebooks() const = 0;
    virtual bool checkSavedSearches() const = 0;

    virtual QTextStream & Print(QTextStream & strm) const = 0;

private:
    ILocalStorageCacheExpiryChecker() Q_DECL_DELETE;
    ILocalStorageCacheExpiryChecker(const ILocalStorageCacheExpiryChecker & other) Q_DECL_DELETE;
    ILocalStorageCacheExpiryChecker(ILocalStorageCacheExpiryChecker && other) Q_DECL_DELETE;
    ILocalStorageCacheExpiryChecker & operator=(const ILocalStorageCacheExpiryChecker & other) Q_DECL_DELETE;
    ILocalStorageCacheExpiryChecker & operator=(ILocalStorageCacheExpiryChecker && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
