#ifndef __LIB_QUTE_NOTE__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
#define __LIB_QUTE_NOTE__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

class QUTE_NOTE_EXPORT ILocalStorageCacheExpiryChecker: public Printable
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOCAL_STORAGE__I_LOCAL_STORAGE_CACHE_EXPIRY_CHECKER_H
