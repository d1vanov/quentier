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
#include <QTimerEvent>
#include <QFile>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#include <algorithm>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (5)
#define LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE (100)
#define LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET (100)
#define LOG_VIEWER_MODEL_LOG_FILE_POLLING_TIMER_MSEC (500)
#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

namespace quentier {

LogViewerModel::LogViewerModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_currentLogFileInfo(),
    m_currentLogFileWatcher(),
    m_logParsingRegex(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{3})\\s+(\\w+)\\s+(.+)\\s+@\\s+(\\d+)\\s+\\[(\\w+)\\]:\\s(.+$)"),
                      Qt::CaseInsensitive, QRegExp::RegExp),
    m_currentLogFileStartBytes(),
    m_currentLogFileStartBytesRead(0),
    m_logFileChunksMetadata(),
    m_logFileChunkDataCache(),
    m_lastParsedLogFileChunk(),
    m_canReadMoreLogFileChunks(false),
    m_logFilePosRequestedToBeRead(),
    m_currentLogFileSize(0),
    m_currentLogFileSizePollingTimer(),
    m_pReadLogFileIOThread(new QThread),
    m_pFileReaderAsync(Q_NULLPTR),
    m_pendingCurrentLogFileWipe(false),
    m_wipeCurrentLogFileResultStatus(false),
    m_wipeCurrentLogFileErrorDescription()
{
    QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                     this, QNSLOT(QThread,deleteLater));
    QObject::connect(this, QNSIGNAL(LogViewerModel,destroyed),
                     m_pReadLogFileIOThread, QNSLOT(QThread,quit));
    m_pReadLogFileIOThread->start(QThread::LowPriority);

    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileChanged,QString),
                     this, QNSLOT(LogViewerModel,onFileChanged,QString));
    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileRemoved,QString),
                     this, QNSLOT(LogViewerModel,onFileRemoved,QString));
}

LogViewerModel::~LogViewerModel()
{}

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

    m_logFileChunksMetadata.clear();
    m_logFileChunkDataCache.clear();
    m_lastParsedLogFileChunk.clear();
    m_logFilePosRequestedToBeRead.clear();

    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    QString currentLogFilePath = m_currentLogFileInfo.absoluteFilePath();
    if (!currentLogFilePath.isEmpty()) {
        m_currentLogFileWatcher.removePath(currentLogFilePath);
    }

    m_currentLogFileInfo = newLogFileInfo;

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

    requestDataChunkFromLogFile(0);
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

    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileInfo = QFileInfo();

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
    if (Q_UNLIKELY((row < 0) || (row >= static_cast<int>(m_logFileChunksMetadata.size())))) {
        return Q_NULLPTR;
    }

    const LogFileChunkMetadata * pLogFileChunkMetadata = findLogFileChunkMetadataByModelRow(row);
    if (!pLogFileChunkMetadata) {
        return Q_NULLPTR;
    }

    const QVector<Data> * pLogFileDataChunk = m_logFileChunkDataCache.get(pLogFileChunkMetadata->number());
    if (!pLogFileDataChunk) {
        return Q_NULLPTR;
    }

    int offset = row - pLogFileChunkMetadata->startModelRow();
    if (Q_UNLIKELY(pLogFileDataChunk->size() <= offset)) {
        return Q_NULLPTR;
    }

    const Data & data = pLogFileDataChunk->at(offset);
    return &data;
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
        return QColor(255, 38, 10, alpha);     // Red
    default:
        return QColor();
    }
}

int LogViewerModel::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return static_cast<int>(m_logFileChunksMetadata.size());
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
    if ((rowIndex < 0) || (rowIndex >= static_cast<int>(m_logFileChunksMetadata.size()))) {
        return QVariant();
    }

    const Data * pDataEntry = dataEntry(rowIndex);
    if (pDataEntry)
    {
        switch(columnIndex)
        {
        case Columns::Timestamp:
            return pDataEntry->m_timestamp;
        case Columns::SourceFileName:
            return pDataEntry->m_sourceFileName;
        case Columns::SourceFileLineNumber:
            return pDataEntry->m_sourceFileLineNumber;
        case Columns::LogLevel:
            return pDataEntry->m_logLevel;
        case Columns::LogEntry:
            return pDataEntry->m_logEntry;
        default:
            return QVariant();
        }
    }

    const LogFileChunkMetadata * pLogFileChunkMetadata = findLogFileChunkMetadataByModelRow(rowIndex);
    if (pLogFileChunkMetadata) {
        const_cast<LogViewerModel*>(this)->requestDataChunkFromLogFile(pLogFileChunkMetadata->startLogFilePos());
    }

    return QVariant();
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

    return m_canReadMoreLogFileChunks;
}

