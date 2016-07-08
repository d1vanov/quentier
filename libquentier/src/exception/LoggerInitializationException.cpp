#include <quentier/exception/LoggerInitializationException.h>

namespace quentier {

LoggerInitializationException::LoggerInitializationException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString LoggerInitializationException::exceptionDisplayName() const
{
    return "LoggerInitializationException";
}

}
