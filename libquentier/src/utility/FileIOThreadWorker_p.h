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

#ifndef LIB_QUENTIER_UTILITY_FILE_IO_THREAD_WORKER_P_H
#define LIB_QUENTIER_UTILITY_FILE_IO_THREAD_WORKER_P_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QIODevice>

namespace quentier {

class FileIOThreadWorkerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit FileIOThreadWorkerPrivate(QObject * parent = Q_NULLPTR);

    void setIdleTimePeriod(const qint32 seconds);

Q_SIGNALS:
    void readyForIO();
    void writeFileRequestProcessed(bool success, QNLocalizedString errorDescription, QUuid requestId);
    void readFileRequestProcessed(bool success, QNLocalizedString errorDescription, QByteArray data, QUuid requestId);

public Q_SLOTS:
    void onWriteFileRequest(QString absoluteFilePath, QByteArray data,
                            QUuid requestId, bool append);

    void onReadFileRequest(QString absoluteFilePath, QUuid requestId);

private:
    virtual void timerEvent(QTimerEvent * pEvent);

private:
    qint32  m_idleTimePeriodSeconds;
    qint32  m_postOperationTimerId;
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_FILE_IO_THREAD_WORKER_P_H
