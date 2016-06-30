#include <quentier/exception/UserAdapterAccessException.h>

namespace quentier {

UserAdapterAccessException::UserAdapterAccessException(const QString & message) :
    IQuentierException(message)
{}

const QString UserAdapterAccessException::exceptionDisplayName() const
{
    return QString("UserAdapterAccessException");
}

} // namespace quentier
