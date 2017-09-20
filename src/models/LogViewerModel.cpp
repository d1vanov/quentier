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

#include "LogViewerModel.h"
#include "LogViewerModelFileReaderAsync.h"
#include <quentier/utility/Utility.h>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#include <algorithm>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (5)
#define LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE (100)
#define LOG_VIEWER_MODEL_FETCH_ITEMS_BUCKET_SIZE (100)

namespace quentier {

LogViewerModel::LogViewerModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_currentLogFileInfo(),
    m_currentLogFileWatcher(),
    m_logParsingRegex(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{3})\\s+(\\w+)\\s+(.+)\\s+@\\s+(\\d+)\\s+\\[(\\w+)\\]:\\s(.+$)"),
                      Qt::CaseInsensitive, QRegExp::RegExp),
    m_currentLogFilePos(-1),
    m_currentParsedLogFileLines(0),
    m_currentLogFileLines(),
    m_pendingLogFileReadData(false),
    m_pReadLogFileIOThread(new QThread),
    m_data()
{
    QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                     this, QNSLOT(QThread,deleteLater));
    QObject::connect(this, QNSIGNAL(LogViewerModel,destroyed),
                     m_pReadLogFileIOThread, QNSLOT(QThread,quit));
    m_pReadLogFileIOThread->start(QThread::LowestPriority);
}

QString LogViewerModel::logFileName() const
{
    return m_currentLogFileInfo.fileName();
}

void LogViewerModel::setLogFileName(const QString & logFileName)
{
    QNDEBUG(QStringLiteral("LogViewerModel::setLogFileName: ") << logFileName);

    QFileInfo newLogFileInfo(QuentierLogFilesDirPath() + QStringLiteral("/") + logFileName);
    if (m_currentLogFileInfo.absoluteFilePath() == newLogFileInfo.absoluteFilePath()) {
        QNDEBUG(QStringLiteral("This log file is already set to the model"));
        return;
    }

    beginResetModel();

    m_currentLogFilePos = 0;
    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileInfo = newLogFileInfo;
    m_data.clear();
    m_currentParsedLogFileLines = 0;
    m_currentLogFileLines.clear();

    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentLogFileStartBytesRead = 0;

    QFile currentLogFile(newLogFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!currentLogFile.exists()))
    {
        ErrorString errorDescription(QT_TR_NOOP("Log file doesn't exist"));
        errorDescription.details() = m_currentLogFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        endResetModel();
        return;
    }

    bool open = currentLogFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = m_currentLogFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        endResetModel();
        return;
    }

    m_currentLogFileStartBytesRead = currentLogFile.read(m_currentLogFileStartBytes, sizeof(m_currentLogFileStartBytes));

    m_currentLogFileWatcher.addPath(m_currentLogFileInfo.absoluteFilePath());

    parseFullDataFromLogFile();
    endResetModel();
}

void LogViewerModel::clear()
{
    beginResetModel();

    m_currentLogFilePos = 0;
    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileInfo = QFileInfo();
    m_data.clear();
    m_currentParsedLogFileLines = 0;
    m_currentLogFileLines.clear();

    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentLogFileStartBytesRead = 0;

    endResetModel();
}

QString LogViewerModel::copyAllToString() const
{
    QString modelTextData;
    QTextStream strm(&modelTextData);

    for(int i = 0, size = m_data.size(); i < size; ++i)
    {
        const Data & entry = m_data.at(i);
        strm << entry.m_timestamp.toString(
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                           QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t")
#else
                                           QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz t")
#endif
                                          )
             << QStringLiteral(" ")
             << entry.m_sourceFileName
             << QStringLiteral(" @ ")
             << QString::number(entry.m_sourceFileLineNumber)
             << QStringLiteral(" [")
             << logLevelToString(entry.m_logLevel)
             << QStringLiteral(": ")
             << entry.m_logEntry
             << QStringLiteral("\n");

    }

    strm.flush();
    return modelTextData;
}

void LogViewerModel::copyAllToClipboard() const
{
    QNDEBUG(QStringLiteral("LogViewerModel::copyAllToClipboard"));

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't copy data to clipboard: got null pointer to clipboard from app"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QString modelTextData = copyAllToString();
    pClipboard->setText(modelTextData);
}

const LogViewerModel::Data * LogViewerModel::dataEntry(const int row) const
{
    if (Q_UNLIKELY((row < 0) || (row >= m_data.size()))) {
        return Q_NULLPTR;
    }

    return &(m_data.at(row));
}

int LogViewerModel::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return m_data.size();
    }

    return 0;
}

int LogViewerModel::columnCount(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return LOG_VIEWER_MODEL_COLUMN_COUNT;
    }

    return 0;
}

