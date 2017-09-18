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
#include <algorithm>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (5)

namespace quentier {

LogViewerModel::LogViewerModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_logParsingRegex(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}.\\d{3}\\s+\\w+)\\s+(.+)\\s+@\\s+(\\d+)\\s+\\[(\\w+)\\]:\\s(.+$)"),
                      Qt::CaseInsensitive, QRegExp::RegExp),
    m_currentLogFileInfo(),
    m_currentLogFileWatcher(),
    m_currentPos(-1),
    m_pendingLogFileReadData(false),
    m_pReadLogFileIOThread(new QThread),
    m_data()
{
    QObject::connect(m_pReadLogFileIOThread, QNSIGNAL(QThread,finished),
                     this, QNSLOT(QThread,deleteLater));
    m_pReadLogFileIOThread->start(QThread::LowPriority);
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

    m_currentPos = 0;
    m_currentLogFileInfo = newLogFileInfo;
    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_data.clear();

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

    m_currentPos = 0;
    m_currentLogFileWatcher.removePath(m_currentLogFileInfo.absoluteFilePath());
    m_currentLogFileInfo = QFileInfo();
    m_data.clear();

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
        m_currentPos = 0;
        m_data.clear();
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
    m_currentPos = 0;
    m_data.clear();
    endResetModel();
}

void LogViewerModel::onFileReadAsyncReady(QByteArray readData, qint64 pos, ErrorString errorDescription)
{
    if (!errorDescription.isEmpty()) {
        emit notifyError(errorDescription);
        return;
    }

    m_currentPos = pos;
    parseAndAppendData(QString::fromUtf8(readData));
}

void LogViewerModel::parseFullDataFromLogFile()
{
    QNDEBUG(QStringLiteral("LogViewerModel::parseFullDataFromLogFile"));


    m_currentPos = 0;
    parseDataFromLogFileFromCurrentPos();
}

void LogViewerModel::parseDataFromLogFileFromCurrentPos()
{
    QNDEBUG(QStringLiteral("LogViewerModel::parseDataFromLogFileFromCurrentPos"));

    FileReaderAsync * pFileReaderAsync = new FileReaderAsync(m_currentLogFileInfo.absoluteFilePath(),
                                                             m_currentPos, this);
    pFileReaderAsync->moveToThread(m_pReadLogFileIOThread);
    QObject::connect(this, QNSIGNAL(LogViewerModel,startAsyncLogFileReading),
                     pFileReaderAsync, QNSLOT(FileReaderAsync,onStartReading));
    QObject::connect(pFileReaderAsync, QNSIGNAL(FileReaderAsync,finished,QByteArray,qint64,ErrorString),
                     this, QNSLOT(LogViewerModel,onFileReadAsyncReady,QByteArray,qint64,ErrorString));

    m_pendingLogFileReadData = true;
    emit startAsyncLogFileReading();
}

void LogViewerModel::parseAndAppendData(const QString & logFileFragment)
{
    QNDEBUG(QStringLiteral("LogViewerModel::parseAndAppendData"));

    if (Q_UNLIKELY(!m_logParsingRegex.isValid())) {
        ErrorString errorDescription(QT_TR_NOOP("Can't parse the log file: invalid regexp"));
        errorDescription.details() = m_logParsingRegex.pattern();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    int numFoundMatches = 0;
    QStringList lines = logFileFragment.split(QChar::fromLatin1('\n'), QString::SkipEmptyParts);
    for(int i = 0, numLines = lines.size(); i < numLines; ++i)
    {
        const QString & line = lines.at(i);
        int currentIndex = m_logParsingRegex.indexIn(line);
        if (currentIndex < 0)
        {
            if (m_data.empty() || (numFoundMatches == 0)) {
                continue;
            }

            Data & lastDataEntry = m_data.back();
            lastDataEntry.m_logEntry += line;

            QModelIndex changedDataIndex = index(static_cast<int>(m_data.size()) - 1, Columns::LogEntry);
            emit dataChanged(changedDataIndex, changedDataIndex);
            continue;
        }

        QStringList capturedTexts = m_logParsingRegex.capturedTexts();

        if (capturedTexts.size() != 6) {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: unexpected number of captures by regex"));
            errorDescription.details() += QString::number(capturedTexts.size());
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        bool convertedSourceLineNumberToInt = false;
        int sourceFileLineNumber = capturedTexts[3].toInt(&convertedSourceLineNumberToInt);
        if (!convertedSourceLineNumberToInt) {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to convert the source line number to int"));
            errorDescription.details() += capturedTexts[3];
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        Data dataEntry;
        dataEntry.m_timestamp = QDateTime::fromString(capturedTexts[1],
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                                      QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t")
#else
                                                      QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz t")
#endif
                                                     );
        dataEntry.m_sourceFileName = capturedTexts[2];
        dataEntry.m_sourceFileLineNumber = sourceFileLineNumber;

        const QString & logLevel = capturedTexts[4];
        if (logLevel == QStringLiteral("Trace")) {
            dataEntry.m_logLevel = LogLevel::TraceLevel;
        }
        else if (logLevel == QStringLiteral("Debug")) {
            dataEntry.m_logLevel = LogLevel::DebugLevel;
        }
        else if (logLevel == QStringLiteral("Info")) {
            dataEntry.m_logLevel = LogLevel::InfoLevel;
        }
        else if (logLevel == QStringLiteral("Warn")) {
            dataEntry.m_logLevel = LogLevel::WarnLevel;
        }
        else if (logLevel == QStringLiteral("Error")) {
            dataEntry.m_logLevel = LogLevel::ErrorLevel;
        }
        else if (logLevel == QStringLiteral("Fatal")) {
            dataEntry.m_logLevel = LogLevel::FatalLevel;
        }
        else
        {
            ErrorString errorDescription(QT_TR_NOOP("Error parsing the log file's contents: failed to parse the log level"));
            errorDescription.details() += logLevel;
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        dataEntry.m_logEntry = capturedTexts[5];
        dataEntry.m_logEntry += QStringLiteral("\n");

        beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
        m_data.append(dataEntry);
        endInsertRows();

        ++numFoundMatches;
    }
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
