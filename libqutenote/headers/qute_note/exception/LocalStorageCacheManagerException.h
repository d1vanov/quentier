#ifndef __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
#define __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H

#include <qute_note/exception/IQuteNoteException.h>

namespace qute_note {

class QUTE_NOTE_EXPORT LocalStorageCacheManagerException: public IQuteNoteException
{
public:
    explicit LocalStorageCacheManagerException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__EXCEPTION__LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
