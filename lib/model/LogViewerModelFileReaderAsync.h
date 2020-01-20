/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H
#define QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H

#include "LogViewerModel.h"
#include "LogViewerModelLogFileParser.h"

#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QVector>

namespace quentier {

class LogViewerModel::FileReaderAsync : public QObject
{
    Q_OBJECT
public:
    explicit FileReaderAsync(
        const QString & targetFilePath,
        const QVector<LogLevel> & disabledLogLevels,
        const QString & logEntryContentFilter,
        QObject * parent = nullptr);

    virtual ~FileReaderAsync();

Q_SIGNALS:
    void readLogFileDataEntries(
        qint64 fromPos, qint64 endPos,
        QVector<LogViewerModel::Data> dataEntries,
        ErrorString errorDescription);

public Q_SLOTS:
    void onReadDataEntriesFromLogFile(qint64 fromPos, int maxDataEntries);

private:
    Q_DISABLE_COPY(FileReaderAsync)

private:
    QFile                           m_targetFile;
    QVector<LogLevel>               m_disabledLogLevels;
    QRegExp                         m_filterRegExp;
    LogViewerModel::LogFileParser   m_parser;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H
