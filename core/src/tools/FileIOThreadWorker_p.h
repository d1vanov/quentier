#ifndef __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_PRIVATE_H
#define __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_PRIVATE_H

#include <QObject>
#include <QString>
#include <QUuid>

namespace qute_note {

class FileIOThreadWorkerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit FileIOThreadWorkerPrivate(QObject * parent = nullptr);

    void setIdleTimePeriod(const qint32 seconds);

Q_SIGNALS:
    void readyForIO();
    void writeFileRequestProcessed(bool success, QString errorDescription,
                                   QUuid requestId);
    void readFileRequestProcessed(bool success, QString errorDescription,
                                  QByteArray data, QUuid requestId);

public Q_SLOTS:
    void onWriteFileRequest(QString absoluteFilePath,
                            QByteArray data,
                            QUuid requestId);

    void onReadFileRequest(QString absoluteFilePath,
                           QUuid requestId);

private:
    virtual void timerEvent(QTimerEvent * pEvent);

private:
    qint32  m_idleTimePeriodSeconds;
    qint32  m_postOperationTimerId;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__FILE_IO_THREAD_WORKER_PRIVATE_H
