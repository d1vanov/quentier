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
#include <quentier/utility/StandardPaths.h>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QStringRef>
#include <QTimer>
#include <QTimerEvent>
#include <QFile>
#include <QCoreApplication>
#include <QMetaType>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QTimeZone>
#endif

#include <algorithm>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (5)
#define LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET (1000)
#define LOG_VIEWER_MODEL_LOG_FILE_POLLING_TIMER_MSEC (500)
#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (700)

#define LVMDEBUG(message) \
    if (m_internalLogEnabled) \
    { \
        QString msg; \
        QDebug dbg(&msg); \
        __QNLOG_QDEBUG_HELPER(); \
        dbg << message; \
        QString relativeSourceFileName = QStringLiteral(__FILE__); \
        int prefixIndex = relativeSourceFileName.indexOf(QStringLiteral("libquentier"), Qt::CaseInsensitive); \
        if (prefixIndex >= 0) \
        { \
            relativeSourceFileName.remove(0, prefixIndex); \
        } \
        else \
        { \
            QString appName = QCoreApplication::applicationName().toLower(); \
            prefixIndex = relativeSourceFileName.indexOf(appName, Qt::CaseInsensitive); \
            if (prefixIndex >= 0) { \
                relativeSourceFileName.remove(0, prefixIndex + appName.size() + 1); \
            } \
        } \
        DateTimePrint::Options options(DateTimePrint::IncludeMilliseconds | DateTimePrint::IncludeTimezone); \
        QString fullMsg = printableDateTimeFromTimestamp(QDateTime::currentMSecsSinceEpoch(), options) + QStringLiteral(" ") + \
                          relativeSourceFileName + QStringLiteral(" @ ") + QString::number(__LINE__) + \
                          QStringLiteral(": ") + msg + QStringLiteral("\n"); \
        m_internalLogFile.write(fullMsg.toUtf8()); \
        m_internalLogFile.flush(); \
    }

namespace quentier {

LogViewerModel::LogViewerModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_isActive(false),
    m_currentLogFileInfo(),
    m_currentLogFileWatcher(),
    m_filteringOptions(),
    m_currentLogFileStartBytes(),
    m_currentLogFileStartBytesRead(0),
    m_logFileChunksMetadata(),
    m_logFileChunkDataCache(),
    m_canReadMoreLogFileChunks(false),
    m_logFilePosRequestedToBeRead(),
    m_currentLogFileSize(0),
    m_currentLogFileSizePollingTimer(),
    m_pReadLogFileIOThread(Q_NULLPTR),
    m_pFileReaderAsync(Q_NULLPTR),
    m_targetSaveFile(),
    m_internalLogEnabled(false),
    m_internalLogFile(applicationTemporaryStoragePath() + QStringLiteral("/LogViewerModelLog.txt")),
    m_pendingCurrentLogFileWipe(false),
    m_wipeCurrentLogFileResultStatus(false),
    m_wipeCurrentLogFileErrorDescription()
{
    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileChanged,QString),
                     this, QNSLOT(LogViewerModel,onFileChanged,QString));
    QObject::connect(&m_currentLogFileWatcher, QNSIGNAL(FileSystemWatcher,fileRemoved,QString),
                     this, QNSLOT(LogViewerModel,onFileRemoved,QString));

    qRegisterMetaType<QVector<LogViewerModel::Data> >("QVector<LogViewerModel::Data>");

    setInternalLogEnabled(true);
}

bool LogViewerModel::isActive() const
{
    return m_isActive;
}

LogViewerModel::~LogViewerModel()
{
    if (m_pFileReaderAsync) {
        m_pFileReaderAsync->disconnect(this);
        m_pFileReaderAsync = Q_NULLPTR;
    }
}

QString LogViewerModel::logFileName() const
{
    return m_currentLogFileInfo.fileName();
}

void LogViewerModel::setLogFileName(const QString & logFileName, const FilteringOptions & filteringOptions)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::setLogFileName: ") << logFileName
             << QStringLiteral(", filtering options = ") << filteringOptions);

    QString quentierLogFilesDirPath = QuentierLogFilesDirPath();
    QString logFilePath = logFileName;
    if (!logFilePath.startsWith(quentierLogFilesDirPath)) {
        logFilePath = quentierLogFilesDirPath + QStringLiteral("/") + logFileName;
    }

    QFileInfo newLogFileInfo(logFilePath);
    if (m_isActive && (m_currentLogFileInfo.absoluteFilePath() == newLogFileInfo.absoluteFilePath()))
    {
        LVMDEBUG(QStringLiteral("The log file hasn't changed"));

        if (m_filteringOptions == filteringOptions) {
            return;
        }
    }

    clear();
    m_currentLogFileInfo = newLogFileInfo;
    m_filteringOptions = filteringOptions;

    QFile currentLogFile(m_currentLogFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!currentLogFile.exists())) {
        ErrorString errorDescription(QT_TR_NOOP("Log file doesn't exist"));
        errorDescription.details() = m_currentLogFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    bool open = currentLogFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = m_currentLogFileInfo.absoluteFilePath();
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    m_isActive = true;

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

    qint64 startPos = (m_filteringOptions.m_startLogFilePos.isSet()
                       ? m_filteringOptions.m_startLogFilePos.ref()
                       : qint64(0));
    requestDataEntriesChunkFromLogFile(startPos, LogFileDataEntryRequestReason::InitialRead);
}

