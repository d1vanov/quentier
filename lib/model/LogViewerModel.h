/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_H
#define QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Printable.h>
#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/ErrorString.h>

#include <QAbstractTableModel>
#include <QFileInfo>
#include <QFile>
#include <QList>
#include <QThread>
#include <QRegExp>
#include <QBasicTimer>
#include <QVector>
#include <QHash>
#include <QFlags>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#endif

namespace quentier {

class LogViewerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    LogViewerModel(QObject * parent = Q_NULLPTR);
    ~LogViewerModel();

    struct Columns
    {
        enum type {
            Timestamp = 0,
            SourceFileName,
            SourceFileLineNumber,
            LogLevel,
            LogEntry
        };
    };

    bool isActive() const;

    class FilteringOptions: public Printable
    {
    public:
        FilteringOptions();
        FilteringOptions(const FilteringOptions & other);
        FilteringOptions & operator=(const FilteringOptions & other);
        virtual ~FilteringOptions();

        bool operator==(const FilteringOptions & other) const;
        bool operator!=(const FilteringOptions & other) const;

        bool isEmpty() const;
        void clear();

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        qevercloud::Optional<qint64>    m_startLogFilePos;
        QVector<LogLevel::type>         m_disabledLogLevels;
        QString                         m_logEntryContentFilter;
    };

    QString logFileName() const;
    void setLogFileName(const QString & logFileName,
                        const FilteringOptions & filteringOptions =
                        FilteringOptions());

    qint64 startLogFilePos() const;
    void setStartLogFilePos(const qint64 startLogFilePos);

    qint64 currentLogFileSize() const;
    void setStartLogFilePosAfterCurrentFileSize();

    const QVector<LogLevel::type> & disabledLogLevels() const;
    void setDisabledLogLevels(QVector<LogLevel::type> disabledLogLevels);

    const QString & logEntryContentFilter() const;
    void setLogEntryContentFilter(const QString & logEntryContentFilter);

    bool wipeCurrentLogFile(ErrorString & errorDescription);
    void clear();

    static QString logLevelToString(LogLevel::type logLevel);

    void setInternalLogEnabled(const bool enabled);
    bool internalLogEnabled() const;

    struct Data: public Printable
    {
        Data() :
            m_timestamp(),
            m_sourceFileName(),
            m_sourceFileLineNumber(-1),
            m_logLevel(LogLevel::InfoLevel),
            m_logEntry()
        {}

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        QDateTime       m_timestamp;
        QString         m_sourceFileName;
        qint64          m_sourceFileLineNumber;
        LogLevel::type  m_logLevel;
        QString         m_logEntry;
    };

    const Data * dataEntry(const int row) const;

    const QVector<Data> * dataChunkContainingModelRow(
        const int row, int * pStartModelRow = Q_NULLPTR) const;

    QString dataEntryToString(const Data & dataEntry) const;

    QColor backgroundColorForLogLevel(const LogLevel::type logLevel) const;

