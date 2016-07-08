#ifndef LIB_QUENTIER_EXCEPTION_DATABASE_LOCKED_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_DATABASE_LOCKED_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT DatabaseLockedException: public IQuentierException
{
public:
    explicit DatabaseLockedException(const QNLocalizedString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // LIB_QUENTIER_EXCEPTION_DATABASE_LOCKED_EXCEPTION_H