qint64 LogViewerModel::startLogFilePos() const
{
    return m_filteringOptions.m_startLogFilePos.isSet()
           ? m_filteringOptions.m_startLogFilePos.ref()
           : qint64(-1);
}

void LogViewerModel::setStartLogFilePos(const qint64 startLogFilePos)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::setStartLogFilePos: ") << startLogFilePos);

    if (!m_filteringOptions.m_startLogFilePos.isSet() && (startLogFilePos < 0)) {
        return;
    }

    if (m_filteringOptions.m_startLogFilePos.isSet() && (startLogFilePos >= 0) &&
        (m_filteringOptions.m_startLogFilePos.ref() == startLogFilePos))
    {
        return;
    }

    if (!m_isActive)
    {
        if (startLogFilePos >= 0) {
            m_filteringOptions.m_startLogFilePos = startLogFilePos;
        }
        else {
            m_filteringOptions.m_startLogFilePos.clear();
        }

        return;
    }

    FilteringOptions filteringOptions = m_filteringOptions;
    if (startLogFilePos >= 0) {
        filteringOptions.m_startLogFilePos = startLogFilePos;
    }
    else {
        filteringOptions.m_startLogFilePos.clear();
    }

    QString logFileName = m_currentLogFileInfo.fileName();
    clear();
    setLogFileName(logFileName, filteringOptions);
}

qint64 LogViewerModel::currentLogFileSize() const
{
    return m_currentLogFileInfo.size();
}

void LogViewerModel::setStartLogFilePosAfterCurrentFileSize()
{
    setStartLogFilePos(m_currentLogFileInfo.size());
}

const QVector<LogLevel::type> & LogViewerModel::disabledLogLevels() const
{
    return m_filteringOptions.m_disabledLogLevels;
}

void LogViewerModel::setDisabledLogLevels(QVector<LogLevel::type> disabledLogLevels)
{
    disabledLogLevels.erase(std::unique(disabledLogLevels.begin(), disabledLogLevels.end()), disabledLogLevels.end());

    if (m_filteringOptions.m_disabledLogLevels.size() == disabledLogLevels.size())
    {
        bool foundMismatch = false;
        for(auto it = m_filteringOptions.m_disabledLogLevels.constBegin(),
            end = m_filteringOptions.m_disabledLogLevels.constEnd(); it != end; ++it)
        {
            if (!disabledLogLevels.contains(*it)) {
                foundMismatch = true;
                break;
            }
        }

        if (!foundMismatch) {
            return;
        }
    }

    if (!m_isActive) {
        m_filteringOptions.m_disabledLogLevels = disabledLogLevels;
        return;
    }

    FilteringOptions filteringOptions = m_filteringOptions;
    filteringOptions.m_disabledLogLevels = disabledLogLevels;

    QString logFileName = m_currentLogFileInfo.fileName();
    clear();
    setLogFileName(logFileName, filteringOptions);
}

const QString & LogViewerModel::logEntryContentFilter() const
{
    return m_filteringOptions.m_logEntryContentFilter;
}

void LogViewerModel::setLogEntryContentFilter(const QString & logEntryContentFilter)
{
    if (m_filteringOptions.m_logEntryContentFilter == logEntryContentFilter) {
        return;
    }

    if (!m_isActive) {
        m_filteringOptions.m_logEntryContentFilter = logEntryContentFilter;
        return;
    }

    FilteringOptions filteringOptions = m_filteringOptions;
    filteringOptions.m_logEntryContentFilter = logEntryContentFilter;

    QString logFileName = m_currentLogFileInfo.fileName();
    clear();
    setLogFileName(logFileName, filteringOptions);
}

bool LogViewerModel::wipeCurrentLogFile(ErrorString & errorDescription)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::wipeCurrentLogFile"));

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
    LVMDEBUG(QStringLiteral("LogViewerModel::clear"));

    beginResetModel();

    m_isActive = false;
    m_currentLogFileInfo = QFileInfo();
    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());

    m_filteringOptions.clear();

    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentLogFileStartBytesRead = 0;

    m_logFileChunksMetadata.clear();
    m_logFileChunkDataCache.clear();

    m_canReadMoreLogFileChunks = false;

    m_logFilePosRequestedToBeRead.clear();

    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    // NOTE: not stopping the file reader async's thread and not deleting the async file reader immediately,
    // just disconnect from it, mark it for subsequent deletion when possible and lose the pointer to it
    if (m_pFileReaderAsync) {
        m_pFileReaderAsync->disconnect(this);
        m_pFileReaderAsync = Q_NULLPTR;
    }

    // NOTE: not changing anything about the internal log

    m_pendingCurrentLogFileWipe = false;
    m_wipeCurrentLogFileResultStatus = false;
    m_wipeCurrentLogFileErrorDescription.clear();

    endResetModel();
}

