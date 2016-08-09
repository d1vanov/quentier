/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H
#define LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H

#include <quentier/utility/Linkage.h>
#include <QDebug>
#include <QString>

namespace quentier {

void QUENTIER_EXPORT QuentierInitializeLogging();

struct LogLevel
{
    enum type {
        TraceLevel,
        DebugLevel,
        InfoLevel,
        WarnLevel,
        ErrorLevel,
        FatalLevel
    };
};

void QUENTIER_EXPORT QuentierAddLogEntry(const QString & message, const LogLevel::type logLevel);

void QUENTIER_EXPORT QuentierSetMinLogLevel(const LogLevel::type logLevel);

void QUENTIER_EXPORT QuentierAddStdOutLogDestination();

bool QUENTIER_EXPORT QuentierIsLogLevelActive(const LogLevel::type logLevel);

} // namespace quentier

#define __QNLOG_BASE(message, level) \
    if (quentier::QuentierIsLogLevelActive(quentier::LogLevel::level##Level)) { \
        QString __quentierLogEntry; \
        QDebug __quentierLogStrm(&__quentierLogEntry); \
        __quentierLogStrm << __FILE__ << '@' << __LINE__ << ": " << message; \
        quentier::QuentierAddLogEntry(__quentierLogEntry, quentier::LogLevel::level##Level); \
    }

#define QNTRACE(message) \
    __QNLOG_BASE(message, Trace)

#define QNDEBUG(message) \
    __QNLOG_BASE(message, Debug)

#define QNINFO(message) \
    __QNLOG_BASE(message, Info)

#define QNWARNING(message) \
    __QNLOG_BASE(message, Warn)

#define QNCRITICAL(message) \
    __QNLOG_BASE(message, Error)

#define QNFATAL(message) \
    __QNLOG_BASE(message, Fatal)

#define QUENTIER_SET_MIN_LOG_LEVEL(level) \
    quentier::QuentierSetMinLogLevel(quentier::LogLevel::level##Level)

#define QUENTIER_INITIALIZE_LOGGING() \
    quentier::QuentierInitializeLogging()

#define QUENTIER_ADD_STDOUT_LOG_DESTINATION() \
    quentier::QuentierAddStdOutLogDestination()

#endif // LIB_QUENTIER_LOGGING_QUENTIER_LOGGER_H
