#ifndef LIB_QUENTIER_EXCEPTION_NULL_PTR_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_NULL_PTR_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT NullPtrException: public IQuentierException
{
public:
    explicit NullPtrException(const QNLocalizedString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

}

#endif // LIB_QUENTIER_EXCEPTION_NULL_PTR_EXCEPTION_H
