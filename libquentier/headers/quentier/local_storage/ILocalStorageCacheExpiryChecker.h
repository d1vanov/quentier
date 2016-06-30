#ifndef LIB_QUENTIER_LOCAL_STORAGE_I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define LIB_QUENTIER_LOCAL_STORAGE_I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <quentier/utility/Printable.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

class QUENTIER_EXPORT ILocalStorageCacheExpiryChecker: public Printable
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

    virtual QTextStream & print(QTextStream & strm) const = 0;

private:
    ILocalStorageCacheExpiryChecker() Q_DECL_DELETE;
    Q_DISABLE_COPY(ILocalStorageCacheExpiryChecker)
};

} // namespace quentier

#endif // LIB_QUENTIER_LOCAL_STORAGE_I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
