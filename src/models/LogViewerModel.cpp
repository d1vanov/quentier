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
    m_currentLogFile(),
    m_currentLogFileWatcher(),
    m_currentPos(-1),
    m_offsetPos(-1),
    m_data()
{}

QString LogViewerModel::logFileName() const
{
    return m_currentLogFile.fileName();
}

void LogViewerModel::setLogFileName(const QString & logFileName)
{
    QNDEBUG(QStringLiteral("LogViewerModel::setLogFileName: ") << logFileName);

    QFileInfo currentLogFileInfo(m_currentLogFile);
    QFileInfo newLogFileInfo(QuentierLogFilesDirPath() + QStringLiteral("/") + logFileName);
    if (currentLogFileInfo.absoluteFilePath() == newLogFileInfo.absoluteFilePath()) {
        QNDEBUG(QStringLiteral("This log file is already set to the model"));
        return;
    }

    beginResetModel();

    m_offsetPos = -1;
    m_currentPos = 0;
    m_currentLogFile.close();
    m_currentLogFileWatcher.removePath(currentLogFileInfo.absoluteFilePath());
    m_data.clear();

    m_currentLogFile.setFileName(newLogFileInfo.absoluteFilePath());
    if (Q_UNLIKELY(!m_currentLogFile.exists()))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Log file doesn't exist"));
        errorDescription.details() = info.absoluteFilePath();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        endResetModel();
        return;
    }

    bool open = m_currentLogFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!open))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = info.absoluteFilePath();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        endResetModel();
        return;
    }

    currentLogFileInfo.setFile(m_currentLogFile);
    m_currentLogFileWatcher.addPath(currentLogFileInfo.absoluteFilePath());

    m_currentLogFileStartBytesRead = m_currentLogFile.read(m_currentLogFileStartBytes, sizeof(m_currentLogFileStartBytes));
    if (!m_currentLogFile.seek(0))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't set the position within the current log file to zero"));
        errorDescription.details() = info.absoluteFilePath();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        endResetModel();
        return;
    }

    parseFullDataFromLogFile();
    endResetModel();
}

void LogViewerModel::setOffsetPos(const qint64 offsetPos)
{
    QNDEBUG(QStringLiteral("LogViewerModel::setOffsetPos: ") << offsetPos);

    m_offsetPos = offsetPos;

    if (!m_currentLogFile.isOpen()) {
        QNDEBUG(QStringLiteral("The current log file is not open, won't do anything"));
        return;
    }

    beginResetModel();
    m_data.clear();
    m_currentPos = std::max(offsetPos, qint64(0));
    parseDataFromLogFileFromCurrentPos();
    endResetModel();
}

void LogViewerModel::copyAllToClipboard()
{
    QNDEBUG(QStringLiteral("LogViewerModel::copyAllToClipboard"));

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't copy data to clipboard: got null pointer to clipboard from app"));
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

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
    QFileInfo currentLogFileInfo(m_currentLogFile);
    if (currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    if (!m_currentLogFile.isOpen() && !m_currentLogFile.open(QIODevice::ReadOnly))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't open log file for reading"));
        errorDescription.details() = info.absoluteFilePath();
        emit notifyError(errorDescription);
        return;
    }

    qint64 currentPos = m_currentLogFile.pos();

    if (!m_currentLogFile.seek(0))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't set the position within the current log file to zero"));
        errorDescription.details() = info.absoluteFilePath();
        emit notifyError(errorDescription);
        return;
    }

    char startBytes[sizeof(m_currentLogFileStartBytes)];
    qint64 startBytesRead = m_currentLogFile.read(startBytes, sizeof(startBytes));

    if (!m_currentLogFile.seek(currentPos))
    {
        QFileInfo info(m_currentLogFile);
        ErrorString errorDescription(QT_TR_NOOP("Can't restore the position within the current log file after reading some first bytes from it"));
        errorDescription.details() = info.absoluteFilePath();
        emit notifyError(errorDescription);
        return;
    }

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
        m_offsetPos = -1;
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
    QFileInfo currentLogFileInfo(m_currentLogFile);
    if (currentLogFileInfo.absoluteFilePath() != path) {
        // The changed file is not the current log file
        return;
    }

    beginResetModel();
    m_currentLogFile.close();
    m_currentLogFileWatcher.removePath(path);
    m_currentLogFileStartBytesRead = 0;
    for(size_t i = 0; i < sizeof(m_currentLogFileStartBytes); ++i) {
        m_currentLogFileStartBytes[i] = 0;
    }
    m_currentPos = 0;
    m_offsetPos = -1;
    m_data.clear();
    endResetModel();
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

    if (!m_currentLogFile.seek(m_currentPos)) {
        ErrorString errorDescription(QT_TR_NOOP("Failed to read the data from log file: failed to seek at position"));
        errorDescription.details() = QString::number(m_currentPos);
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    const qint64 bufSize = 1024;
    char buf[bufSize];
    QByteArray readData;

    while(true)
    {
        qint64 bytesRead = m_currentLogFile.read(buf, bufSize);
        if (bytesRead == -1)
        {
            ErrorString errorDescription(QT_TR_NOOP("Failed to read the data from log file"));
            QString logFileError = m_currentLogFile.errorString();
            if (!logFileError.isEmpty()) {
                errorDescription.details() = logFileError;
            }

            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }
        else if (bytesRead == 0)
        {
            QNDEBUG(QStringLiteral("Read all data from the log file"));
            break;
        }

        readData.append(buf, static_cast<int>(bytesRead));
        m_currentPos = m_currentLogFile.pos();
    }

    parseAndAppendData(QString::fromUtf8(readData));
}

void LogViewerModel::parseAndAppendData(const QString & logFileFragment)
{
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

QString LogViewerModel::logLevelToString(LogLevel::type logLevel) const
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
