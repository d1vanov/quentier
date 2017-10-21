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
#include <quentier/utility/EventLoopWithExitStatus.h>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QStringRef>
#include <QTimer>
#include <QFile>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#include <algorithm>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (5)
#define LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE (100)
#define LOG_VIEWER_MODEL_FETCH_ITEMS_BUCKET_SIZE (100)
#define LOG_VIEWER_MODEL_LOG_FILE_POLLING_TIMER_MSEC (500)
#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

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
    m_currentLogFileSize(0),
    m_currentLogFileSizePollingTimer(),
    m_pendingLogFileReadData(false),
    m_pReadLogFileIOThread(new QThread),
    m_pFileReaderAsync(Q_NULLPTR),
    m_data(),
    m_pendingCurrentLogFileWipe(false),
    m_wipeCurrentLogFileResultStatus(false),
    m_wipeCurrentLogFileErrorDescription()
{
    QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                     this, QNSLOT(QThread,deleteLater));
    QObject::connect(this, QNSIGNAL(LogViewerModel,destroyed),
                     m_pReadLogFileIOThread, QNSLOT(QThread,quit));
    m_pReadLogFileIOThread->start(QThread::LowestPriority);

    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileChanged,QString),
                     this, QNSLOT(LogViewerModel,onFileChanged,QString));
    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileRemoved,QString),
                     this, QNSLOT(LogViewerModel,onFileRemoved,QString));
}

QString LogViewerModel::logFileName() const
{
    return m_currentLogFileInfo.fileName();
}

void LogViewerModel::setLogFileName(const QString & logFileName)
{
    QFileInfo newLogFileInfo(QuentierLogFilesDirPath() + QStringLiteral("/") + logFileName);
    if (m_currentLogFileInfo.absoluteFilePath() == newLogFileInfo.absoluteFilePath()) {
        return;
    }

    beginResetModel();

    m_currentLogFilePos = 0;
    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    QString currentLogFilePath = m_currentLogFileInfo.absoluteFilePath();
    if (!currentLogFilePath.isEmpty()) {
        m_currentLogFileWatcher.removePath(currentLogFilePath);
    }

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
        Q_EMIT notifyError(errorDescription);
        endResetModel();
        return;
    }

    bool open = currentLogFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = m_currentLogFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        endResetModel();
        return;
    }

    m_currentLogFileStartBytesRead = currentLogFile.read(m_currentLogFileStartBytes, sizeof(m_currentLogFileStartBytes));
    currentLogFile.close();

    m_currentLogFileSize = m_currentLogFileInfo.size();
    m_currentLogFileSizePollingTimer.start(LOG_VIEWER_MODEL_LOG_FILE_POLLING_TIMER_MSEC, this);

    // NOTE: for unknown reason QFileSystemWatcher from Qt4 fails to add any log file path + also hangs the process;
    // hence, using only the current log file size polling timer as the means to watch the log file's changes with Qt4
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString filePath = m_currentLogFileInfo.absoluteFilePath();
    if (!m_currentLogFileWatcher.files().contains(filePath)) {
        m_currentLogFileWatcher.addPath(filePath);
    }
#endif

    parseFullDataFromLogFile();
    endResetModel();
}

bool LogViewerModel::wipeCurrentLogFile(ErrorString & errorDescription)
{
    if (!m_pFileReaderAsync) {
        return wipeCurrentLogFileImpl(errorDescription);
    }

    // Need to wait until the file reader finishes its async job
    m_pendingCurrentLogFileWipe = true;

    // Need to set up the blocking event loop to return bool from this method

    int eventLoopResult = -1;
    {
        QTimer timer;
        timer.setInterval(10000);
        timer.setSingleShot(true);

        EventLoopWithExitStatus loop;
        QObject::connect(&timer, QNSIGNAL(QTimer,timeout), &loop, QNSLOT(EventLoopWithExitStatus,exitAsTimeout));
        QObject::connect(this, QNSIGNAL(LogViewerModel,wipeCurrentLogFileFinished), &loop, QNSLOT(EventLoopWithExitStatus,exitAsSuccess));

        timer.start();
        eventLoopResult = loop.exec();
    }

    if (Q_UNLIKELY(eventLoopResult == EventLoopWithExitStatus::ExitStatus::Timeout)) {
        errorDescription.setBase(QT_TR_NOOP("Wiping out the current log file failed to finish in time"));
        m_pendingCurrentLogFileWipe = false;
        return false;
    }

    errorDescription = m_wipeCurrentLogFileErrorDescription;
    bool res = m_wipeCurrentLogFileResultStatus;

    m_wipeCurrentLogFileResultStatus = false;
    m_wipeCurrentLogFileErrorDescription.clear();

    return res;
}