const LogViewerModel::Data * LogViewerModel::dataEntry(const int row) const
{
    int startModelRow = 0;
    const QVector<Data> * pLogFileDataChunk = dataChunkContainingModelRow(row, &startModelRow);
    if (!pLogFileDataChunk) {
        return Q_NULLPTR;
    }

    int offset = row - startModelRow;
    if (Q_UNLIKELY(pLogFileDataChunk->size() <= offset)) {
        return Q_NULLPTR;
    }

    const Data & data = pLogFileDataChunk->at(offset);
    return &data;
}

const QVector<LogViewerModel::Data> * LogViewerModel::dataChunkContainingModelRow(const int row, int * pStartModelRow) const
{
    if (Q_UNLIKELY(row < 0)) {
        return Q_NULLPTR;
    }

    const LogFileChunkMetadata * pLogFileChunkMetadata = findLogFileChunkMetadataByModelRow(row);
    if (!pLogFileChunkMetadata) {
        return Q_NULLPTR;
    }

    if (pStartModelRow) {
        *pStartModelRow = pLogFileChunkMetadata->startModelRow();
    }

    return m_logFileChunkDataCache.get(pLogFileChunkMetadata->number());
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
         << dataEntry.m_logEntry;

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

void LogViewerModel::saveModelEntriesToFile(const QString & targetFilePath)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::saveModelEntriesToFile: ") << targetFilePath);

    m_targetSaveFile.setFileName(targetFilePath);
    if (!m_targetSaveFile.open(QIODevice::WriteOnly)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't save log entries to file: could not open the selected file for writing"));
        LVMDEBUG(errorDescription);
        Q_EMIT saveModelEntriesToFileFinished(errorDescription);
        return;
    }

    const LogFileChunksMetadataIndexByNumber & indexByNumber = m_logFileChunksMetadata.get<LogFileChunksMetadataByNumber>();
    if (indexByNumber.empty()) {
        LVMDEBUG(QStringLiteral("The index of log file chunks metadata by number is empty, requesting the chunks starting from the first"));
        requestDataEntriesChunkFromLogFile(0, LogFileDataEntryRequestReason::SaveLogEntriesToFile);
        return;
    }

    qint64 currentLogFileSize = m_currentLogFileInfo.size();
    LVMDEBUG(QStringLiteral("The number of log file metadata chunks = ") << indexByNumber.size()
             << QStringLiteral(", current log file size = ") << currentLogFileSize);

    for(auto it = indexByNumber.begin(), end = indexByNumber.end(); it != end; ++it)
    {
        const QVector<Data> * pDataEntries = m_logFileChunkDataCache.get(it->number());
        if (!pDataEntries) {
            LVMDEBUG(QStringLiteral("Log data entries are not cached for log file metadata chunk with number ")
                     << it->number() << QStringLiteral(", requesting log file data chunks starting from pos ")
                     << it->startLogFilePos());
            requestDataEntriesChunkFromLogFile(it->startLogFilePos(), LogFileDataEntryRequestReason::SaveLogEntriesToFile);
            return;
        }

        LVMDEBUG(QStringLiteral("Processing ") << pDataEntries->size() << QStringLiteral(" data entries"));
        for(auto dit = pDataEntries->constBegin(), dend = pDataEntries->constEnd(); dit != dend; ++dit) {
            QString entry = dataEntryToString(*dit);
            entry += QStringLiteral("\n");
            m_targetSaveFile.write(entry.toUtf8());
        }

        if (currentLogFileSize > 0) {
            double progressPercent = static_cast<double>(it->endLogFilePos()) / static_cast<double>(currentLogFileSize) * 100.0;
            LVMDEBUG(QStringLiteral("Emitting save log to file progress: ") << progressPercent);
            Q_EMIT saveModelEntriesToFileProgress(progressPercent);
        }
    }

    if (canFetchMore(QModelIndex())) {
        auto lastIt = indexByNumber.end();
        --lastIt;
        LVMDEBUG(QStringLiteral("The model can fetch more entries, requesting data entries starting from pos ")
                 << lastIt->endLogFilePos());
        requestDataEntriesChunkFromLogFile(lastIt->endLogFilePos(), LogFileDataEntryRequestReason::SaveLogEntriesToFile);
        return;
    }

    // If we got here, we found all data entries within the cache, can close the file and return
    LVMDEBUG(QStringLiteral("Seemingly all log entries were written to the file, reporting finish"));
    m_targetSaveFile.close();
    Q_EMIT saveModelEntriesToFileFinished(ErrorString());
}

