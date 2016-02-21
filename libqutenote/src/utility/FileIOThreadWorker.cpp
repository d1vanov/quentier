#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/Qt4Helper.h>
#include "FileIOThreadWorker_p.h"

namespace qute_note {

FileIOThreadWorker::FileIOThreadWorker(QObject * parent) :
    QObject(parent),
    d_ptr(new FileIOThreadWorkerPrivate(this))
{
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,readyForIO),
                     this, QNSIGNAL(FileIOThreadWorker,readyForIO));
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                     this, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid));
}

void FileIOThreadWorker::setIdleTimePeriod(qint32 seconds)
{
    Q_D(FileIOThreadWorker);
    d->setIdleTimePeriod(seconds);
}

void FileIOThreadWorker::onWriteFileRequest(QString absoluteFilePath, QByteArray data, QUuid requestId, QIODevice::OpenMode mode)
{
    Q_D(FileIOThreadWorker);
    d->onWriteFileRequest(absoluteFilePath, data, requestId, mode);
}

void FileIOThreadWorker::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    Q_D(FileIOThreadWorker);
    d->onReadFileRequest(absoluteFilePath, requestId);
}

} // namespace qute_note
