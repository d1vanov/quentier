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

#include "FileIOThreadWorker_p.h"
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimerEvent>

namespace quentier {

FileIOThreadWorkerPrivate::FileIOThreadWorkerPrivate(QObject * parent) :
    QObject(parent),
    m_idleTimePeriodSeconds(30),
    m_postOperationTimerId(0)
{}

void FileIOThreadWorkerPrivate::setIdleTimePeriod(const qint32 seconds)
{
    QNDEBUG(QStringLiteral("FileIOThreadWorkerPrivate::setIdleTimePeriod: seconds = ") << seconds);
    m_idleTimePeriodSeconds = seconds;
}

#define RESTART_TIMER() \
    if (m_postOperationTimerId != 0) { \
        killTimer(m_postOperationTimerId); \
    } \
    m_postOperationTimerId = startTimer(SEC_TO_MSEC(m_idleTimePeriodSeconds)); \
    QNTRACE(QStringLiteral("FileIOThreadWorkerPrivate: started timer with id ") << m_postOperationTimerId)

void FileIOThreadWorkerPrivate::onWriteFileRequest(QString absoluteFilePath, QByteArray data,
                                                   QUuid requestId, bool append)
{
    QNDEBUG(QStringLiteral("FileIOThreadWorkerPrivate::onWriteFileRequest: file path = ") << absoluteFilePath
            << QStringLiteral(", request id = ") << requestId << QStringLiteral(", append = ")
            << (append ? QStringLiteral("true") : QStringLiteral("false")));

    QFileInfo fileInfo(absoluteFilePath);
    QDir folder = fileInfo.absoluteDir();
    if (!folder.exists())
    {
        bool madeFolder = folder.mkpath(folder.absolutePath());
        if (!madeFolder) {
            ErrorString error(QT_TRANSLATE_NOOP("", "can't create folder to write file into"));
            error.details() = absoluteFilePath;
            QNWARNING(error);
            emit writeFileRequestProcessed(false, error, requestId);
            RESTART_TIMER();
            return;
        }
    }

    QFile file(absoluteFilePath);

    QIODevice::OpenMode mode;
    if (append) {
        mode = QIODevice::Append;
    }
    else {
        mode = QIODevice::WriteOnly;
    }

    bool open = file.open(mode);
    if (Q_UNLIKELY(!open)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open file for writing/appending"));
        error.details() = absoluteFilePath;
        QNWARNING(error);
        emit writeFileRequestProcessed(false, error, requestId);
        RESTART_TIMER();
        return;
    }

    qint64 writtenBytes = file.write(data);
    if (Q_UNLIKELY(writtenBytes < data.size())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't write the whole data to file"));
        error.details() = absoluteFilePath;
        QNWARNING(error);
        emit writeFileRequestProcessed(false, error, requestId);
        RESTART_TIMER();
        return;
    }

    file.close();
    QNDEBUG(QStringLiteral("Successfully wrote the file ") + absoluteFilePath);
    emit writeFileRequestProcessed(true, ErrorString(), requestId);
    RESTART_TIMER();
}

void FileIOThreadWorkerPrivate::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FileIOThreadWorkerPrivate::onReadFileRequest: file path = ") << absoluteFilePath
            << QStringLiteral(", request id = ") << requestId);

    QFile file(absoluteFilePath);
    if (!file.exists()) {
        QNTRACE(QStringLiteral("The file to read does not exist, sending empty data in return"));
        emit readFileRequestProcessed(true, ErrorString(), QByteArray(), requestId);
        RESTART_TIMER();
        return;
    }

    bool open = file.open(QIODevice::ReadOnly);
    if (!open) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open file for reading"));
        error.details() = absoluteFilePath;
        QNDEBUG(error);
        emit readFileRequestProcessed(false, error, QByteArray(), requestId);
        RESTART_TIMER();
        return;
    }

    QByteArray data = file.readAll();
    emit readFileRequestProcessed(true, ErrorString(), data, requestId);
    RESTART_TIMER();
}

void FileIOThreadWorkerPrivate::timerEvent(QTimerEvent * pEvent)
{
    if (!pEvent) {
        QNWARNING(QStringLiteral("Detected null pointer to QTimerEvent in FileIOThreadWorkerPrivate"));
        return;
    }

    qint32 timerId = pEvent->timerId();

    if (timerId != m_postOperationTimerId) {
        QNTRACE(QStringLiteral("Received unidentified timer event for FileIOThreadWorkerPrivate"));
        return;
    }

    killTimer(timerId);
    m_postOperationTimerId = 0;

    emit readyForIO();
}

} // namespace quentier
