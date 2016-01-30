// Copyright (c) 2013, Razvan Petru
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLog.h"
#include "QsLogDest.h"
#ifdef QS_LOG_SEPARATE_THREAD
#include <QThreadPool>
#include <QRunnable>
#endif
#include <QMutex>
#include <QVector>
#include <QDateTime>
#include <QLatin1String>
#include <QtGlobal>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace QsLogging
{
typedef QVector<DestinationPtr> DestinationList;

#ifdef QS_LOG_SEPARATE_THREAD
class LogWriterRunnable : public QRunnable
{
public:
    LogWriterRunnable(const LogMessage& message);
    virtual void run();

private:
    LogMessage mMessage;
};
#endif

class LoggerImpl
{
public:
    LoggerImpl();

#ifdef QS_LOG_SEPARATE_THREAD
    QThreadPool threadPool;
#endif
    QMutex logMutex;
    Level level;
    DestinationList destList;
};

#ifdef QS_LOG_SEPARATE_THREAD
LogWriterRunnable::LogWriterRunnable(const LogMessage& message)
    : QRunnable()
    , mMessage(message)
{
}

void LogWriterRunnable::run()
{
    Logger::instance().write(mMessage);
}
#endif


LoggerImpl::LoggerImpl()
    : level(InfoLevel)
{
    // assume at least file + console
    destList.reserve(2);
#ifdef QS_LOG_SEPARATE_THREAD
    threadPool.setMaxThreadCount(1);
    threadPool.setExpiryTimeout(-1);
#endif
}


Logger::Logger()
    : d(new LoggerImpl)
{
    qRegisterMetaType<LogMessage>("QsLogging::LogMessage");
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

// tries to extract the level from a string log message. If available, conversionSucceeded will
// contain the conversion result.
Level Logger::levelFromLogMessage(const QString& logMessage, bool* conversionSucceeded)
{
    using namespace QsLogging;

    if (conversionSucceeded)
        *conversionSucceeded = true;

    if (logMessage.startsWith(QLatin1String(LevelName(TraceLevel))))
        return TraceLevel;
    if (logMessage.startsWith(QLatin1String(LevelName(DebugLevel))))
        return DebugLevel;
    if (logMessage.startsWith(QLatin1String(LevelName(InfoLevel))))
        return InfoLevel;
    if (logMessage.startsWith(QLatin1String(LevelName(WarnLevel))))
        return WarnLevel;
    if (logMessage.startsWith(QLatin1String(LevelName(ErrorLevel))))
        return ErrorLevel;
    if (logMessage.startsWith(QLatin1String(LevelName(FatalLevel))))
        return FatalLevel;

    if (conversionSucceeded)
        *conversionSucceeded = false;
    return OffLevel;
}

Logger::~Logger()
{
#ifdef QS_LOG_SEPARATE_THREAD
    d->threadPool.waitForDone();
#endif
    delete d;
    d = 0;
}

void Logger::addDestination(DestinationPtr destination)
{
    assert(destination.data());
    QMutexLocker lock(&d->logMutex);
    d->destList.push_back(destination);
}

void Logger::removeDestination(const DestinationPtr& destination)
{
    QMutexLocker lock(&d->logMutex);
    const int destinationIndex = d->destList.indexOf(destination);
    if (destinationIndex != -1) {
        d->destList.remove(destinationIndex);
    }
}

bool Logger::hasDestinationOfType(const char* type) const
{
    QMutexLocker lock(&d->logMutex);
    for (DestinationList::iterator it = d->destList.begin(),
        endIt = d->destList.end();it != endIt;++it) {
        if ((*it)->type() == QLatin1String(type)) {
            return true;
        }
    }

    return false;
}

void Logger::setLoggingLevel(Level newLevel)
{
    d->level = newLevel;
}

Level Logger::loggingLevel() const
{
    return d->level;
}

//! creates the complete log message and passes it to the logger
void Logger::Helper::writeToLog()
{
    const LogMessage msg(buffer, QDateTime::currentDateTimeUtc(), level);
    Logger::instance().enqueueWrite(msg);
}

Logger::Helper::~Helper()
{
    try {
        writeToLog();
    }
    catch(std::exception&) {
        // you shouldn't throw exceptions from a sink
        assert(!"exception in logger helper destructor");
        throw;
    }
}

//! directs the message to the task queue or writes it directly
void Logger::enqueueWrite(const LogMessage& message)
{
#ifdef QS_LOG_SEPARATE_THREAD
    LogWriterRunnable *r = new LogWriterRunnable(message);
    d->threadPool.start(r);
#else
    write(message);
#endif
}

//! Sends the message to all the destinations. The level for this message is passed in case
//! it's useful for processing in the destination.
void Logger::write(const LogMessage& message)
{
    QMutexLocker lock(&d->logMutex);
    for (DestinationList::iterator it = d->destList.begin(),
        endIt = d->destList.end();it != endIt;++it) {
        (*it)->write(message);
    }
}

} // end namespace
