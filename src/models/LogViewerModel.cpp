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
#include <QFileInfo>

#define LOG_VIEWER_MODEL_COLUMN_COUNT (4)

namespace quentier {

LogViewerModel::LogViewerModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_currentLogFile(),
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

    beginResetModel();

    m_offsetPos = -1;
    m_currentPos = 0;
    m_currentLogFile.close();

    // TODO: stop watching for current log file's changes

    m_currentLogFile.setFileName(QuentierLogFilesDirPath() + QStringLiteral("/") + logFileName);
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

    // TODO: start watching for current log file's changes

    parseFullDataFromLogFile();
    endResetModel();
}

void LogViewerModel::setOffsetPos(const qint64 offsetPos)
{
    QNDEBUG(QStringLiteral("LogViewerModel::setOffsetPos: ") << offsetPos);
    // TODO: implement
}

void LogViewerModel::copyAllToClipboard()
{
    QNDEBUG(QStringLiteral("LogViewerModel::copyAllToClipboard"));
    // TODO: implement
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

    // Convert the read data into UTF8 string
    QString readDataString = QString::fromUtf8(readData);
    readData.clear();   // free the no longer used memory

    // TODO: scan through the read data string, recognize the particular log entries and append them to m_data
    Q_UNUSED(readDataString)
}

QTextStream & LogViewerModel::Data::print(QTextStream & strm) const
{
    strm << QStringLiteral("Source file name = ") << m_sourceFileName
         << QStringLiteral(", line number = ") << m_sourceFileLineNumber
         << QStringLiteral(", log level = ") << m_logLevel
         << QStringLiteral(", log entry: ") << m_logEntry;
    return strm;
}

} // namespace quentier