bool LogViewerModel::isSavingModelEntriesToFileInProgress() const
{
    for(auto it = m_logFilePosRequestedToBeRead.constBegin(),
        end = m_logFilePosRequestedToBeRead.constEnd(); it != end; ++it)
    {
        if (it.value() & LogFileDataEntryRequestReason::SaveLogEntriesToFile) {
            return true;
        }
    }

    return false;
}

void LogViewerModel::cancelSavingModelEntriesToFile()
{
    for(auto it = m_logFilePosRequestedToBeRead.begin();
        it != m_logFilePosRequestedToBeRead.end(); )
    {
        if (it.value() == LogFileDataEntryRequestReasons(LogFileDataEntryRequestReason::SaveLogEntriesToFile)) {
            it = m_logFilePosRequestedToBeRead.erase(it);
        }
        else if (it.value() & LogFileDataEntryRequestReason::SaveLogEntriesToFile) {
            it.value() &= ~LogFileDataEntryRequestReason::SaveLogEntriesToFile;
            ++it;
        }
    }

    m_targetSaveFile.close();
}

int LogViewerModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    const LogFileChunksMetadataIndexByNumber & indexByNumber = m_logFileChunksMetadata.get<LogFileChunksMetadataByNumber>();
    if (indexByNumber.empty()) {
        return 0;
    }

    auto lastIt = indexByNumber.end();
    --lastIt;
    return lastIt->endModelRow();
}

int LogViewerModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return LOG_VIEWER_MODEL_COLUMN_COUNT;
}

