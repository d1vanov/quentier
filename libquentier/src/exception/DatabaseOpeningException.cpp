#include <quentier/exception/DatabaseOpeningException.h>

namespace quentier {

DatabaseOpeningException::DatabaseOpeningException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString DatabaseOpeningException::exceptionDisplayName() const
{
    return QString("DatabaseOpeningException");
}

}