void LogViewerModel::clear()
{
    beginResetModel();

    m_currentLogFilePos = 0;
    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileInfo = QFileInfo();
    m_data.clear();
    m_currentParsedLogFileLines = 0;
    m_currentLogFileLines.clear();

    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentLogFileStartBytesRead = 0;

    m_pendingCurrentLogFileWipe = false;
    m_wipeCurrentLogFileResultStatus = false;
    m_wipeCurrentLogFileErrorDescription.clear();

    endResetModel();
}

const LogViewerModel::Data * LogViewerModel::dataEntry(const int row) const
{
    if (Q_UNLIKELY((row < 0) || (row >= m_data.size()))) {
        return Q_NULLPTR;
    }

    return &(m_data.at(row));
}

QString LogViewerModel::dataEntryToString(const LogViewerModel::Data & dataEntry) const
{
    QString result;
    QTextStream strm(&result);

    strm << dataEntry.m_timestamp.toString(
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                             QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t")
#else
                                             QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")
#endif
                                             )
         << QStringLiteral(" ")
         << dataEntry.m_sourceFileName
         << QStringLiteral(" @ ")
         << QString::number(dataEntry.m_sourceFileLineNumber)
         << QStringLiteral(" [")
         << LogViewerModel::logLevelToString(dataEntry.m_logLevel)
         << QStringLiteral("]: ")
         << dataEntry.m_logEntry
         << QStringLiteral("\n");

    strm.flush();
    return result;
}

QColor LogViewerModel::backgroundColorForLogLevel(const LogLevel::type logLevel) const
{
    int alpha = 140;
    switch(logLevel)
    {
    case LogLevel::TraceLevel:
        return QColor(127, 174, 255, alpha);   // Light blue
    case LogLevel::DebugLevel:
        return QColor(127, 255, 142, alpha);   // Light green
    case LogLevel::InfoLevel:
        return QColor(252, 255, 127, alpha);   // Light yellow
    case LogLevel::WarnLevel:
        return QColor(255, 212, 127, alpha);   // Orange
    case LogLevel::ErrorLevel:
        return QColor(255, 128, 127, alpha);   // Pink
    case LogLevel::FatalLevel:
        return QColor(255, 38, 10, alpha);     // Red
    default:
        return QColor();
    }
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
        Q_EMIT notifyError(errorDescription);
        return;
    }

    char startBytes[sizeof(m_currentLogFileStartBytes)];
    qint64 startBytesRead = currentLogFile.read(startBytes, sizeof(startBytes));

    currentLogFile.close();

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
        m_currentLogFileSize = 0;
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
    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();
    m_data.clear();
    m_currentParsedLogFileLines = 0;
    m_currentLogFileLines.clear();
    endResetModel();
}

void LogViewerModel::onFileReadAsyncReady(qint64 pos, QString readData, ErrorString errorDescription)
{
    if (m_pFileReaderAsync)
    {
        QObject::disconnect(m_pFileReaderAsync);
        Q_EMIT deleteFileReaderAsync();
        m_pFileReaderAsync = Q_NULLPTR;

        if (m_pendingCurrentLogFileWipe)
        {
            m_pendingCurrentLogFileWipe = false;

            m_wipeCurrentLogFileErrorDescription.clear();
            m_wipeCurrentLogFileResultStatus = wipeCurrentLogFileImpl(m_wipeCurrentLogFileErrorDescription);
            Q_EMIT wipeCurrentLogFileFinished();

            return;
        }
    }

    m_pendingLogFileReadData = false;

    // NOTE: it is necessary to create a new object of QFileInfo type
    // because the existing m_currentLogFileInfo has cached value of
    // current log file size, it doesn't update in live regime
    QFileInfo currentLogFileInfo(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileSize = currentLogFileInfo.size();

    if (!errorDescription.isEmpty()) {
        Q_EMIT notifyError(errorDescription);
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
    if (Q_UNLIKELY(m_pFileReaderAsync)) {
        QObject::disconnect(m_pFileReaderAsync);
        m_pFileReaderAsync->deleteLater();
        m_pFileReaderAsync = Q_NULLPTR;
    }

    m_pFileReaderAsync = new FileReaderAsync(m_currentLogFileInfo.absoluteFilePath(),
                                             m_currentLogFilePos);
    m_pFileReaderAsync->moveToThread(m_pReadLogFileIOThread);
    QObject::connect(this, QNSIGNAL(LogViewerModel,startAsyncLogFileReading),
                     m_pFileReaderAsync, QNSLOT(FileReaderAsync,onStartReading),
                     Qt::QueuedConnection);
    QObject::connect(m_pFileReaderAsync, QNSIGNAL(FileReaderAsync,finished,qint64,QString,ErrorString),
                     this, QNSLOT(LogViewerModel,onFileReadAsyncReady,qint64,QString,ErrorString),
                     Qt::QueuedConnection);
    QObject::connect(this, QNSIGNAL(LogViewerModel,deleteFileReaderAsync),
                     m_pFileReaderAsync, QNSLOT(FileReaderAsync,deleteLater));

    m_pendingLogFileReadData = true;

    Q_EMIT startAsyncLogFileReading();
}

void LogViewerModel::parseNextChunkOfLogFileLines()
{
    QList<LogViewerModel::Data> readLogFileEntries;
    int lastParsedLogFileLine = 0;
    bool res = parseNextChunkOfLogFileLines(m_currentParsedLogFileLines, readLogFileEntries, lastParsedLogFileLine);
    m_currentParsedLogFileLines = lastParsedLogFileLine + 1;
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
            appendLogEntryLine(lastEntry, line);
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
            Q_EMIT notifyError(errorDescription);
            return false;
        }

        bool convertedSourceLineNumberToInt = false;
        int sourceFileLineNumber = capturedTexts[4].toInt(&convertedSourceLineNumberToInt);
        if (!convertedSourceLineNumberToInt) {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to convert the source line number to int"));
            errorDescription.details() += capturedTexts[3];
            QNWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
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
            Q_EMIT notifyError(errorDescription);
            return false;
        }

        appendLogEntryLine(entry, capturedTexts[6]);
        readLogFileEntries.push_back(entry);
        ++numFoundMatches;
    }

    return true;
}