QVariant LogViewerModel::data(const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole) {
        LVMDEBUG(QStringLiteral("Not display role, returning empty QVariant"));
        return QVariant();
    }

    LVMDEBUG(QStringLiteral("LogViewerModel::data: index.isValid() = ")
             << (index.isValid() ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral(", index.row() = ") << index.row()
             << QStringLiteral(", index.column() = ") << index.column());

    if (!index.isValid()) {
        LVMDEBUG(QStringLiteral("Not valid, returning empty QVariant"));
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= LOG_VIEWER_MODEL_COLUMN_COUNT)) {
        LVMDEBUG(QStringLiteral("Column outside of valid range, returning empty QVariant"));
        return QVariant();
    }

    int rowIndex = index.row();
    if (rowIndex < 0) {
        LVMDEBUG(QStringLiteral("Row is negative, returning empty QVariant"));
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

    LVMDEBUG(QStringLiteral("LogViewerModel::data: data entry was not found for row ") << rowIndex
             << QStringLiteral(" and column ") << columnIndex << QStringLiteral(", returning empty QVariant"));
    const LogFileChunkMetadata * pLogFileChunkMetadata = findLogFileChunkMetadataByModelRow(rowIndex);
    if (pLogFileChunkMetadata) {
        const_cast<LogViewerModel*>(this)->requestDataEntriesChunkFromLogFile(pLogFileChunkMetadata->startLogFilePos(),
                                                                              LogFileDataEntryRequestReason::CacheMiss);
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

    LVMDEBUG(QStringLiteral("LogViewerModel::canFetchMore: ") << (m_canReadMoreLogFileChunks ? QStringLiteral("true") : QStringLiteral("false")));
    return m_canReadMoreLogFileChunks;
}

void LogViewerModel::fetchMore(const QModelIndex & parent)
{
    if (parent.isValid()) {
        return;
    }

    LVMDEBUG(QStringLiteral("LogViewerModel::fetchMore"));

    qint64 startPos = 0;
    const LogFileChunksMetadataIndexByStartLogFilePos & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
    if (!index.empty())
    {
        auto lastIt = index.end();
        --lastIt;

        startPos = lastIt->endLogFilePos();
    }

    requestDataEntriesChunkFromLogFile(startPos, LogFileDataEntryRequestReason::FetchMore);
}

void LogViewerModel::onFileChanged(const QString & path)
{
    if (m_currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    LVMDEBUG(QStringLiteral("LogViewerModel::onFileChanged"));

    m_currentLogFileInfo.refresh();

    QFile currentLogFile(path);
    if (!currentLogFile.isOpen() && !currentLogFile.open(QIODevice::ReadOnly)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = path;
        LVMDEBUG(errorDescription);
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

        LVMDEBUG(QStringLiteral("Initial several bytes of the log file have changed, the log file was probably rotated"));

        m_currentLogFileStartBytesRead = startBytesRead;

        for(qint64 i = 0, size = sizeof(startBytes); i < size; ++i) {
            size_t index = static_cast<size_t>(i);
            m_currentLogFileStartBytes[index] = startBytes[index];
        }

        beginResetModel();

        m_logFileChunksMetadata.clear();
        m_logFileChunkDataCache.clear();
        m_logFilePosRequestedToBeRead.clear();

        m_canReadMoreLogFileChunks = false;

        requestDataEntriesChunkFromLogFile(0, LogFileDataEntryRequestReason::InitialRead);

        m_currentLogFileSize = 0;

        endResetModel();

        return;
    }

    // New log entries were appended to the log file, should now be able to fetch more
    LVMDEBUG(QStringLiteral("The initial bytes of the log file haven't changed => new log entries were added, can read more now"));
    m_canReadMoreLogFileChunks = true;

    if (m_logFileChunksMetadata.empty()) {
        qint64 startPos = (m_filteringOptions.m_startLogFilePos.isSet()
                           ? m_filteringOptions.m_startLogFilePos.ref()
                           : qint64(0));
        requestDataEntriesChunkFromLogFile(startPos, LogFileDataEntryRequestReason::InitialRead);
    }
}

void LogViewerModel::onFileRemoved(const QString & path)
{
    if (m_currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    LVMDEBUG(QStringLiteral("LogViewerModel::onFileRemoved"));

    beginResetModel();
    m_currentLogFileInfo = QFileInfo();
    m_currentLogFileWatcher.removePath(path);
    m_currentLogFileStartBytesRead = 0;
    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }

    m_logFileChunksMetadata.clear();
    m_logFileChunkDataCache.clear();
    m_logFilePosRequestedToBeRead.clear();

    m_currentLogFileSize = 0;
    m_currentLogFileSizePollingTimer.stop();

    m_canReadMoreLogFileChunks = false;

    endResetModel();
}

void LogViewerModel::onLogFileDataEntriesRead(qint64 fromPos, qint64 endPos,
                                              QVector<LogViewerModel::Data> dataEntries,
                                              ErrorString errorDescription)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::onLogFileDataEntriesRead: from pos = ") << fromPos
             << QStringLiteral(", end pos = ") << endPos
             << QStringLiteral(", num parsed data entries = ") << dataEntries.size()
             << QStringLiteral(", error description = ") << errorDescription);

    auto fromPosIt = m_logFilePosRequestedToBeRead.find(fromPos);
    if (fromPosIt == m_logFilePosRequestedToBeRead.end()) {
        return;
    }

    LogFileDataEntryRequestReasons reasons = fromPosIt.value();
    Q_UNUSED(m_logFilePosRequestedToBeRead.erase(fromPosIt))

    if (!errorDescription.isEmpty())
    {
        ErrorString error(QT_TR_NOOP("Failed to read a portion of log from file: "));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();

        if (reasons.testFlag(LogFileDataEntryRequestReason::SaveLogEntriesToFile)) {
            m_targetSaveFile.close();
            Q_EMIT saveModelEntriesToFileFinished(error);
        }
        else {
            Q_EMIT notifyError(error);
        }

        return;
    }

    if (reasons.testFlag(LogFileDataEntryRequestReason::SaveLogEntriesToFile))
    {
        QChar newline(QChar::fromLatin1('\n'));
        for(auto it = dataEntries.constBegin(), end = dataEntries.constEnd(); it != end; ++it)
        {
            QString entry = dataEntryToString(*it);
            if (!entry.endsWith(newline)) {
                entry += newline;
            }

            m_targetSaveFile.write(entry.toUtf8());
        }

        if (dataEntries.size() < LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET)
        {
            // We must have reached the end of the log file
            m_targetSaveFile.close();
            Q_EMIT saveModelEntriesToFileFinished(ErrorString());
        }
        else
        {
            qint64 currentLogFileSize = m_currentLogFileInfo.size();
            if (currentLogFileSize > 0) {
                double progressPercent = static_cast<double>(endPos) / static_cast<double>(currentLogFileSize) * 100.0;
                Q_EMIT saveModelEntriesToFileProgress(progressPercent);
            }

            requestDataEntriesChunkFromLogFile(endPos, LogFileDataEntryRequestReason::SaveLogEntriesToFile);
        }
    }

    if (reasons == LogFileDataEntryRequestReasons(LogFileDataEntryRequestReason::SaveLogEntriesToFile)) {
        // If there were no other reasons for this request, don't really need to do anything here
        return;
    }

    int logFileChunkNumber = 0;
    int startModelRow = 0;
    int endModelRow = 0;

    if (!dataEntries.isEmpty())
    {
        LogFileChunksMetadataIndexByStartLogFilePos & indexByStartPos = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
        auto it = findLogFileChunkMetadataIteratorByLogFilePos(fromPos);
        bool newEntry = (it == indexByStartPos.end());
        if (newEntry)
        {
            const LogFileChunksMetadataIndexByNumber & indexByNumber = m_logFileChunksMetadata.get<LogFileChunksMetadataByNumber>();
            if (!indexByNumber.empty()) {
                auto lastIndexIt = indexByNumber.end();
                --lastIndexIt;
                logFileChunkNumber = lastIndexIt->number() + 1;
                startModelRow = lastIndexIt->endModelRow() + 1;
            }
            else {
                startModelRow = rowCount();
            }

            endModelRow = startModelRow + dataEntries.size() - 1;

            LVMDEBUG(QStringLiteral("Inserting new rows into the model: start row = ")
                     << startModelRow << QStringLiteral(", end row = ") << endModelRow);
            beginInsertRows(QModelIndex(), startModelRow, endModelRow);

            LogFileChunkMetadata metadata(logFileChunkNumber, startModelRow, endModelRow, fromPos, endPos);
            LVMDEBUG(QStringLiteral("Appending new log file chunk metadata: ") << metadata);
            auto insertResult = m_logFileChunksMetadata.insert(metadata);
            if (!insertResult.second) {
                LVMDEBUG(QStringLiteral("The log file chunk metadata was actually updated, not inserted! Here it is: ") << *insertResult.first);
            }
        }
        else
        {
            LogFileChunkMetadata metadata = *it;

            logFileChunkNumber = metadata.number();
            startModelRow = metadata.startModelRow();
            endModelRow = startModelRow + dataEntries.size() - 1;

            metadata = LogFileChunkMetadata(logFileChunkNumber, startModelRow, endModelRow, fromPos, endPos);
            Q_UNUSED(indexByStartPos.replace(it, metadata))
                LVMDEBUG(QStringLiteral("Updated log file chunk metadata: ") << metadata);
        }

        m_logFileChunkDataCache.put(logFileChunkNumber, dataEntries);
        LVMDEBUG(QStringLiteral("Put parsed log file data chunk to the LRUCache, chunk number = ") << logFileChunkNumber);

        if (newEntry) {
            LVMDEBUG(QStringLiteral("End insert rows"));
            endInsertRows();
        }
        else {
            LVMDEBUG(QStringLiteral("Notifying the view of the model change: start row = ") << startModelRow
                     << QStringLiteral(", end row = ") << endModelRow);
            QModelIndex startIndex = index(startModelRow, Columns::Timestamp, QModelIndex());
            QModelIndex endIndex = index(endModelRow, Columns::LogEntry, QModelIndex());
            Q_EMIT dataChanged(startIndex, endIndex);
        }

        LVMDEBUG(QStringLiteral("Emitting model rows cached signal: start model row = ")
                 << startModelRow << QStringLiteral(", end model row = ") << endModelRow);
        Q_EMIT notifyModelRowsCached(startModelRow, endModelRow);
    }

    if (dataEntries.size() < LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET) {
        // It appears we've read to the end of the log file
        LVMDEBUG(QStringLiteral("It appears the end of the log file was reached"));
        m_canReadMoreLogFileChunks = false;
    }
    else {
        LVMDEBUG(QStringLiteral("Received precisely as many data entries as requested, probably more of them can be read"));
        m_canReadMoreLogFileChunks = true;
    }
}

