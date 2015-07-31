#include "FileIOThreadWorker_p.h"
#include <qute_note/utility/Utility.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QFile>
#include <QTimerEvent>

namespace qute_note {

FileIOThreadWorkerPrivate::FileIOThreadWorkerPrivate(QObject * parent) :
    QObject(parent),
    m_idleTimePeriodSeconds(30),
    m_postOperationTimerId(0)
{}

void FileIOThreadWorkerPrivate::setIdleTimePeriod(const qint32 seconds)
{
    QNDEBUG("FileIOThreadWorkerPrivate::setIdleTimePeriod: seconds = " << seconds);
    m_idleTimePeriodSeconds = seconds;
}

#define RESTART_TIMER() \
    killTimer(m_postOperationTimerId); \
    m_postOperationTimerId = startTimer(SEC_TO_MSEC(m_idleTimePeriodSeconds))

void FileIOThreadWorkerPrivate::onWriteFileRequest(QString absoluteFilePath, QByteArray data, QUuid requestId)
{
    QNDEBUG("FileIOThreadWorkerPrivate::onWriteFileRequest: file path = " << absoluteFilePath
            << ", request id = " << requestId);

    QFile file(absoluteFilePath);
    bool open = file.open(QIODevice::WriteOnly);
    if (!open) {
        emit writeFileRequestProcessed(false, QT_TR_NOOP("Can't open file for writing: ") + absoluteFilePath,
                                       requestId);
        RESTART_TIMER();
        return;
    }

    qint64 writtenBytes = file.write(data);
    if (writtenBytes < data.size()) {
        emit writeFileRequestProcessed(false, QT_TR_NOOP("Can't write the whole file: ") + absoluteFilePath,
                                       requestId);
        RESTART_TIMER();
        return;
    }

    file.close();
    emit writeFileRequestProcessed(true, QString(), requestId);
    RESTART_TIMER();
}

void FileIOThreadWorkerPrivate::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    QNDEBUG("FileIOThreadWorkerPrivate::onReadFileRequest: file path = " << absoluteFilePath
            << ", request id = " << requestId);

    QFile file(absoluteFilePath);
    bool open = file.open(QIODevice::ReadOnly);
    if (!open) {
        emit readFileRequestProcessed(false, QT_TR_NOOP("Can't open file for reading: ") + absoluteFilePath,
                                      QByteArray(), requestId);
        RESTART_TIMER();
        return;
    }

    QByteArray data = file.readAll();
    emit readFileRequestProcessed(true, QString(), data, requestId);
    RESTART_TIMER();
}

void FileIOThreadWorkerPrivate::timerEvent(QTimerEvent * pEvent)
{
    if (!pEvent) {
        QNWARNING("Detected null pointer to QTimerEvent in FileIOThreadWorkerPrivate");
        return;
    }

    qint32 timerId = pEvent->timerId();
    killTimer(timerId);

    if (timerId != m_postOperationTimerId) {
        QNWARNING("Received unidentified timer event for FileIOThreadWorkerPrivate");
        return;
    }

    emit readyForIO();
}

} // namespace qute_note
