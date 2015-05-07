#include "FileIOThreadWorker.h"
#include "FileIOThreadWorker_p.h"

namespace qute_note {

FileIOThreadWorker::FileIOThreadWorker(QObject * parent) :
    QObject(parent),
    d_ptr(new FileIOThreadWorkerPrivate(this))
{
    QObject::connect(d_ptr, SIGNAL(readyForIO()), this, SIGNAL(readyForIO()));
    QObject::connect(d_ptr, SIGNAL(writeFileRequestProcessed(bool,QString,QUuid)),
                     this, SIGNAL(writeFileRequestProcessed(bool,QString,QUuid)));
    QObject::connect(d_ptr, SIGNAL(readFileRequestProcessed(bool,QString,QByteArray,QUuid)),
                     this, SIGNAL(readFileRequestProcessed(bool,QString,QByteArray,QUuid)));
}

void FileIOThreadWorker::setIdleTimePeriod(qint32 seconds)
{
    Q_D(FileIOThreadWorker);
    d->setIdleTimePeriod(seconds);
}

void FileIOThreadWorker::onWriteFileRequest(QString absoluteFilePath, QByteArray data, QUuid requestId)
{
    Q_D(FileIOThreadWorker);
    d->onWriteFileRequest(absoluteFilePath, data, requestId);
}

void FileIOThreadWorker::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    Q_D(FileIOThreadWorker);
    d->onReadFileRequest(absoluteFilePath, requestId);
}

} // namespace qute_note
