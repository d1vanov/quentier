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

#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/Macros.h>
#include "FileIOThreadWorker_p.h"

namespace quentier {

FileIOThreadWorker::FileIOThreadWorker(QObject * parent) :
    QObject(parent),
    d_ptr(new FileIOThreadWorkerPrivate(this))
{
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,readyForIO),
                     this, QNSIGNAL(FileIOThreadWorker,readyForIO));
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,writeFileRequestProcessed,bool,QNLocalizedString,QUuid),
                     this, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QNLocalizedString,QUuid));
    QObject::connect(d_ptr, QNSIGNAL(FileIOThreadWorkerPrivate,readFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid),
                     this, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QNLocalizedString,QByteArray,QUuid));
}

void FileIOThreadWorker::setIdleTimePeriod(qint32 seconds)
{
    Q_D(FileIOThreadWorker);
    d->setIdleTimePeriod(seconds);
}

void FileIOThreadWorker::onWriteFileRequest(QString absoluteFilePath, QByteArray data, QUuid requestId, bool append)
{
    Q_D(FileIOThreadWorker);
    d->onWriteFileRequest(absoluteFilePath, data, requestId, append);
}

void FileIOThreadWorker::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    Q_D(FileIOThreadWorker);
    d->onReadFileRequest(absoluteFilePath, requestId);
}

} // namespace quentier
