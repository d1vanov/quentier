#include <quentier/exception/DatabaseSqlErrorException.h>

namespace quentier {

DatabaseSqlErrorException::DatabaseSqlErrorException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString DatabaseSqlErrorException::exceptionDisplayName() const
{
    return QString("DatabaseSqlErrorException");
}

}
