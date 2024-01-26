/*
 * Copyright 2018-2024 Dmitry Ivanov
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

#pragma once

#include "LogViewerModel.h"

#include <QRegularExpression>

namespace quentier {

class LogViewerModel::LogFileParser
{
public:
    LogFileParser();

    [[nodiscard]] bool parseDataEntriesFromLogFile(
        qint64 fromPos, int maxDataEntries,
        const QList<LogLevel> & disabledLogLevels,
        const QRegularExpression & filterContentRegExp, QFile & logFile,
        QList<LogViewerModel::Data> & dataEntries, qint64 & endPos,
        ErrorString & errorDescription);

private:
    enum class ParseLineStatus
    {
        AppendedToLastEntry = 0,
        FilteredEntry,
        CreatedNewEntry,
        Error
    };

    [[nodiscard]] ParseLineStatus parseLogFileLine(
        const QString & line, const ParseLineStatus previousParseLineStatus,
        const QList<LogLevel> & disabledLogLevels,
        const QRegularExpression & filterContentRegExp,
        QList<LogViewerModel::Data> & dataEntries,
        ErrorString & errorDescription);

    void appendLogEntryLine(
        LogViewerModel::Data & data, const QString & line) const;

    void setInternalLogEnabled(const bool enabled);

private:
    QRegularExpression m_logParsingRegex;

    QFile m_internalLogFile;
    bool m_internalLogEnabled;
};

} // namespace quentier
