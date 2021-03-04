/*
 * Copyright 2017-2021 Dmitry Ivanov
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
#include <QTimeZone>

namespace quentier {

LogViewerModel::FileReaderAsync::FileReaderAsync(
    const QString & targetFilePath, const QVector<LogLevel> & disabledLogLevels,
    const QString & logEntryContentFilter, QObject * parent) :
    QObject(parent),
    m_targetFile(targetFilePath), m_disabledLogLevels(disabledLogLevels),
    m_filterRegExp(logEntryContentFilter, Qt::CaseSensitive, QRegExp::Wildcard)
{}

LogViewerModel::FileReaderAsync::~FileReaderAsync()
{
    if (m_targetFile.isOpen()) {
        m_targetFile.close();
    }
}

void LogViewerModel::FileReaderAsync::onReadDataEntriesFromLogFile(
    qint64 fromPos, int maxDataEntries)
{
    QVector<LogViewerModel::Data> dataEntries;
    qint64 endPos = -1;
    ErrorString errorDescription;
    const bool res = m_parser.parseDataEntriesFromLogFile(
        fromPos, maxDataEntries, m_disabledLogLevels, m_filterRegExp,
        m_targetFile, dataEntries, endPos, errorDescription);
    if (res) {
        Q_EMIT readLogFileDataEntries(
            fromPos, endPos, dataEntries, ErrorString());
    }
    else {
        Q_EMIT readLogFileDataEntries(
            fromPos, -1, QVector<LogViewerModel::Data>(), errorDescription);
    }
}

} // namespace quentier