void LogViewerModel::fetchMore(const QModelIndex & parent)
{
    if (parent.isValid()) {
        return;
    }

    qint64 startPos = 0;
    const LogFileChunksMetadataIndexByStartLogFilePos & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
    if (!index.empty())
    {
        auto lastIt = index.end();
        --lastIt;

        startPos = lastIt->endLogFilePos();
    }

    requestDataChunkFromLogFile(startPos);
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

        m_logFileChunksMetadata.clear();
        m_logFileChunkDataCache.clear();
        m_lastParsedLogFileChunk.clear();
        m_logFilePosRequestedToBeRead.clear();

        requestDataChunkFromLogFile(0);

        m_currentLogFileSize = 0;

        endResetModel();

        return;
    }

    // New log entries were appended to the log file, should now be able to fetch more
    m_canReadMoreLogFileChunks = true;
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

    m_logFileChunksMetadata.clear();
    m_logFileChunkDataCache.clear();
    m_lastParsedLogFileChunk.clear();
    m_logFilePosRequestedToBeRead.clear();

    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();
    endResetModel();
}

void LogViewerModel::onLogFileLinesRead(qint64 fromPos, qint64 endPos, QStringList lines, ErrorString errorDescription)
{
    if (!errorDescription.isEmpty()) {
        ErrorString error(QT_TR_NOOP("Failed to read a portion of log from file: "));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        Q_EMIT notifyError(error);
        return;
    }

    int lastParsedLogFileLine = 0;
    bool res = parseLogFileDataChunk(0, lines, m_lastParsedLogFileChunk, lastParsedLogFileLine);
    if (!res) {
        return;
    }

    if (m_lastParsedLogFileChunk.size() >= LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET)
    {
        QVector<Data> data;
        data.reserve(LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET);
        for(auto it = m_lastParsedLogFileChunk.constBegin(), end = m_lastParsedLogFileChunk.constEnd(); it != end; ++it) {
            data << *it;
        }

        int logFileChunkNumber = 0;
        int startModelRow = 0;
        int endModelRow = 0;

        LogFileChunksMetadataIndexByStartLogFilePos & indexByStartPos = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
        auto it = indexByStartPos.find(fromPos);
        bool newEntry = (it == indexByStartPos.end());
        if (newEntry)
        {
            const LogFileChunksMetadataIndexByNumber & indexByNumber = m_logFileChunksMetadata.get<LogFileChunksMetadataByNumber>();
            if (!indexByNumber.empty()) {
                auto lastIndexIt = indexByNumber.end();
                --lastIndexIt;
                logFileChunkNumber = lastIndexIt->number();
            }

            startModelRow = rowCount();
            endModelRow = startModelRow + LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET - 1;

            LogFileChunkMetadata metadata(logFileChunkNumber, startModelRow, endModelRow, fromPos, endPos);
            Q_UNUSED(indexByStartPos.insert(metadata))
        }
        else
        {
            LogFileChunkMetadata metadata = *it;

            logFileChunkNumber = metadata.number();
            startModelRow = metadata.startModelRow();
            endModelRow = startModelRow + LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET - 1;

            metadata = LogFileChunkMetadata(logFileChunkNumber, startModelRow, endModelRow, fromPos, endPos);
            Q_UNUSED(indexByStartPos.replace(it, metadata))
        }

        if (newEntry) {
            beginInsertRows(QModelIndex(), startModelRow, endModelRow);
        }

        m_logFileChunkDataCache.put(logFileChunkNumber, data);

        if (newEntry) {
            endInsertRows();
        }
        else {
            QModelIndex startIndex = index(startModelRow, Columns::Timestamp, QModelIndex());
            QModelIndex endIndex = index(endModelRow, Columns::LogEntry, QModelIndex());
            Q_EMIT dataChanged(startIndex, endIndex);
        }

        m_lastParsedLogFileChunk.erase(m_lastParsedLogFileChunk.begin(),
                                       m_lastParsedLogFileChunk.begin() + LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET);
    }

    if (lines.size() < LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE) {
        // It appears we've read to the end of the log file
        m_canReadMoreLogFileChunks = false;
        return;
    }

    if (m_logFileChunkDataCache.size() < m_logFileChunkDataCache.max_size()) {
        // Still have some capacity within the cache of log file chunks, the lines
        // following the just received & parsed ones are likely to be requested soon
        // so reading them up front
        Q_EMIT readLogFileLines(endPos, LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE);
    }
}

