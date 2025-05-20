/*
 * Copyright 2018-2025 Dmitry Ivanov
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

#include <lib/preferences/keys/Logging.h>

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DateTime.h>
#include <quentier/utility/StandardPaths.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <QTimeZone>

#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

#define LVMPDEBUG(message)                                                     \
    if (m_internalLogEnabled) {                                                \
        QString msg;                                                           \
        QDebug dbg(&msg);                                                      \
        dbg.nospace();                                                         \
        dbg.noquote();                                                         \
        dbg << message;                                                        \
        QString relativeSourceFileName = QStringLiteral(__FILE__);             \
        auto prefixIndex = relativeSourceFileName.indexOf(                     \
            QStringLiteral("libquentier"), Qt::CaseInsensitive);               \
        if (prefixIndex >= 0) {                                                \
            relativeSourceFileName.remove(0, prefixIndex);                     \
        }                                                                      \
        else {                                                                 \
            QString appName = QCoreApplication::applicationName().toLower();   \
            prefixIndex =                                                      \
                relativeSourceFileName.indexOf(appName, Qt::CaseInsensitive);  \
            if (prefixIndex >= 0) {                                            \
                relativeSourceFileName.remove(                                 \
                    0, prefixIndex + appName.size() + 1);                      \
            }                                                                  \
        }                                                                      \
        utility::DateTimePrintOptions options(                                 \
            utility::DateTimePrintOption::IncludeMilliseconds |                \
            utility::DateTimePrintOption::IncludeTimezone);                    \
        QString fullMsg = utility::printableDateTimeFromTimestamp(             \
                              QDateTime::currentMSecsSinceEpoch(), options) +  \
            QStringLiteral(" ") + relativeSourceFileName +                     \
            QStringLiteral(QNLOG_FILE_LINENUMBER_DELIMITER) +                  \
            QString::number(__LINE__) + QStringLiteral(": ") + msg +           \
            QStringLiteral("\n");                                              \
        m_internalLogFile.write(fullMsg.toUtf8());                             \
        m_internalLogFile.flush();                                             \
    }

namespace quentier {

#define REGEX_QNLOG_DATE                                                       \
    "^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{1,17})(?:\\s+(\\w+))?"

// note: QNLOG_FILE_LINENUMBER_DELIMITER is here incorporated into regex
#define REGEX_QNLOG_SOURCE_LINENUMBER "([a-zA-Z0-9\\\\\\/_.]+):(\\d+)"

// full logline regex
#define REGEX_QNLOG_LINE                                                       \
    REGEX_QNLOG_DATE                                                           \
    "\\s+" REGEX_QNLOG_SOURCE_LINENUMBER                                       \
    "\\s+"                                                                     \
    "\\[(\\w+)\\]"                                                             \
    "(?:\\s+\\[((?:\\w+|:|-|_)+)\\])?:\\s+(.+$)"

LogViewerModel::LogFileParser::LogFileParser() :
    m_logParsingRegex(
        QStringLiteral(REGEX_QNLOG_LINE),
        QRegularExpression::CaseInsensitiveOption),
    m_internalLogFile(
        applicationPersistentStoragePath() +
        QStringLiteral("/logs-quentier/LogViewerModelLogFileParserLog.txt")),
    m_internalLogEnabled(false)
{
    ApplicationSettings appSettings;

    QVariant enableLogViewerInternalLogsValue;
    {
        appSettings.beginGroup(preferences::keys::loggingGroup.data());
        ApplicationSettings::GroupCloser groupCloser{appSettings};

        enableLogViewerInternalLogsValue = appSettings.value(
            preferences::keys::enableLogViewerInternalLogs.data());
    }

    bool enableLogViewerInternalLogs = false;
    if (enableLogViewerInternalLogsValue.isValid()) {
        enableLogViewerInternalLogs =
            enableLogViewerInternalLogsValue.toBool();
    }

    setInternalLogEnabled(enableLogViewerInternalLogs);
}

bool LogViewerModel::LogFileParser::parseDataEntriesFromLogFile(
    const qint64 fromPos, const int maxDataEntries,
    const QList<LogLevel> & disabledLogLevels,
    const QRegularExpression & filterContentRegExp, QFile & logFile,
    QList<LogViewerModel::Data> & dataEntries, qint64 & endPos,
    ErrorString & errorDescription)
{
    LVMPDEBUG(
        "LogViewerModel::LogFileParser::parseDataEntriesFromLogFile: "
        << "from pos = " << fromPos
        << ", max data entries = " << maxDataEntries);

    if (!logFile.isOpen() && !logFile.open(QIODevice::ReadOnly)) {
        QFileInfo targetFileInfo{logFile};
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "LogViewerModel::LogFileParser",
            "Can't open log file for reading"));
        errorDescription.details() = targetFileInfo.absoluteFilePath();
        LVMPDEBUG(errorDescription);
        return false;
    }

    QTextStream strm{&logFile};

    if (!strm.seek(fromPos)) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "LogViewerModel::LogFileParser",
            "Failed to read the data from log file: failed to seek at "
            "position"));

        errorDescription.details() = QString::number(fromPos);
        LVMPDEBUG(errorDescription);
        return false;
    }

    QString line;
    int numFoundMatches = 0;
    dataEntries.clear();
    dataEntries.reserve(maxDataEntries);
    ParseLineStatus previousParseLineStatus = ParseLineStatus::FilteredEntry;
    bool insideEntry = false;
    while (!strm.atEnd()) {
        line = strm.readLine();
        LVMPDEBUG("Processing line " << line);

        auto parseLineStatus = parseLogFileLine(
            line, previousParseLineStatus, disabledLogLevels,
            filterContentRegExp, dataEntries, errorDescription);

        if (parseLineStatus == ParseLineStatus::Error) {
            LVMPDEBUG("Returning error: " << errorDescription);
            return false;
        }

        previousParseLineStatus = parseLineStatus;

        if (parseLineStatus == ParseLineStatus::FilteredEntry) {
            if (insideEntry && (numFoundMatches > 0)) {
                --numFoundMatches;
                insideEntry = false;
            }

            continue;
        }
        else if (parseLineStatus == ParseLineStatus::AppendedToLastEntry) {
            LVMPDEBUG("No new entry created, continuing");
            continue;
        }
        else if (parseLineStatus == ParseLineStatus::CreatedNewEntry) {
            insideEntry = true;
            ++numFoundMatches;
            LVMPDEBUG(
                "New entry was created, " << numFoundMatches
                                          << " entries found now");

            if (numFoundMatches >= maxDataEntries) {
                LVMPDEBUG(
                    "Exceeded the allowed number of entries "
                    "to parse, returning");
                break;
            }
        }
    }

    endPos = strm.pos();
    LVMPDEBUG("End pos before returning = " << endPos);
    return true;
}

LogViewerModel::LogFileParser::ParseLineStatus
LogViewerModel::LogFileParser::parseLogFileLine(
    const QString & line, const ParseLineStatus previousParseLineStatus,
    const QList<LogLevel> & disabledLogLevels,
    const QRegularExpression & filterContentRegExp,
    QList<LogViewerModel::Data> & dataEntries, ErrorString & errorDescription)
{
    const auto logParsingRegexMatch = m_logParsingRegex.match(line);
    if (!logParsingRegexMatch.hasMatch()) {
        if (previousParseLineStatus == ParseLineStatus::FilteredEntry) {
            return ParseLineStatus::FilteredEntry;
        }

        if (!dataEntries.isEmpty()) {
            LogViewerModel::Data & lastEntry = dataEntries.back();
            appendLogEntryLine(lastEntry, line);

            if (!filterContentRegExp.pattern().isEmpty() &&
                (filterContentRegExp.match(lastEntry.m_logEntry).hasMatch()))
            {
                dataEntries.pop_back();
                return ParseLineStatus::FilteredEntry;
            }
        }

        return ParseLineStatus::AppendedToLastEntry;
    }

    QStringList capturedTexts = logParsingRegexMatch.capturedTexts();

    if (capturedTexts.size() != 8) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "LogViewerModel::LogFileParser",
            "Error parsing the log file's contents: unexpected number of "
            "captures by regex"));

        errorDescription.details() += QString::number(capturedTexts.size());
        return ParseLineStatus::Error;
    }

    bool convertedSourceLineNumberToInt = false;

    int sourceFileLineNumber =
        capturedTexts[4].toInt(&convertedSourceLineNumberToInt);

    if (!convertedSourceLineNumberToInt) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "LogViewerModel::LogFileParser",
            "Error parsing the log file's contents: failed to convert the "
            "source line number to int"));

        errorDescription.details() += capturedTexts[3];
        return ParseLineStatus::Error;
    }

    Data entry;
    entry.m_timestamp = QDateTime::fromString(
        capturedTexts[1], QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

    // Trying to add timezone info
    QTimeZone timezone{capturedTexts[2].toLocal8Bit()};
    if (timezone.isValid()) {
        entry.m_timestamp.setTimeZone(timezone);
    }

    entry.m_sourceFileName = capturedTexts[3];
    entry.m_sourceFileLineNumber = sourceFileLineNumber;

    const QString & logLevel = capturedTexts[5];
    if (logLevel == QStringLiteral("Trace")) {
        entry.m_logLevel = LogLevel::Trace;
    }
    else if (logLevel == QStringLiteral("Debug")) {
        entry.m_logLevel = LogLevel::Debug;
    }
    else if (logLevel == QStringLiteral("Info")) {
        entry.m_logLevel = LogLevel::Info;
    }
    else if (logLevel == QStringLiteral("Warn")) {
        entry.m_logLevel = LogLevel::Warning;
    }
    else if (logLevel == QStringLiteral("Error")) {
        entry.m_logLevel = LogLevel::Error;
    }
    else {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "LogViewerModel::LogFileParser",
            "Error parsing the log file's contents: failed to parse the log "
            "level"));

        errorDescription.details() += logLevel;
        return ParseLineStatus::Error;
    }

    if (disabledLogLevels.contains(entry.m_logLevel)) {
        return ParseLineStatus::FilteredEntry;
    }

    entry.m_component = capturedTexts[6];

    if (!filterContentRegExp.pattern().isEmpty())
    {
        if (filterContentRegExp.isValid())
        {
            return ParseLineStatus::FilteredEntry;
        }
    }
    else if (!filterContentRegExp.match(capturedTexts[7]).hasMatch() &&
             !filterContentRegExp.match(capturedTexts[1]).hasMatch() &&
             !filterContentRegExp.match(entry.m_sourceFileName).hasMatch())
    {
        return ParseLineStatus::FilteredEntry;
    }

    appendLogEntryLine(entry, capturedTexts[7]);
    dataEntries.push_back(entry);

    return ParseLineStatus::CreatedNewEntry;
}

void LogViewerModel::LogFileParser::appendLogEntryLine(
    LogViewerModel::Data & data, const QString & line) const
{
    if (!data.m_logEntry.isEmpty()) {
        data.m_logEntry += QStringLiteral("\n");
    }

    data.m_logEntry += line;
}

void LogViewerModel::LogFileParser::setInternalLogEnabled(const bool enabled)
{
    if (m_internalLogEnabled == enabled) {
        return;
    }

    m_internalLogEnabled = enabled;

    if (m_internalLogEnabled) {
        m_internalLogFile.open(QIODevice::WriteOnly);
    }
    else {
        m_internalLogFile.close();
    }
}

} // namespace quentier
