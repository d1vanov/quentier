/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include "AsyncFileWriter.h"

#include <quentier/logging/QuentierLogger.h>

#include <QFile>

namespace quentier {

AsyncFileWriter::AsyncFileWriter(
        const QString & filePath,
        const QByteArray & dataToWrite,
        QObject * parent) :
    QObject(parent),
    QRunnable(),
    m_filePath(filePath),
    m_dataToWrite(dataToWrite)
{}

void AsyncFileWriter::run()
{
    QNDEBUG("utility", "AsyncFileWriter::run: file path = " << m_filePath);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        ErrorString error(QT_TR_NOOP("can't open file for writing"));
        error.details() = file.errorString();
        QNWARNING("utility", error);
        Q_EMIT fileWriteFailed(error);
    }

    qint64 dataSize = static_cast<qint64>(m_dataToWrite.size());
    qint64 bytesWritten = file.write(m_dataToWrite);
    if (bytesWritten != dataSize) {
        QNDEBUG("utility", "Couldn't write the entire file: expected "
            << dataSize << ", got only " << bytesWritten);
        Q_EMIT fileWriteIncomplete(bytesWritten, dataSize);
    }
    else {
        QNDEBUG("utility", "Successfully written the file");
        Q_EMIT fileSuccessfullyWritten(m_filePath);
    }
}

} // namespace quentier
