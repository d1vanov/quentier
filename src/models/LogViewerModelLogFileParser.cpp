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

#include "LogViewerModelLogFileParser.h"
#include <QTextStream>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

namespace quentier {

LogViewerModel::LogFileParser::LogFileParser() :
    m_logParsingRegex(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{3})\\s+(\\w+)\\s+(.+)\\s+@\\s+(\\d+)\\s+\\[(\\w+)\\]:\\s(.+$)"),
                      Qt::CaseInsensitive, QRegExp::RegExp)
{}

bool LogViewerModel::LogFileParser::parseDataEntriesFromLogFile(const qint64 fromPos, const int maxDataEntries,
                                                                const QVector<LogLevel::type> & disabledLogLevels,
                                                                const QRegExp & filterContentRegExp, QFile & logFile,
                                                                QVector<LogViewerModel::Data> & dataEntries, qint64 & endPos,
                                                                ErrorString & errorDescription)
{
    if (!logFile.isOpen() && !logFile.open(QIODevice::ReadOnly)) {
        QFileInfo targetFileInfo(logFile);
        errorDescription.setBase(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = targetFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        return false;
    }

    QTextStream strm(&logFile);

    if (!strm.seek(fromPos)) {
        errorDescription.setBase(QT_TR_NOOP("Failed to read the data from log file: failed to seek at position"));
        errorDescription.details() = QString::number(fromPos);
        QNWARNING(errorDescription);
        return false;
    }

    QString line;
    int numFoundMatches = 0;
    dataEntries.clear();
    dataEntries.reserve(maxDataEntries);
    ParseLineStatus::type previousParseLineStatus = ParseLineStatus::Error;
    for(quint32 lineNum = 0; !strm.atEnd(); ++lineNum)
    {
        line = strm.readLine();
        if (line.isNull()) {
            break;
        }

        ParseLineStatus::type parseLineStatus = parseLogFileLine(line, previousParseLineStatus, disabledLogLevels,
                                                                 filterContentRegExp, dataEntries, errorDescription);
        if (parseLineStatus == ParseLineStatus::Error) {
            return false;
        }

        previousParseLineStatus = parseLineStatus;

        if ( (parseLineStatus == ParseLineStatus::AppendedToLastEntry) ||
             (parseLineStatus == ParseLineStatus::FilteredEntry) )
        {
            continue;
        }
        else if (parseLineStatus == ParseLineStatus::CreatedNewEntry)
        {
            ++numFoundMatches;
            if (numFoundMatches >= maxDataEntries) {
                break;
            }
        }
    }

    endPos = strm.pos();
    return true;
}

LogViewerModel::LogFileParser::ParseLineStatus::type LogViewerModel::LogFileParser::parseLogFileLine(const QString & line,
                                                                                                     const ParseLineStatus::type previousParseLineStatus,
                                                                                                     const QVector<LogLevel::type> & disabledLogLevels,
                                                                                                     const QRegExp & filterContentRegExp,
                                                                                                     QVector<LogViewerModel::Data> & dataEntries,
                                                                                                     ErrorString & errorDescription)
{
    int currentIndex = m_logParsingRegex.indexIn(line);
    if (currentIndex < 0)
    {
        if (previousParseLineStatus == ParseLineStatus::FilteredEntry) {
            return ParseLineStatus::FilteredEntry;
        }

        if (!dataEntries.isEmpty())
        {
            LogViewerModel::Data & lastEntry = dataEntries.back();
            appendLogEntryLine(lastEntry, line);

            if (!filterContentRegExp.isEmpty() && (filterContentRegExp.indexIn(lastEntry.m_logEntry) >= 0)) {
                dataEntries.pop_back();
                return ParseLineStatus::FilteredEntry;
            }
        }

        return ParseLineStatus::AppendedToLastEntry;
    }

    QStringList capturedTexts = m_logParsingRegex.capturedTexts();

    if (capturedTexts.size() != 7) {
        errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: unexpected number of captures by regex"));
        errorDescription.details() += QString::number(capturedTexts.size());
        QNWARNING(errorDescription);
        return ParseLineStatus::Error;
    }

    bool convertedSourceLineNumberToInt = false;
    int sourceFileLineNumber = capturedTexts[4].toInt(&convertedSourceLineNumberToInt);
    if (!convertedSourceLineNumberToInt) {
        errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: failed to convert the source line number to int"));
        errorDescription.details() += capturedTexts[3];
        QNWARNING(errorDescription);
        return ParseLineStatus::Error;
    }

    Data entry;
    entry.m_timestamp = QDateTime::fromString(capturedTexts[1],
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                              QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")
#else
                                              QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")
#endif
                                             );

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    // Trying to add timezone info
    QTimeZone timezone(capturedTexts[2].toLocal8Bit());
    if (timezone.isValid()) {
        entry.m_timestamp.setTimeZone(timezone);
    }
#endif

    entry.m_sourceFileName = capturedTexts[3];
    entry.m_sourceFileLineNumber = sourceFileLineNumber;

    const QString & logLevel = capturedTexts[5];
    if (logLevel == QStringLiteral("Trace"))
    {
        entry.m_logLevel = LogLevel::TraceLevel;
    }
    else if (logLevel == QStringLiteral("Debug"))
    {
        entry.m_logLevel = LogLevel::DebugLevel;
    }
    else if (logLevel == QStringLiteral("Info"))
    {
        entry.m_logLevel = LogLevel::InfoLevel;
    }
    else if (logLevel == QStringLiteral("Warn"))
    {
        entry.m_logLevel = LogLevel::WarnLevel;
    }
    else if (logLevel == QStringLiteral("Error"))
    {
        entry.m_logLevel = LogLevel::ErrorLevel;
    }
    else
    {
        errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: failed to parse the log level"));
        errorDescription.details() += logLevel;
        QNWARNING(errorDescription);
        return ParseLineStatus::Error;
    }

    if (disabledLogLevels.contains(entry.m_logLevel)) {
        return ParseLineStatus::FilteredEntry;
    }

    if (filterContentRegExp.indexIn(capturedTexts[6]) >= 0) {
        return ParseLineStatus::FilteredEntry;
    }

    appendLogEntryLine(entry, capturedTexts[6]);
    dataEntries.push_back(entry);

    return ParseLineStatus::CreatedNewEntry;
}

void LogViewerModel::LogFileParser::appendLogEntryLine(LogViewerModel::Data & data, const QString & line) const
{
    int lineSize = line.size();
    if (lineSize < LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE)
    {
        if (!data.m_logEntry.isEmpty()) {
            data.m_logEntry += QStringLiteral("\n");
        }

        data.m_logEntry += line;
        ++data.m_numLogEntryLines;

        if (data.m_logEntryMaxNumCharsPerLine < lineSize) {
            data.m_logEntryMaxNumCharsPerLine = lineSize;
        }

        return;
    }

    int position = 0;
    while(position < lineSize)
    {
        int size = std::min(lineSize - position, LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE);
        QStringRef linePart(&line, position, size);

        if (!data.m_logEntry.isEmpty()) {
            data.m_logEntry += QStringLiteral("\n");
        }

        data.m_logEntry += linePart;
        ++data.m_numLogEntryLines;

        if (data.m_logEntryMaxNumCharsPerLine < size) {
            data.m_logEntryMaxNumCharsPerLine = size;
        }

        position += size;
    }
}

} // namespace quentier
