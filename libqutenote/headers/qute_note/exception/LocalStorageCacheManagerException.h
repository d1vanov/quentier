#ifndef __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class LocalStorageCacheManagerException : public IQuteNoteException
{
public:
    explicit LocalStorageCacheManagerException(const QString & message);

#ifdef _MSC_VER
    virtual ~LocalStorageCacheManagerException();
#elif __cplusplus >= 201103L
    virtual ~LocalStorageCacheManagerException() noexcept;
#else
    virtual ~LocalStorageCacheManagerException() throw();
#endif

    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
