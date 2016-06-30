#ifndef LIB_QUENTIER_EXCEPTION_RESOURCE_ADAPTER_ACCESS_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_RESOURCE_ADAPTER_ACCESS_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT ResourceAdapterAccessException: public IQuentierException
{
public:
    explicit ResourceAdapterAccessException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_RESOURCE_ADAPTER_ACCESS_EXCEPTION_H
