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

#ifndef QUENTIER_MODELS_LOG_VIEWER_MODEL_H
#define QUENTIER_MODELS_LOG_VIEWER_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Printable.h>
#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/utility/LRUCache.hpp>
#include <quentier/types/ErrorString.h>
#include <QAbstractTableModel>
#include <QFileInfo>
#include <QList>
#include <QThread>
#include <QRegExp>
#include <QBasicTimer>
#include <QSet>

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

    QString logFileName() const;
    void setLogFileName(const QString & logFileName);

    qint64 currentLogFilePos() const { return m_currentLogFilePos; }

    bool wipeCurrentLogFile(ErrorString & errorDescription);
    void clear();

    static QString logLevelToString(LogLevel::type logLevel);

    struct Data: public Printable
    {
        Data() :
            m_timestamp(),
            m_sourceFileName(),
            m_sourceFileLineNumber(-1),
            m_logLevel(LogLevel::InfoLevel),
            m_logEntry(),
            m_numLogEntryLines(0),
            m_logEntryMaxNumCharsPerLine(0)
        {}

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        QDateTime       m_timestamp;
        QString         m_sourceFileName;
        qint64          m_sourceFileLineNumber;
        LogLevel::type  m_logLevel;
        QString         m_logEntry;

        // These two properties are useful to have in the model
        // for the delegate's sake as it's too non-performant
        // to compute these numbers within the delegate
        int             m_numLogEntryLines;
        int             m_logEntryMaxNumCharsPerLine;
    };

    const Data * dataEntry(const int row) const;

    QString dataEntryToString(const Data & dataEntry) const;

    QColor backgroundColorForLogLevel(const LogLevel::type logLevel) const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);

    // private signals
    void startAsyncLogFileReading();
    void readLogFileLines(qint64 fromPos, quint32 maxLines);
    void deleteFileReaderAsync();
    void wipeCurrentLogFileFinished();

public:
    // QAbstractTableModel interface
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool canFetchMore(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual void fetchMore(const QModelIndex & parent) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

    void onFileReadAsyncReady(qint64 logFilePos, QString readData, ErrorString errorDescription);

    void onLogFileLinesRead(qint64 fromPos, qint64 endPos, QStringList lines, ErrorString errorDescription);

private:
    void startReadingDataFromLogFile();
    void startReadingDataFromLogFile(const qint64 startPos);
    void startReadingDataFromLogFileFromCurrentPos();

    void parseNextChunkOfLogFileLines();
    bool parseNextChunkOfLogFileLines(const int lineNumFrom, QVector<Data> & readLogFileEntries,
                                      int & lastParsedLogFileLine);

private:
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void appendLogEntryLine(Data & data, const QString & line) const;
    bool wipeCurrentLogFileImpl(ErrorString & errorDescription);

private:
    class FileReaderAsync;

private:
    Q_DISABLE_COPY(LogViewerModel)

private:
    class LogFileChunkMetadata: public Printable
    {
    public:
        LogFileChunkMetadata(const int number,
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

    struct LogFileChunkByNumber{};
    struct LogFileChunkByStartModelRow{};
    struct LogFileChunkByStartLogFilePos{};

    typedef boost::multi_index_container<
        LogFileChunkMetadata,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunkByNumber>,
                boost::multi_index::const_mem_fun<LogFileChunkMetadata,int,&LogFileChunkMetadata::number>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunkByStartModelRow>,
                boost::multi_index::const_mem_fun<LogFileChunkMetadata,int,&LogFileChunkMetadata::startModelRow>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<LogFileChunkByStartLogFilePos>,
                boost::multi_index::const_mem_fun<LogFileChunkMetadata,qint64,&LogFileChunkMetadata::startLogFilePos>
            >
        >
    > LogFileChunkData;

    typedef LogFileChunkData::index<LogFileChunkByNumber>::type LogFileChunkDataByNumber;
    typedef LogFileChunkData::index<LogFileChunkByStartModelRow>::type LogFileChunkDataByStartModelRow;
    typedef LogFileChunkData::index<LogFileChunkByStartLogFilePos>::type LogFileChunkDataByStartLogFilePos;

private:
    QFileInfo           m_currentLogFileInfo;
    FileSystemWatcher   m_currentLogFileWatcher;

    QRegExp             m_logParsingRegex;

    char                m_currentLogFileStartBytes[256];
    qint64              m_currentLogFileStartBytesRead;

    LogFileChunkData    m_logFileChunkMetadata;
    LRUCache<qint32, QVector<Data> >    m_logFileChunkDataCache;

    QVector<Data>       m_lastParsedLogFileChunk;

    qint64              m_currentLogFilePos;

    QSet<qint64>        m_logFilePosRequestedToBeRead;

    int                 m_currentParsedLogFileLines;
    QStringList         m_currentLogFileLines;

    qint64              m_currentLogFileSize;
    QBasicTimer         m_currentLogFileSizePollingTimer;

    bool                m_pendingLogFileReadData;

    QThread *           m_pReadLogFileIOThread;
    FileReaderAsync *   m_pFileReaderAsync;

    QList<Data>         m_data;

    bool                m_pendingCurrentLogFileWipe;
    bool                m_wipeCurrentLogFileResultStatus;
    ErrorString         m_wipeCurrentLogFileErrorDescription;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::LogViewerModel::Data)
Q_DECLARE_METATYPE(QList<quentier::LogViewerModel::Data>)

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_H
