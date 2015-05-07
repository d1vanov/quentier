#ifndef __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_H
#define __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_H

#include "Linkage.h"
#include <QObject>
#include <QString>
#include <QUuid>
#include <QByteArray>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorkerPrivate)

/**
 * @brief The FileIOThreadWorker class is a wrapper under simple file IO operations,
 * it is meant to be used for simple asynchronous IO
 */
class QUTE_NOTE_EXPORT FileIOThreadWorker: public QObject
{
    Q_OBJECT
public:
    explicit FileIOThreadWorker(QObject * parent = nullptr);

    /**
     * @brief setIdleTimePeriod - set time period defining the idle state of FileIOThreadWorker:
     * once the time measured since the last IO operation is over the specified number of seconds,
     * the class emits readyForIO signal to any interested listeners of this event. If this method
     * is not called ever, the default idle time period would be 30 seconds.
     * @param seconds - number of seconds for idle time period
     */
    void setIdleTimePeriod(qint32 seconds);

Q_SIGNALS:
    /**
     * @brief readyForIO - signal sent when the queue for file IO is empty for some time
     * (30 seconds by default, can also be configured via setIdleTimePeriod method)
     * after the last IO event to indicate to signal listeners that they can perform some IO
     * via the FileIOThreadWorker thread
     */
    void readyForIO();

    /**
     * @brief writeFileRequestProcessed - signal sent when the file write request with given id is finished
     * @param success - true if write operation was successful, false otherwise
     * @param errorDescription - textual description of the error
     * @param requestId - unique identifier of the file write request
     */
    void writeFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

    /**
     * @brief readFileRequestProcessed - signal send when the file read request with given id is finished
     * @param success - true if read operation was successful, false otherwise
     * @param errorDescription - textual description of the error
     * @param data - data read from file
     * @param requestId - unique identifier of the file read request
     */
    void readFileRequestProcessed(bool success, QString errorDescription, QByteArray data,
                                  QUuid requestId);

public Q_SLOTS:
    /**
     * @brief onWriteFileRequest - slot processing the file write request with given request id
     * @param absoluteFilePath - absolute file path to be written
     * @param data - data to be written to the file
     * @param requestId - unique identifier of the file write request
     */
    void onWriteFileRequest(QString absoluteFilePath,
                            QByteArray data,
                            QUuid requestId);

    /**
     * @brief onReadFileRequest - slot processing the file read request with given request id
     * @param absoluteFilePath - absolute file path to be read
     * @param requestId - unique identifier of the file read request
     */
    void onReadFileRequest(QString absoluteFilePath,
                           QUuid requestId);

private:
    FileIOThreadWorkerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(FileIOThreadWorker)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_H
