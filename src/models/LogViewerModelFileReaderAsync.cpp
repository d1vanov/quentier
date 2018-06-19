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
#include <QTextStream>

namespace quentier {

LogViewerModel::FileReaderAsync::FileReaderAsync(QString targetFilePath, QObject * parent) :
    QObject(parent),
    m_targetFile(targetFilePath)
{}

LogViewerModel::FileReaderAsync::~FileReaderAsync()
{
    if (m_targetFile.isOpen()) {
        m_targetFile.close();
    }
}

void LogViewerModel::FileReaderAsync::onReadLogFileLines(qint64 fromPos, quint32 maxLines)
{
    if (!m_targetFile.isOpen() && !m_targetFile.open(QIODevice::ReadOnly)) {
        QFileInfo targetFileInfo(m_targetFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = targetFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        Q_EMIT readLogFileLines(fromPos, -1, QStringList(), errorDescription);
        return;
    }

    QTextStream strm(&m_targetFile);

    if (!strm.seek(fromPos)) {
        ErrorString errorDescription(QT_TR_NOOP("Failed to read the data from log file: failed to seek at position"));
        errorDescription.details() = QString::number(fromPos);
        QNWARNING(errorDescription);
        Q_EMIT readLogFileLines(fromPos, -1, QStringList(), errorDescription);
        return;
    }

    QString line;
    QStringList lines;
    for(quint32 lineNum = 0; (lineNum < maxLines) && !strm.atEnd(); ++lineNum)
    {
        line = strm.readLine();
        if (line.isNull()) {
            break;
        }

        lines << line;
    }

    Q_EMIT readLogFileLines(fromPos, strm.pos(), lines, ErrorString());
}

} // namespace quentier
