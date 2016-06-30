#include <quentier/exception/DatabaseOpeningException.h>

namespace quentier {

DatabaseOpeningException::DatabaseOpeningException(const QString & message) :
    IQuentierException(message)
{}

const QString DatabaseOpeningException::exceptionDisplayName() const
{
    return QString("DatabaseOpeningException");
}

}
