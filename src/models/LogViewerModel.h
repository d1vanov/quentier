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
#include <quentier/types/ErrorString.h>
#include <QAbstractTableModel>
#include <QFileInfo>
#include <QVector>
#include <QRegExp>
#include <QThread>

namespace quentier {

class LogViewerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    LogViewerModel(QObject * parent = Q_NULLPTR);

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

    qint64 currentPos() const { return m_currentPos; }

    void clear();

    QString copyAllToString() const;
    void copyAllToClipboard() const;

    static QString logLevelToString(LogLevel::type logLevel);

Q_SIGNALS:
    void notifyError(ErrorString errorDescription) const;

    // private signals
    void startAsyncLogFileReading();

public:
    // QAbstractTableModel interface
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

    void onFileReadAsyncReady(QByteArray readData, qint64 pos,
                              ErrorString errorDescription);

private:
    void parseFullDataFromLogFile();
    void parseDataFromLogFileFromCurrentPos();
    void parseAndAppendData(const QString & logFileFragment);

private:
    class FileReaderAsync;

private:
    Q_DISABLE_COPY(LogViewerModel)

private:
    QRegExp             m_logParsingRegex;

    QFileInfo           m_currentLogFileInfo;
    FileSystemWatcher   m_currentLogFileWatcher;

    char                m_currentLogFileStartBytes[256];
    qint64              m_currentLogFileStartBytesRead;

    qint64              m_currentPos;

    bool                m_pendingLogFileReadData;

    QThread *           m_pReadLogFileIOThread;

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
        int             m_sourceFileLineNumber;
        LogLevel::type  m_logLevel;
        QString         m_logEntry;
    };

    QVector<Data>       m_data;
};

} // namespace quentier

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_H
