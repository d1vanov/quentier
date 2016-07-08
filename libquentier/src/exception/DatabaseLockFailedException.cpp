#include <quentier/exception/DatabaseLockFailedException.h>

namespace quentier {

DatabaseLockFailedException::DatabaseLockFailedException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString DatabaseLockFailedException::exceptionDisplayName() const
{
    return QString("DatabaseLockFailedException");
}

} // namespace quentier