void LogViewerModel::requestDataEntriesChunkFromLogFile(const qint64 startPos, const LogFileDataEntryRequestReason::type reason)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::requestDataEntriesChunkFromLogFile: start pos = ") << startPos
             << QStringLiteral(", request reason = ") << reason);

    auto it = m_logFilePosRequestedToBeRead.find(startPos);
    if (it != m_logFilePosRequestedToBeRead.end()) {
        LVMDEBUG(QStringLiteral("Have already requested log file data entries chunk from this start pos, not doing anything"));
        it.value() |= reason;
        return;
    }

    if (!m_pReadLogFileIOThread)
    {
        m_pReadLogFileIOThread = new QThread;

        QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                         this, QNSLOT(QThread,deleteLater));
        QObject::connect(this, QNSIGNAL(LogViewerModel,destroyed),
                         m_pReadLogFileIOThread, QNSLOT(QThread,quit));
        m_pReadLogFileIOThread->start(QThread::LowPriority);
    }

    if (!m_pFileReaderAsync)
    {
        m_pFileReaderAsync = new FileReaderAsync(m_currentLogFileInfo.absoluteFilePath(),
                                                 m_filteringOptions.m_disabledLogLevels,
                                                 m_filteringOptions.m_logEntryContentFilter);
        m_pFileReaderAsync->moveToThread(m_pReadLogFileIOThread);

        QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,deleteLater));
        QObject::connect(this, QNSIGNAL(LogViewerModel,readLogFileDataEntries,qint64,int),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,onReadDataEntriesFromLogFile,qint64,int),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(m_pFileReaderAsync, QNSIGNAL(FileReaderAsync,readLogFileDataEntries,qint64,qint64,QVector<LogViewerModel::Data>,ErrorString),
                         this, QNSLOT(LogViewerModel,onLogFileDataEntriesRead,qint64,qint64,QVector<LogViewerModel::Data>,ErrorString),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        QObject::connect(this, QNSIGNAL(LogViewerModel,deleteFileReaderAsync),
                         m_pFileReaderAsync, QNSLOT(FileReaderAsync,deleteLater));
    }

    m_logFilePosRequestedToBeRead[startPos] |= reason;
    Q_EMIT readLogFileDataEntries(startPos, LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET);
    LVMDEBUG(QStringLiteral("Emitted the request to read no more than ") << LOG_VIEWER_MODEL_NUM_ITEMS_PER_CACHE_BUCKET
             << QStringLiteral(" log file data entries starting at pos ") << startPos);
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

