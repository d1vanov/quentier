/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/utility/Printable.h>
#include <quentier/utility/SuppressWarnings.h>

#include <QAbstractTableModel>
#include <QBasicTimer>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QFlags>
#include <QHash>
#include <QList>
#include <QThread>

#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <array>
#include <optional>

namespace quentier {

class LogViewerModel final : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LogViewerModel(QObject * parent = nullptr);

    ~LogViewerModel() override;

    Q_DISABLE_COPY(LogViewerModel)

    enum class Column
    {
        Timestamp = 0,
        SourceFileName,
        SourceFileLineNumber,
        Component,
        LogLevel,
        LogEntry
    };

    [[nodiscard]] bool isActive() const noexcept;

    class FilteringOptions : public Printable
    {
    public:
        [[nodiscard]] bool operator==(
            const FilteringOptions & other) const noexcept;

        [[nodiscard]] bool operator!=(
            const FilteringOptions & other) const noexcept;

        [[nodiscard]] bool isEmpty() const noexcept;
        void clear();

        QTextStream & print(QTextStream & strm) const override;

        std::optional<qint64> m_startLogFilePos;
        QList<LogLevel> m_disabledLogLevels;
        QString m_logEntryContentFilter;
    };

    [[nodiscard]] QString logFileName() const;

    void setLogFileName(
        const QString & logFileName,
        const FilteringOptions & filteringOptions = {});

    [[nodiscard]] qint64 startLogFilePos() const noexcept;
    void setStartLogFilePos(const qint64 startLogFilePos);

    [[nodiscard]] qint64 currentLogFileSize() const;
    void setStartLogFilePosAfterCurrentFileSize();

    [[nodiscard]] const QList<LogLevel> & disabledLogLevels() const noexcept;
    void setDisabledLogLevels(QList<LogLevel> disabledLogLevels);

    [[nodiscard]] const QString & logEntryContentFilter() const noexcept;
    void setLogEntryContentFilter(const QString & logEntryContentFilter);

    [[nodiscard]] bool wipeCurrentLogFile(ErrorString & errorDescription);
    void clear();

    [[nodiscard]] static QString logLevelToString(LogLevel logLevel);

    void setInternalLogEnabled(const bool enabled);
    [[nodiscard]] bool internalLogEnabled() const noexcept;

    struct Data : public Printable
    {
        QTextStream & print(QTextStream & strm) const override;

        QDateTime m_timestamp;
        QString m_sourceFileName;
        qint64 m_sourceFileLineNumber = -1;
        QString m_component;
        LogLevel m_logLevel = LogLevel::Info;
        QString m_logEntry;
    };

    [[nodiscard]] const Data * dataEntry(const int row) const;

    [[nodiscard]] const QList<Data> * dataChunkContainingModelRow(
        const int row, int * startModelRow = nullptr) const;

    [[nodiscard]] QString dataEntryToString(const Data & dataEntry) const;

    [[nodiscard]] QColor backgroundColorForLogLevel(
        const LogLevel logLevel) const;

    void saveModelEntriesToFile(const QString & targetFilePath);
    [[nodiscard]] bool isSavingModelEntriesToFileInProgress() const;

    void cancelSavingModelEntriesToFile();

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    void notifyEndOfLogFileReached();

    /**
     * This signal is emitted after either beginInsertRows/endInsertRows or
     * dataChanged signal to notify specific listeners (not just views) about
     * the fact that the data corresponding to the specific rows of the model
     * has been cached and hence is present and actual.
     */
    void notifyModelRowsCached(int from, int to);

    /**
     * This signal is emitted in response to the earlier invokation of of
     * saveModelEntriesToFile method. errorDescription is empty if no error
     * occurred in the process, non-empty otherwise.
     */
    void saveModelEntriesToFileFinished(ErrorString errorDescription);

    /**
     * This signal is emitted to notify anyone interested about the progress of
     * saving the model's log entries to a file.
     *
     * @param progressPercent       The percentage of progress, from 0 to 100
     */
    void saveModelEntriesToFileProgress(double progressPercent);

    // private signals
    void startAsyncLogFileReading();
    void readLogFileDataEntries(qint64 fromPos, int maxDataEntries);
    void deleteFileReaderAsync();
    void wipeCurrentLogFileFinished();

public:
    // QAbstractTableModel interface
    [[nodiscard]] int rowCount(const QModelIndex & parent = {}) const override;
    [[nodiscard]] int columnCount(
        const QModelIndex & parent = {}) const override;

    [[nodiscard]] QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    [[nodiscard]] bool canFetchMore(const QModelIndex & parent) const override;
    void fetchMore(const QModelIndex & parent) override;

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

    void onLogFileDataEntriesRead(
        qint64 fromPos, qint64 endPos,
        QList<LogViewerModel::Data> dataEntries,
        ErrorString errorDescription);

private:
    enum class LogFileDataEntryRequestReason
    {
        InitialRead = 1 << 1,
        CacheMiss = 1 << 2,
        FetchMore = 1 << 3,
        SaveLogEntriesToFile = 1 << 4
    };

    friend QDebug & operator<<(
        QDebug & dbg, LogFileDataEntryRequestReason reason);

