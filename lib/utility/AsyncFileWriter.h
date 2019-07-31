/*
 * Copyright 2017-2019 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_LIB_UTILITY_ASYNC_FILE_WRITER_H
#define QUENTIER_LIB_UTILITY_ASYNC_FILE_WRITER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QRunnable>
#include <QString>

namespace quentier {

class AsyncFileWriter: public QObject,
                       public QRunnable
{
    Q_OBJECT
public:
    explicit AsyncFileWriter(
        const QString & filePath,
        const QByteArray & dataToWrite,
        QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void fileSuccessfullyWritten(QString filePath);
    void fileWriteFailed(ErrorString error);

    void fileWriteIncomplete(
        const qint64 bytesWritten, const qint64 bytesTotal);

private:
    virtual void run() Q_DECL_OVERRIDE;

private:
    QString     m_filePath;
    QByteArray  m_dataToWrite;
};

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_ASYNC_FILE_WRITER_H
