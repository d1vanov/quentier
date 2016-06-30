#include <quentier/exception/LoggerInitializationException.h>

namespace quentier {

LoggerInitializationException::LoggerInitializationException(const QString & message) :
    IQuentierException(message)
{}

const QString LoggerInitializationException::exceptionDisplayName() const
{
    return "LoggerInitializationException";
}

}