    Q_DECLARE_FLAGS(
        LogFileDataEntryRequestReasons, LogFileDataEntryRequestReason)

    friend QDebug & operator<<(
        QDebug & dbg, LogFileDataEntryRequestReasons reasons);

    void requestDataEntriesChunkFromLogFile(
        const qint64 startPos,
        const LogFileDataEntryRequestReason reason);

private:
    void timerEvent(QTimerEvent * event) override;

private:
    class FileReaderAsync;
    class LogFileParser;

private:
    class LogFileChunkMetadata : public Printable
    {
    public:
        LogFileChunkMetadata(
            const int number, const int startModelRow, const int endModelRow,
            const qint64 startLogFilePos, const qint64 endLogFilePos) :
            m_number{number},
            m_startModelRow{startModelRow}, m_endModelRow{endModelRow},
            m_startLogFilePos{startLogFilePos}, m_endLogFilePos{endLogFilePos}
        {}

        [[nodiscard]] bool isEmpty() const noexcept
        {
            return (m_number < 0);
        }

        [[nodiscard]] int number() const noexcept
        {
            return m_number;
        }

        [[nodiscard]] int startModelRow() const noexcept
        {
            return m_startModelRow;
        }

        [[nodiscard]] int endModelRow() const noexcept
        {
            return m_endModelRow;
        }

        [[nodiscard]] qint64 startLogFilePos() const noexcept
        {
            return m_startLogFilePos;
        }

        [[nodiscard]] qint64 endLogFilePos() const noexcept
        {
            return m_endLogFilePos;
        }

        QTextStream & print(QTextStream & strm) const override;

    private:
        int m_number;

        int m_startModelRow;
        int m_endModelRow;

        qint64 m_startLogFilePos;
        qint64 m_endLogFilePos;
    };

    struct LowerBoundByStartModelRowComparator
    {
        [[nodiscard]] bool operator()(
            const LogFileChunkMetadata & lhs, const int row) const noexcept
        {
            return lhs.startModelRow() < row;
        }
    };

    struct LowerBoundByStartLogFilePosComparator
    {
        [[nodiscard]] bool operator()(
            const LogFileChunkMetadata & lhs, const qint64 pos) const noexcept
        {
            return lhs.startLogFilePos() < pos;
        }
    };

    struct LogFileChunksMetadataByNumber
    {};

    struct LogFileChunksMetadataByStartModelRow
    {};

    struct LogFileChunksMetadataByStartLogFilePos
    {};

    using LogFileChunksMetadata = boost::multi_index_container<
        LogFileChunkMetadata,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByNumber>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata, int, &LogFileChunkMetadata::number>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByStartModelRow>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata, int,
                    &LogFileChunkMetadata::startModelRow>>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByStartLogFilePos>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata, qint64,
                    &LogFileChunkMetadata::startLogFilePos>>>>;

    using LogFileChunksMetadataIndexByNumber =
        LogFileChunksMetadata::index<LogFileChunksMetadataByNumber>::type;

    using LogFileChunksMetadataIndexByStartModelRow =
        LogFileChunksMetadata::index<
            LogFileChunksMetadataByStartModelRow>::type;

    using LogFileChunksMetadataIndexByStartLogFilePos =
        LogFileChunksMetadata::index<
            LogFileChunksMetadataByStartLogFilePos>::type;

private:
    [[nodiscard]] const LogFileChunkMetadata *
    findLogFileChunkMetadataByModelRow(const int row) const;

    [[nodiscard]] const LogFileChunkMetadata *
    findLogFileChunkMetadataByLogFilePos(const qint64 pos) const;

    [[nodiscard]] LogFileChunksMetadataIndexByStartModelRow::const_iterator
    findLogFileChunkMetadataIteratorByModelRow(const int row) const;

    [[nodiscard]] LogFileChunksMetadataIndexByStartLogFilePos::const_iterator
    findLogFileChunkMetadataIteratorByLogFilePos(const qint64 pos) const;

private:
    bool m_isActive = false;
    QFileInfo m_currentLogFileInfo;
    FileSystemWatcher m_currentLogFileWatcher;

    FilteringOptions m_filteringOptions;

    std::array<char, 256> m_currentLogFileStartBytes;
    qint64 m_currentLogFileStartBytesRead = 0;

    LogFileChunksMetadata m_logFileChunksMetadata;
    LRUCache<qint32, QList<Data>> m_logFileChunkDataCache;

    bool m_canReadMoreLogFileChunks = false;

    QHash<qint64, LogFileDataEntryRequestReasons> m_logFilePosRequestedToBeRead;

    qint64 m_currentLogFileSize = 0;
    QBasicTimer m_currentLogFileSizePollingTimer;

    QThread * m_readLogFileIOThread = nullptr;
    FileReaderAsync * m_fileReaderAsync = nullptr;

    QFile m_targetSaveFile;

    bool m_internalLogEnabled = false;
    mutable QFile m_internalLogFile;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::LogViewerModel::Data)
Q_DECLARE_METATYPE(QList<quentier::LogViewerModel::Data>)
