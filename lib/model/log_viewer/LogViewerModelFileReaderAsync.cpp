/*
 * Copyright 2017-2024 Dmitry Ivanov
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
    const QString & targetFilePath, const QList<LogLevel> & disabledLogLevels,
    const QString & logEntryContentFilter, QObject * parent) :
    QObject{parent},
    m_targetFile{targetFilePath}, m_disabledLogLevels{disabledLogLevels},
    m_filterRegExp{
        QRegularExpression::wildcardToRegularExpression(logEntryContentFilter)}
{}

LogViewerModel::FileReaderAsync::~FileReaderAsync()
{
    if (m_targetFile.isOpen()) {
        m_targetFile.close();
    }
}

void LogViewerModel::FileReaderAsync::onReadDataEntriesFromLogFile(
    const qint64 fromPos, const int maxDataEntries)
{
    QList<LogViewerModel::Data> dataEntries;
    qint64 endPos = -1;
    ErrorString errorDescription;
    bool res = m_parser.parseDataEntriesFromLogFile(
        fromPos, maxDataEntries, m_disabledLogLevels, m_filterRegExp,
        m_targetFile, dataEntries, endPos, errorDescription);
    if (res) {
        Q_EMIT readLogFileDataEntries(
            fromPos, endPos, dataEntries, ErrorString());
    }
    else {
        Q_EMIT readLogFileDataEntries(
            fromPos, -1, QList<LogViewerModel::Data>{}, errorDescription);
    }
}

} // namespace quentier
