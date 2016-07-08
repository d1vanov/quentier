#ifndef LIB_QUENTIER_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H
#define LIB_QUENTIER_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class QUENTIER_EXPORT DatabaseSqlErrorException: public IQuentierException
{
public:
    explicit DatabaseSqlErrorException(const QNLocalizedString & message);

protected:
    virtual const QString exceptionDisplayName() const Q_DECL_OVERRIDE;
};

}

#endif // LIB_QUENTIER_EXCEPTION_DATABASE_SQL_ERROR_EXCEPTION_H
