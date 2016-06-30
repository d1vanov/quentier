#include <quentier/exception/ResourceAdapterAccessException.h>

namespace quentier {

ResourceAdapterAccessException::ResourceAdapterAccessException(const QString & message) :
    IQuentierException(message)
{}

const QString ResourceAdapterAccessException::exceptionDisplayName() const
{
    return QString("ResourceAdapterAccessException");
}

} // namespace quentier
