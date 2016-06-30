#ifndef LIB_QUENTIER_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT EmptyDataElementException: public IQuentierException
{
public:
    explicit EmptyDataElementException(const QString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

}

#endif // LIB_QUENTIER_EXCEPTION_EMPTY_DATA_ELEMENT_EXCEPTION_H