    void saveModelEntriesToFile(const QString & targetFilePath);
    bool isSavingModelEntriesToFileInProgress() const;
    void cancelSavingModelEntriesToFile();

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

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
    virtual int rowCount(
        const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;

    virtual int columnCount(
        const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;

    virtual QVariant data(
        const QModelIndex & index,
        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual bool canFetchMore(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual void fetchMore(const QModelIndex & parent) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

    void onLogFileDataEntriesRead(
        qint64 fromPos, qint64 endPos,
        QVector<LogViewerModel::Data> dataEntries,
        ErrorString errorDescription);

private:
    struct LogFileDataEntryRequestReason
    {
        enum type
        {
            InitialRead = 1 << 1,
            CacheMiss = 1 << 2,
            FetchMore = 1 << 3,
            SaveLogEntriesToFile = 1 << 4
        };
    };
    Q_DECLARE_FLAGS(LogFileDataEntryRequestReasons,
                    LogFileDataEntryRequestReason::type)

    void requestDataEntriesChunkFromLogFile(
        const qint64 startPos, const LogFileDataEntryRequestReason::type reason);

private:
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;

private:
    class FileReaderAsync;
    class LogFileParser;

private:
    Q_DISABLE_COPY(LogViewerModel)

private:
    class LogFileChunkMetadata: public Printable
    {
    public:
        LogFileChunkMetadata(
                const int number,
                const int startModelRow,
                const int endModelRow,
                const qint64 startLogFilePos,
                const qint64 endLogFilePos) :
            m_number(number),
            m_startModelRow(startModelRow),
            m_endModelRow(endModelRow),
            m_startLogFilePos(startLogFilePos),
            m_endLogFilePos(endLogFilePos)
        {}

        bool isEmpty() const { return (m_number < 0); }
        int number() const { return m_number; }
        int startModelRow() const { return m_startModelRow; }
        int endModelRow() const { return m_endModelRow; }
        qint64 startLogFilePos() const { return m_startLogFilePos; }
        qint64 endLogFilePos() const { return m_endLogFilePos; }

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    private:
        int     m_number;

        int     m_startModelRow;
        int     m_endModelRow;

        qint64  m_startLogFilePos;
        qint64  m_endLogFilePos;
    };

    struct LowerBoundByStartModelRowComparator
    {
        bool operator()(const LogFileChunkMetadata & lhs, const int row) const
        {
            return lhs.startModelRow() < row;
        }
    };

    struct LowerBoundByStartLogFilePosComparator
    {
        bool operator()(const LogFileChunkMetadata & lhs, const qint64 pos) const
        {
            return lhs.startLogFilePos() < pos;
        }
    };

    struct LogFileChunksMetadataByNumber{};
    struct LogFileChunksMetadataByStartModelRow{};
    struct LogFileChunksMetadataByStartLogFilePos{};

    typedef boost::multi_index_container<
        LogFileChunkMetadata,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByNumber>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata,int,&LogFileChunkMetadata::number>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByStartModelRow>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata,int,&LogFileChunkMetadata::startModelRow>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunksMetadataByStartLogFilePos>,
                boost::multi_index::const_mem_fun<
                    LogFileChunkMetadata,qint64,&LogFileChunkMetadata::startLogFilePos>
            >
        >
    > LogFileChunksMetadata;

    typedef LogFileChunksMetadata::index<LogFileChunksMetadataByNumber>::type
        LogFileChunksMetadataIndexByNumber;
    typedef LogFileChunksMetadata::index<LogFileChunksMetadataByStartModelRow>::type
        LogFileChunksMetadataIndexByStartModelRow;
    typedef LogFileChunksMetadata::index<LogFileChunksMetadataByStartLogFilePos>::type
        LogFileChunksMetadataIndexByStartLogFilePos;

private:
    const LogFileChunkMetadata *
    findLogFileChunkMetadataByModelRow(const int row) const;

    const LogFileChunkMetadata *
    findLogFileChunkMetadataByLogFilePos(const qint64 pos) const;

    LogFileChunksMetadataIndexByStartModelRow::const_iterator
    findLogFileChunkMetadataIteratorByModelRow(const int row) const;

    LogFileChunksMetadataIndexByStartLogFilePos::const_iterator
    findLogFileChunkMetadataIteratorByLogFilePos(const qint64 pos) const;

private:
    bool                m_isActive;
    QFileInfo           m_currentLogFileInfo;
    FileSystemWatcher   m_currentLogFileWatcher;

    FilteringOptions    m_filteringOptions;

    char                m_currentLogFileStartBytes[256];
    qint64              m_currentLogFileStartBytesRead;

    LogFileChunksMetadata               m_logFileChunksMetadata;
    LRUCache<qint32, QVector<Data> >    m_logFileChunkDataCache;

    bool                m_canReadMoreLogFileChunks;

    QHash<qint64, LogFileDataEntryRequestReasons>   m_logFilePosRequestedToBeRead;

    qint64              m_currentLogFileSize;
    QBasicTimer         m_currentLogFileSizePollingTimer;

    QThread *           m_pReadLogFileIOThread;
    FileReaderAsync *   m_pFileReaderAsync;

    QFile               m_targetSaveFile;

    bool                m_internalLogEnabled;
    mutable QFile       m_internalLogFile;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::LogViewerModel::Data)
Q_DECLARE_METATYPE(QList<quentier::LogViewerModel::Data>)

#endif // QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_H
