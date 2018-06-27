/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H
#define QUENTIER_MODELS_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H

#include "LogViewerModel.h"
#include <QRegExp>

namespace quentier {

class LogViewerModel::LogFileParser
{
public:
    LogFileParser();

    bool parseDataEntriesFromLogFile(const qint64 fromPos, const int maxDataEntries,
                                     const QVector<LogLevel::type> & disabledLogLevels,
                                     const QRegExp & filterContentRegExp,
                                     QFile & logFile, QVector<LogViewerModel::Data> & dataEntries,
                                     qint64 & endPos, ErrorString & errorDescription);

private:
    struct ParseLineStatus
    {
        enum type
        {
            AppendedToLastEntry = 0,
            FilteredEntry,
            CreatedNewEntry,
            Error
        };
    };

    ParseLineStatus::type parseLogFileLine(const QString & line, const ParseLineStatus::type previousParseLineStatus,
                                           const QVector<LogLevel::type> & disabledLogLevels, const QRegExp & filterContentRegExp,
                                           QVector<LogViewerModel::Data> & dataEntries, ErrorString & errorDescription);

    void appendLogEntryLine(LogViewerModel::Data & data, const QString & line) const;

private:
    QRegExp     m_logParsingRegex;
};

} // namespace quentier

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_LOG_FILE_PARSER_H
