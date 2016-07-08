#ifndef LIB_QUENTIER_EXCEPTION_LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT LocalStorageCacheManagerException: public IQuentierException
{
public:
    explicit LocalStorageCacheManagerException(const QNLocalizedString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_LOCAL_STORAGE_CACHE_MANAGER_EXCEPTION_H
