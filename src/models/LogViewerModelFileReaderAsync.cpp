/*
 * Copyright 2017 Dmitry Ivanov
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

#include "LogViewerModelFileReaderAsync.h"
#include <QFileInfo>

namespace quentier {

LogViewerModel::FileReaderAsync::FileReaderAsync(QString targetFilePath,
                                                 qint64 startPos, QObject * parent) :
    QObject(parent),
    m_targetFile(targetFilePath),
    m_startPos(startPos)
{}

void LogViewerModel::FileReaderAsync::onStartReading()
{
    if (!m_targetFile.open(QIODevice::ReadOnly)) {
        QFileInfo targetFileInfo(m_targetFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = targetFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        emit finished(QByteArray(), -1, errorDescription);
        return;
    }

    if (!m_targetFile.seek(m_startPos)) {
        ErrorString errorDescription(QT_TR_NOOP("Failed to read the data from log file: failed to seek at position"));
        errorDescription.details() = QString::number(m_startPos);
        QNWARNING(errorDescription);
        emit finished(QByteArray(), -1, errorDescription);
        return;
    }

    const qint64 bufSize = 1024;
    char buf[bufSize];
    QByteArray readData;

    qint64 currentPos = m_startPos;
    while(true)
    {
        qint64 bytesRead = m_targetFile.read(buf, bufSize);
        if (bytesRead == -1)
        {
            ErrorString errorDescription(QT_TR_NOOP("Failed to read the data from log file"));
            QString logFileError = m_targetFile.errorString();
            if (!logFileError.isEmpty()) {
                errorDescription.details() = logFileError;
            }

            QNWARNING(errorDescription);
            emit finished(QByteArray(), -1, errorDescription);
            return;
        }
        else if (bytesRead == 0)
        {
            QNDEBUG(QStringLiteral("Read all data from the log file"));
            break;
        }

        readData.append(buf, static_cast<int>(bytesRead));
        currentPos = m_targetFile.pos();
    }

    emit finished(readData, currentPos, ErrorString());
}

} // namespace quentier