void LogViewerModel::requestDataChunkFromLogFile(const qint64 startPos)
{
    auto it = m_logFilePosRequestedToBeRead.find(startPos);
    if (it != m_logFilePosRequestedToBeRead.end()) {
        return;
    }

    if (Q_UNLIKELY(!m_pFileReaderAsync))
    {
        m_pFileReaderAsync = new FileReaderAsync(m_currentLogFileInfo.absoluteFilePath());
        m_pFileReaderAsync->moveToThread(m_pReadLogFileIOThread);

        QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,deleteLater));
        QObject::connect(this, QNSIGNAL(LogViewerModel,readLogFileLines,qint64,quint32),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,onReadLogFileLines,qint64,quint32),
                         Qt::QueuedConnection);
        QObject::connect(m_pFileReaderAsync, QNSIGNAL(FileReaderAsync,readLogFileLines,qint64,qint64,QStringList,ErrorString),
                         this, QNSLOT(LogViewerModel,onLogFileLinesRead,qint64,qint64,QStringList,ErrorString),
                         Qt::QueuedConnection);
        QObject::connect(this, QNSIGNAL(LogViewerModel,deleteFileReaderAsync),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,deleteLater));
    }

    Q_UNUSED(m_logFilePosRequestedToBeRead.insert(startPos))
    Q_EMIT readLogFileLines(startPos, LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE);
}

bool LogViewerModel::parseLogFileDataChunk(const int lineNumFrom, const QStringList & logFileDataLinesToParse,
                                           QVector<Data> & parsedLogFileDataEntries, int & lastParsedLogFileLine)
{
    int estimatedLastLine = lineNumFrom + LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE;
    parsedLogFileDataEntries.reserve(std::max(parsedLogFileDataEntries.size(), 0) + LOG_VIEWER_MODEL_PARSED_LINES_BUCKET_SIZE / 10);  // Just a rough guess

    int numFoundMatches = 0;
    lastParsedLogFileLine = 0;
    for(int i = lineNumFrom, numLines = logFileDataLinesToParse.size(); i < numLines; ++i)
    {
        const QString & line = logFileDataLinesToParse.at(i);
        int currentIndex = m_logParsingRegex.indexIn(line);
        if (currentIndex < 0)
        {
            lastParsedLogFileLine = i;

            if ((numFoundMatches == 0) || parsedLogFileDataEntries.empty()) {
                continue;
            }

            LogViewerModel::Data & lastEntry = parsedLogFileDataEntries.back();
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
        else
        {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to parse the log level"));
            errorDescription.details() += logLevel;
            QNWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
            return false;
        }

        appendLogEntryLine(entry, capturedTexts[6]);
        parsedLogFileDataEntries.push_back(entry);
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
        m_currentLogFileSize = 0;

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
    default:
        return QStringLiteral("Unknown");
    }
}

const LogViewerModel::LogFileChunkMetadata * LogViewerModel::findLogFileChunkMetadataByModelRow(const int row) const
{
    const LogFileChunksMetadataIndexByStartModelRow & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartModelRow>();
    auto it = std::lower_bound(index.begin(), index.end(), row, LowerBoundByStartModelRowComparator());
    if (it == index.end()) {
        return Q_NULLPTR;
    }

    if (it->startModelRow() == row) {
        return &(*it);
    }

    if (it == index.begin()) {
        return Q_NULLPTR;
    }

    // By definition of std::lower_bound, it should point to the first metadata entry
    // which has startModelRow greater than or equal to row; if we got here, it is certainly not equal to row
    // but strictly greater. Hence, previous iterator's startModelRow should be less than row,
    // hence previous iterator should point to the metadata entry we are looking for.
    auto prevIt = it;
    --prevIt;

    return &(*prevIt);
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

QTextStream & LogViewerModel::LogFileChunkMetadata::print(QTextStream & strm) const
{
    strm << QStringLiteral("Log file chunk: number = ") << m_number
         << QStringLiteral(", start model row = ") << m_startModelRow
         << QStringLiteral(", end model row = ") << m_endModelRow
         << QStringLiteral(", start log file pos = ") << m_startLogFilePos
         << QStringLiteral(", end log file pos = ") << m_endLogFilePos;
    return strm;
}

} // namespace quentier