void LogViewerModel::timerEvent(QTimerEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    if (pEvent->timerId() == m_currentLogFileSizePollingTimer.timerId())
    {
        if (m_currentLogFileInfo.absoluteFilePath().isEmpty()) {
            m_currentLogFileSizePollingTimer.stop();
            return;
        }

        // NOTE: it is necessary to create a new object of QFileInfo type
        // because the existing m_currentLogFileInfo has cached value of
        // current log file size, it doesn't update in live regime
        QFileInfo currentLogFileInfo(m_currentLogFileInfo.absoluteFilePath());
        qint64 size = currentLogFileInfo.size();
        if (size != m_currentLogFileSize) {
            m_currentLogFileSize = size;
            onFileChanged(m_currentLogFileInfo.absoluteFilePath());
        }

        return;
    }

    QAbstractTableModel::timerEvent(pEvent);
}

void LogViewerModel::appendLogEntryLine(LogViewerModel::Data & data, const QString & line) const
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

bool LogViewerModel::wipeCurrentLogFileImpl(ErrorString & errorDescription)
{
    if (!m_currentLogFileInfo.exists()) {
        errorDescription.setBase(QT_TR_NOOP("No log file is selected"));
        return false;
    }

    if (Q_UNLIKELY(!m_currentLogFileInfo.isFile())) {
        errorDescription.setBase(QT_TR_NOOP("Currently selected log file name does not really correspond to a file"));
        return false;
    }

    if (Q_UNLIKELY(!m_currentLogFileInfo.isWritable())) {
        errorDescription.setBase(QT_TR_NOOP("Currently selected log file is not writable"));
        return false;
    }

    beginResetModel();

    QFile currentLogFile(m_currentLogFileInfo.absoluteFilePath());
    bool res = currentLogFile.resize(qint64(0));
    if (Q_UNLIKELY(!res))
    {
        errorDescription.setBase(QT_TR_NOOP("Failed to clear the contents of the log file"));
        QString errorString = currentLogFile.errorString();
        if (!errorString.isEmpty()) {
            errorDescription.details() = errorString;
        }
    }
    else
    {
        m_currentLogFilePos = 0;
        m_currentLogFileSize = 0;

        m_data.clear();
        m_currentParsedLogFileLines = 0;
        m_currentLogFileLines.clear();

        for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
            m_currentLogFileStartBytes[i] = 0;
        }
        m_currentLogFileStartBytesRead = 0;

        m_pendingCurrentLogFileWipe = false;
        m_wipeCurrentLogFileResultStatus = false;
        m_wipeCurrentLogFileErrorDescription.clear();
    }

    endResetModel();
    return res;
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
         << QStringLiteral(", log entry: ") << m_logEntry
         << QStringLiteral("\nNum log entry lines = ") << m_numLogEntryLines
         << QStringLiteral(", log entry max num chars per line = ") << m_logEntryMaxNumCharsPerLine;
    return strm;
}

} // namespace quentier
