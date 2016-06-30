#include <quentier/exception/ApplicationSettingsInitializationException.h>

namespace quentier {

ApplicationSettingsInitializationException::ApplicationSettingsInitializationException(const QString & message) :
    IQuentierException(message)
{}

const QString ApplicationSettingsInitializationException::exceptionDisplayName() const
{
    return QString("ApplicationSettingsInitializationException");
}

} // namespace quentier
