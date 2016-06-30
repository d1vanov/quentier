#include <quentier/exception/DatabaseSqlErrorException.h>

namespace quentier {

DatabaseSqlErrorException::DatabaseSqlErrorException(const QString & message) :
    IQuentierException(message)
{}

const QString DatabaseSqlErrorException::exceptionDisplayName() const
{
    return QString("DatabaseSqlErrorException");
}

}
