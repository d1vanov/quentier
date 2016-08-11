#include <quentier/logging/QuentierLogger.h>
#include "QuentierLogger_p.h"

namespace quentier {

void QuentierInitializeLogging()
{
    QuentierLogger & logger = QuentierLogger::instance();
    Q_UNUSED(logger)
}

void QuentierAddLogEntry(const QString & message, const LogLevel::type logLevel)
{
    QuentierLogger & logger = QuentierLogger::instance();
    logger.write(message);
    Q_UNUSED(logLevel)
}

void QuentierSetMinLogLevel(const LogLevel::type logLevel)
{
    QuentierLogger & logger = QuentierLogger::instance();
    logger.setMinLogLevel(logLevel);
}

bool QuentierIsLogLevelActive(const LogLevel::type logLevel)
{
    return (QuentierLogger::instance().minLogLevel() <= logLevel);
}

void QuentierAddStdOutLogDestination()
{
    QuentierLogger & logger = QuentierLogger::instance();
    logger.addLogWriter(new QuentierConsoleLogWriter);
}

} // namespace quentier