bool LogViewerModel::wipeCurrentLogFileImpl(ErrorString & errorDescription)
{
    LVMDEBUG(QStringLiteral("LogViewerModel::wipeCurrentLogFileImpl"));

    if (!m_currentLogFileInfo.exists()) {
        errorDescription.setBase(QT_TR_NOOP("No log file is selected"));
        LVMDEBUG(errorDescription);
        return false;
    }

    if (Q_UNLIKELY(!m_currentLogFileInfo.isFile())) {
        errorDescription.setBase(QT_TR_NOOP("Currently selected log file name does not really correspond to a file"));
        LVMDEBUG(errorDescription);
        return false;
    }

    if (Q_UNLIKELY(!m_currentLogFileInfo.isWritable())) {
        errorDescription.setBase(QT_TR_NOOP("Currently selected log file is not writable"));
        LVMDEBUG(errorDescription);
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
        LVMDEBUG(errorDescription);
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

void LogViewerModel::setInternalLogEnabled(const bool enabled)
{
    if (m_internalLogEnabled == enabled) {
        return;
    }

    m_internalLogEnabled = enabled;

    if (m_internalLogEnabled) {
        Q_UNUSED(m_internalLogFile.open(QIODevice::WriteOnly))
    }
    else {
        m_internalLogFile.close();
    }
}

bool LogViewerModel::internalLogEnabled() const
{
    return m_internalLogEnabled;
}

const LogViewerModel::LogFileChunkMetadata * LogViewerModel::findLogFileChunkMetadataByModelRow(const int row) const
{
    const LogFileChunksMetadataIndexByStartModelRow & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartModelRow>();
    auto it = findLogFileChunkMetadataIteratorByModelRow(row);
    if (it == index.end()) {
        return Q_NULLPTR;
    }

    return &(*it);
}

const LogViewerModel::LogFileChunkMetadata * LogViewerModel::findLogFileChunkMetadataByLogFilePos(const qint64 pos) const
{
    const LogFileChunksMetadataIndexByStartLogFilePos & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
    auto it = findLogFileChunkMetadataIteratorByLogFilePos(pos);
    if (it == index.end()) {
        return Q_NULLPTR;
    }

    return &(*it);
}

LogViewerModel::LogFileChunksMetadataIndexByStartModelRow::const_iterator
LogViewerModel::findLogFileChunkMetadataIteratorByModelRow(const int row) const
{
    const LogFileChunksMetadataIndexByStartModelRow & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartModelRow>();
    auto it = std::lower_bound(index.begin(), index.end(), row, LowerBoundByStartModelRowComparator());
    if (it == index.end())
    {
        // Unless the index is empty, need to check the previous iterator
        if (index.empty()) {
            return index.end();
        }

        auto lastIt = index.end();
        --lastIt;
        if ((lastIt->startModelRow() < row) && (lastIt->endModelRow() >= row)) {
            return lastIt;
        }

        LVMDEBUG(QStringLiteral("Appropriate log file chunk metadata was not found for model row ") << row);
        return index.end();
    }

    if (it->startModelRow() == row) {
        return it;
    }

    if (it == index.begin()) {
        return index.end();
    }

    // By definition of std::lower_bound, it should point to the first metadata entry
    // which has startModelRow greater than or equal to row; if we got here, it is certainly not equal to row
    // but strictly greater. Hence, previous iterator's startModelRow should be less than row,
    // hence previous iterator should point to the metadata entry we are looking for.
    auto prevIt = it;
    --prevIt;
    return prevIt;
}

LogViewerModel::LogFileChunksMetadataIndexByStartLogFilePos::const_iterator
LogViewerModel::findLogFileChunkMetadataIteratorByLogFilePos(const qint64 pos) const
{
    const LogFileChunksMetadataIndexByStartLogFilePos & index = m_logFileChunksMetadata.get<LogFileChunksMetadataByStartLogFilePos>();
    auto it = std::lower_bound(index.begin(), index.end(), pos, LowerBoundByStartLogFilePosComparator());
    if (it == index.end())
    {
        // Unless the index is empty, need to check the previous iterator
        if (index.empty()) {
            return index.end();
        }

        auto lastIt = index.end();
        --lastIt;
        if ((lastIt->startLogFilePos() < pos) && (lastIt->endLogFilePos() > pos)) {
            return lastIt;
        }

        LVMDEBUG(QStringLiteral("Log file chunk metadata was not found for log file pos ") << pos);
        return index.end();
    }

    if (it->startLogFilePos() == pos) {
        return it;
    }

    if (it == index.begin()) {
        return index.end();
    }

    // By definition of std::lower_bound, it should point to the first metadata entry
    // which has startLogFilePos greater than or equal to pos; if we got here, it is certainly not equal to pos
    // but strictly greater. Hence, previous iterator's startLogFilePos should be less than pos,
    // hence previous iterator should point to the metadata entry we are looking for.
    auto prevIt = it;
    --prevIt;
    return prevIt;
}

LogViewerModel::FilteringOptions::FilteringOptions() :
    m_startLogFilePos(),
    m_disabledLogLevels(),
    m_logEntryContentFilter()
{}

LogViewerModel::FilteringOptions::FilteringOptions(const FilteringOptions & filteringOptions) :
    m_startLogFilePos(filteringOptions.m_startLogFilePos),
    m_disabledLogLevels(filteringOptions.m_disabledLogLevels),
    m_logEntryContentFilter(filteringOptions.m_logEntryContentFilter)
{}

LogViewerModel::FilteringOptions & LogViewerModel::FilteringOptions::operator=(const FilteringOptions & filteringOptions)
{
    if (this != &filteringOptions) {
        m_startLogFilePos = filteringOptions.m_startLogFilePos;
        m_disabledLogLevels = filteringOptions.m_disabledLogLevels;
        m_logEntryContentFilter = filteringOptions.m_logEntryContentFilter;
    }

    return *this;
}

LogViewerModel::FilteringOptions::~FilteringOptions()
{}

bool LogViewerModel::FilteringOptions::operator==(const FilteringOptions & filteringOptions) const
{
    if (!m_startLogFilePos.isEqual(filteringOptions.m_startLogFilePos)) {
        return false;
    }

    if (m_disabledLogLevels.size() != filteringOptions.m_disabledLogLevels.size()) {
        return false;
    }

    for(auto it = m_disabledLogLevels.constBegin(), end = m_disabledLogLevels.constEnd(); it != end; ++it)
    {
        if (!filteringOptions.m_disabledLogLevels.contains(*it)) {
            return false;
        }
    }

    if (m_logEntryContentFilter != filteringOptions.m_logEntryContentFilter) {
        return false;
    }

    return true;
}

bool LogViewerModel::FilteringOptions::operator!=(const FilteringOptions & filteringOptions) const
{
    return !operator==(filteringOptions);
}

bool LogViewerModel::FilteringOptions::isEmpty() const
{
    if (m_startLogFilePos.isSet()) {
        return false;
    }

    if (!m_disabledLogLevels.isEmpty()) {
        return false;
    }

    return m_logEntryContentFilter.isEmpty();
}

void LogViewerModel::FilteringOptions::clear()
{
    m_startLogFilePos.clear();
    m_disabledLogLevels.clear();
    m_logEntryContentFilter.clear();
}

QTextStream & LogViewerModel::FilteringOptions::print(QTextStream & strm) const
{
    strm << QStringLiteral("FilteringOptions: start log file pos = ");

    if (m_startLogFilePos.isSet()) {
        strm << m_startLogFilePos.ref();
    }
    else {
        strm << QStringLiteral("<not set>");
    }

    strm << QStringLiteral(", disabled log levels: ");
    if (!m_disabledLogLevels.isEmpty())
    {
        for(int i = 0, size = m_disabledLogLevels.size(); i < size; ++i)
        {
            switch(m_disabledLogLevels[i])
            {
            case LogLevel::TraceLevel:
                strm << QStringLiteral("Trace");
                break;
            case LogLevel::DebugLevel:
                strm << QStringLiteral("Debug");
                break;
            case LogLevel::InfoLevel:
                strm << QStringLiteral("Info");
                break;
            case LogLevel::WarnLevel:
                strm << QStringLiteral("Warning");
                break;
            case LogLevel::ErrorLevel:
                strm << QStringLiteral("Error");
                break;
            default:
                strm << QStringLiteral("Unknown (") << m_disabledLogLevels[i] << QStringLiteral(")");
                break;
            }

            if (i != (size - 1)) {
                strm << QStringLiteral(", ");
            }
        }
    }
    else
    {
        strm << QStringLiteral("<not set>");
    }

    strm << QStringLiteral(", log entry content filtering: ") << m_logEntryContentFilter;
    return strm;
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