QVariant LogViewerModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= LOG_VIEWER_MODEL_COLUMN_COUNT)) {
        return QVariant();
    }

    int rowIndex = index.row();
    if ((rowIndex < 0) || (rowIndex >= m_data.size())) {
        return QVariant();
    }

    const Data & dataEntry = m_data.at(rowIndex);

    switch(columnIndex)
    {
    case Columns::Timestamp:
        return dataEntry.m_timestamp;
    case Columns::SourceFileName:
        return dataEntry.m_sourceFileName;
    case Columns::SourceFileLineNumber:
        return dataEntry.m_sourceFileLineNumber;
    case Columns::LogLevel:
        return dataEntry.m_logLevel;
    case Columns::LogEntry:
        return dataEntry.m_logEntry;
    default:
        return QVariant();
    }
}

QVariant LogViewerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::Timestamp:
        // TRANSLATOR: log entry's datetime
        return QVariant(tr("Datetime"));
    case Columns::SourceFileName:
        // TRANSLATOR: log source file name
        return QVariant(tr("Source file"));
    case Columns::SourceFileLineNumber:
        // TRANSLATOR: line number within the source file
        return QVariant(tr("Line number"));
    case Columns::LogLevel:
        // TRANSLATOR: the log level i.e. Trace, Debug, Info etc.
        return QVariant(tr("Log level"));
    case Columns::LogEntry:
        // TRANSLATOR: the actual recorded log message
        return QVariant(tr("Message"));
    default:
        return QVariant();
    }
}

bool LogViewerModel::canFetchMore(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return false;
    }

    return m_currentParsedLogFileLines < m_currentLogFileLines.size();
}

void LogViewerModel::fetchMore(const QModelIndex & parent)
{
    if (parent.isValid()) {
        return;
    }

    parseNextChunkOfLogFileLines();
}

void LogViewerModel::onFileChanged(const QString & path)
{
    if (m_currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    QFile currentLogFile(path);
    if (!currentLogFile.isOpen() && !currentLogFile.open(QIODevice::ReadOnly))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = path;
        emit notifyError(errorDescription);
        return;
    }

    char startBytes[sizeof(m_currentLogFileStartBytes)];
    qint64 startBytesRead = currentLogFile.read(startBytes, sizeof(startBytes));

    bool fileStartBytesChanged = false;
    if (startBytesRead != m_currentLogFileStartBytesRead)
    {
        fileStartBytesChanged = true;
    }
    else
    {
        for(qint64 i = 0, size = std::min(startBytesRead, qint64(sizeof(startBytes))); i < size; ++i)
        {
            size_t index = static_cast<size_t>(i);
            if (startBytes[index] != m_currentLogFileStartBytes[index]) {
                fileStartBytesChanged = true;
                break;
            }
        }
    }

    if (fileStartBytesChanged)
    {
        // The change within the file is not just the addition of new log entry, the file's start bytes changed,
        // hence should reset the model

        m_currentLogFileStartBytesRead = startBytesRead;

        for(qint64 i = 0, size = sizeof(startBytes); i < size; ++i) {
            size_t index = static_cast<size_t>(i);
            m_currentLogFileStartBytes[index] = startBytes[index];
        }

        beginResetModel();
        m_currentLogFilePos = 0;
        m_data.clear();
        m_currentParsedLogFileLines = 0;
        m_currentLogFileLines.clear();
        parseFullDataFromLogFile();
        endResetModel();

        return;
    }

    // New log entries were appended to the log file, need to process them
    parseDataFromLogFileFromCurrentPos();
}

