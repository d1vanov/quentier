#ifndef LIB_QUENTIER_EXCEPTION_DATABASE_LOCK_FAILED_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_DATABASE_LOCK_FAILED_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT DatabaseLockFailedException: public IQuentierException
{
public:
    explicit DatabaseLockFailedException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_DATABASE_LOCK_FAILED_EXCEPTION_H
