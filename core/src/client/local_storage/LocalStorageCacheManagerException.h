#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H

#include <tools/IQuteNoteException.h>

namespace qute_note {

class LocalStorageCacheManagerException : public IQuteNoteException
{
public:
    explicit LocalStorageCacheManagerException(const QString & message);
    virtual ~LocalStorageCacheManagerException();

    virtual const QString exceptionDisplayName() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