void LogViewerModel::onFileRemoved(const QString & path)
{
    if (m_currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    beginResetModel();
    m_currentLogFileInfo = QFileInfo();
    m_currentLogFileWatcher.removePath(path);
    m_currentLogFileStartBytesRead = 0;
    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentLogFilePos = 0;
    m_data.clear();
    m_currentParsedLogFileLines = 0;
    m_currentLogFileLines.clear();
    endResetModel();
}

void LogViewerModel::onFileReadAsyncReady(qint64 pos, QString readData, ErrorString errorDescription)
{
    FileReaderAsync * pFileReaderAsync = qobject_cast<FileReaderAsync*>(sender());
    if (pFileReaderAsync) {
        pFileReaderAsync->deleteLater();
    }

    m_pendingLogFileReadData = false;

    if (!errorDescription.isEmpty()) {
        emit notifyError(errorDescription);
        return;
    }

    m_currentLogFilePos = pos;
    m_currentLogFileLines.append(readData.split(QChar::fromLatin1('\n'), QString::SkipEmptyParts));
    parseNextChunkOfLogFileLines();
}

void LogViewerModel::parseFullDataFromLogFile()
{
    m_currentLogFilePos = 0;
    parseDataFromLogFileFromCurrentPos();
}

void LogViewerModel::parseDataFromLogFileFromCurrentPos()
{
    FileReaderAsync * pFileReaderAsync = new FileReaderAsync(m_currentLogFileInfo.absoluteFilePath(),
                                                             m_currentLogFilePos, this);
    pFileReaderAsync->moveToThread(m_pReadLogFileIOThread);
    QObject::connect(this, QNSIGNAL(LogViewerModel,startAsyncLogFileReading),
                     pFileReaderAsync, QNSLOT(FileReaderAsync,onStartReading),
                     Qt::QueuedConnection);
    QObject::connect(pFileReaderAsync, QNSIGNAL(FileReaderAsync,finished,qint64,QString,ErrorString),
                     this, QNSLOT(LogViewerModel,onFileReadAsyncReady,qint64,QString,ErrorString),
                     Qt::QueuedConnection);

    m_pendingLogFileReadData = true;

    emit startAsyncLogFileReading();
}

void LogViewerModel::parseNextChunkOfLogFileLines()
{
    QList<LogViewerModel::Data> readLogFileEntries;
    int lastParsedLogFileLine = 0;
    bool res = parseNextChunkOfLogFileLines(m_currentParsedLogFileLines, readLogFileEntries, lastParsedLogFileLine);
    m_currentParsedLogFileLines = lastParsedLogFileLine;
    if (!res) {
        return;
    }

    int numNewRows = readLogFileEntries.size();
    if (Q_UNLIKELY(numNewRows < 0)) {
        return;
    }

    beginInsertRows(QModelIndex(), m_data.size(), m_data.size() + numNewRows);

    for(int i = 0, numEntries = readLogFileEntries.size(); i < numEntries; ++i) {
        m_data.append(readLogFileEntries[i]);
    }

    endInsertRows();
}

bool LogViewerModel::parseNextChunkOfLogFileLines(const int lineNumFrom, QList<Data> & readLogFileEntries,
                                                  int & lastParsedLogFileLine)
{
    int estimatedLastLine = lineNumFrom + LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE;
    readLogFileEntries.reserve(std::max(readLogFileEntries.size(), 0) + LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE / 10);  // Just a rough guess

    int numFoundMatches = 0;
    lastParsedLogFileLine = 0;
    for(int i = lineNumFrom, numLines = m_currentLogFileLines.size(); i < numLines; ++i)
    {
        const QString & line = m_currentLogFileLines.at(i);
        int currentIndex = m_logParsingRegex.indexIn(line);
        if (currentIndex < 0)
        {
            lastParsedLogFileLine = i;

            if ((numFoundMatches == 0) || readLogFileEntries.empty()) {
                continue;
            }

            LogViewerModel::Data & lastEntry = readLogFileEntries.back();
            lastEntry.m_logEntry += line;
            continue;
        }

        if (i >= estimatedLastLine) {
            break;
        }

        lastParsedLogFileLine = i;

        QStringList capturedTexts = m_logParsingRegex.capturedTexts();

        if (capturedTexts.size() != 7) {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: unexpected number of captures by regex"));
            errorDescription.details() += QString::number(capturedTexts.size());
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return false;
        }

        bool convertedSourceLineNumberToInt = false;
        int sourceFileLineNumber = capturedTexts[4].toInt(&convertedSourceLineNumberToInt);
        if (!convertedSourceLineNumberToInt) {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to convert the source line number to int"));
            errorDescription.details() += capturedTexts[3];
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
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
        else if (logLevel == QStringLiteral("Fatal")) {
            entry.m_logLevel = LogLevel::FatalLevel;
        }
        else
        {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to parse the log level"));
            errorDescription.details() += logLevel;
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return false;
        }

        entry.m_logEntry = capturedTexts[6];
        entry.m_logEntry += QStringLiteral("\n");

        readLogFileEntries.push_back(entry);

        ++numFoundMatches;
    }

    return true;
}

QString LogViewerModel::logLevelToString(LogLevel::type logLevel)
{
    switch(logLevel)
    {
    case LogLevel::TraceLevel:
        return QStringLiteral("Trace");
    case LogLevel::DebugLevel:
        return QStringLiteral("Debug");
    case LogLevel::InfoLevel:
        return QStringLiteral("Info");
    case LogLevel::WarnLevel:
        return QStringLiteral("Warn");
    case LogLevel::ErrorLevel:
        return QStringLiteral("Error");
    case LogLevel::FatalLevel:
        return QStringLiteral("Fatal");
    default:
        return QStringLiteral("Unknown");
    }
}

QTextStream & LogViewerModel::Data::print(QTextStream & strm) const
{
    strm << QStringLiteral("Timestamp = ") << printableDateTimeFromTimestamp(m_timestamp.toMSecsSinceEpoch())
         << QStringLiteral(", source file name = ") << m_sourceFileName
         << QStringLiteral(", line number = ") << m_sourceFileLineNumber
         << QStringLiteral(", log level = ") << m_logLevel
         << QStringLiteral(", log entry: ") << m_logEntry;
    return strm;
}

} // namespace quentier
