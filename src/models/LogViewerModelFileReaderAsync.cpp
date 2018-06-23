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

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

namespace quentier {

LogViewerModel::FileReaderAsync::FileReaderAsync(QString targetFilePath, QObject * parent) :
    QObject(parent),
    m_targetFile(targetFilePath),
    m_logParsingRegex(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{3})\\s+(\\w+)\\s+(.+)\\s+@\\s+(\\d+)\\s+\\[(\\w+)\\]:\\s(.+$)"),
                      Qt::CaseInsensitive, QRegExp::RegExp)
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

void LogViewerModel::FileReaderAsync::onReadDataEntriesFromLogFile(qint64 fromPos, int maxDataEntries)
{
    QVector<LogViewerModel::Data> dataEntries;
    qint64 endPos = -1;
    ErrorString errorDescription;
    bool res = readDataEntriesFromLogFile(fromPos, maxDataEntries, dataEntries, endPos, errorDescription);
    if (res) {
        Q_EMIT readLogFileDataEntries(fromPos, endPos, dataEntries, ErrorString());
    }
    else {
        Q_EMIT readLogFileDataEntries(fromPos, -1, QVector<LogViewerModel::Data>(), errorDescription);
    }
}

bool LogViewerModel::FileReaderAsync::readDataEntriesFromLogFile(const qint64 fromPos, const int maxDataEntries,
                                                                 QVector<LogViewerModel::Data> & dataEntries,
                                                                 qint64 & endPos, ErrorString & errorDescription)
{
    if (!m_targetFile.isOpen() && !m_targetFile.open(QIODevice::ReadOnly)) {
        QFileInfo targetFileInfo(m_targetFile);
        errorDescription.setBase(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = targetFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        return false;
    }

    QTextStream strm(&m_targetFile);

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
    for(quint32 lineNum = 0; !strm.atEnd(); ++lineNum)
    {
        line = strm.readLine();
        if (line.isNull()) {
            break;
        }

        int currentIndex = m_logParsingRegex.indexIn(line);
        if (currentIndex < 0)
        {
            if ((numFoundMatches == 0) || dataEntries.empty()) {
                continue;
            }

            LogViewerModel::Data & lastEntry = dataEntries.back();
            appendLogEntryLine(lastEntry, line);
            continue;
        }

        QStringList capturedTexts = m_logParsingRegex.capturedTexts();

        if (capturedTexts.size() != 7) {
            errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: unexpected number of captures by regex"));
            errorDescription.details() += QString::number(capturedTexts.size());
            QNWARNING(errorDescription);
            return false;
        }

        bool convertedSourceLineNumberToInt = false;
        int sourceFileLineNumber = capturedTexts[4].toInt(&convertedSourceLineNumberToInt);
        if (!convertedSourceLineNumberToInt) {
            errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: failed to convert the source line number to int"));
            errorDescription.details() += capturedTexts[3];
            QNWARNING(errorDescription);
            return false;
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
        if (logLevel == QStringLiteral("Trace")) {
            entry.m_logLevel = LogLevel::TraceLevel;
        }
        else if (logLevel == QStringLiteral("Debug")) {
            entry.m_logLevel = LogLevel::DebugLevel;
        }
        else if (logLevel == QStringLiteral("Info")) {
            entry.m_logLevel = LogLevel::InfoLevel;
        }
        else if (logLevel == QStringLiteral("Warn")) {
            entry.m_logLevel = LogLevel::WarnLevel;
        }
        else if (logLevel == QStringLiteral("Error")) {
            entry.m_logLevel = LogLevel::ErrorLevel;
        }
        else
        {
            errorDescription.setBase(QT_TR_NOOP("Error parsing the log file's contents: failed to parse the log level"));
            errorDescription.details() += logLevel;
            QNWARNING(errorDescription);
            return false;
        }

        appendLogEntryLine(entry, capturedTexts[6]);
        dataEntries.push_back(entry);
        ++numFoundMatches;

        if (numFoundMatches >= maxDataEntries) {
            break;
        }
    }

    endPos = strm.pos();
    return true;
}

void LogViewerModel::FileReaderAsync::appendLogEntryLine(LogViewerModel::Data & data, const QString & line) const
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
