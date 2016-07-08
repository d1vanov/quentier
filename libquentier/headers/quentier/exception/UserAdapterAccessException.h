#ifndef LIB_QUENTIER_EXCEPTION_USER_ADAPTER_ACCESS_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_USER_ADAPTER_ACCESS_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT UserAdapterAccessException: public IQuentierException
{
public:
    explicit UserAdapterAccessException(const QNLocalizedString & message);

protected:
    const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_USER_ADAPTER_ACCESS_EXCEPTION_H
