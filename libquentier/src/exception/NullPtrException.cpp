#include <quentier/exception/NullPtrException.h>

namespace quentier {

NullPtrException::NullPtrException(const QString & message) :
    IQuentierException(message)
{}

const QString NullPtrException::exceptionDisplayName() const
{
    return QString("NullPtrException");
}

} // namespace quentier
