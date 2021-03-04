/*
 * Copyright 2018-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H
#define QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H

#include "LogViewerModel.h"

#include <QRegExp>

namespace quentier {

class LogViewerModel::LogFileParser
{
public:
    LogFileParser();

    [[nodiscard]] bool parseDataEntriesFromLogFile(
        const qint64 fromPos, const int maxDataEntries,
        const QVector<LogLevel> & disabledLogLevels,
        const QRegExp & filterContentRegExp, QFile & logFile,
        QVector<LogViewerModel::Data> & dataEntries, qint64 & endPos,
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
        const QVector<LogLevel> & disabledLogLevels,
        const QRegExp & filterContentRegExp,
        QVector<LogViewerModel::Data> & dataEntries,
        ErrorString & errorDescription);

    void appendLogEntryLine(
        LogViewerModel::Data & data, const QString & line) const;

    void setInternalLogEnabled(const bool enabled);

private:
    QRegExp m_logParsingRegex;
    QFile m_internalLogFile;
    bool m_internalLogEnabled = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H
