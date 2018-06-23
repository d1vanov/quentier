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

#ifndef QUENTIER_MODELS_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H
#define QUENTIER_MODELS_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H

#include "LogViewerModel.h"
#include <QFile>
#include <QStringList>
#include <QVector>
#include <QRegExp>

namespace quentier {

class LogViewerModel::FileReaderAsync : public QObject
{
    Q_OBJECT
public:
    explicit FileReaderAsync(QString targetFilePath, QObject * parent = Q_NULLPTR);
    ~FileReaderAsync();

Q_SIGNALS:
    void readLogFileLines(qint64 fromPos, qint64 endPos, QStringList lines, ErrorString errorDescription);
    void readLogFileDataEntries(qint64 fromPos, qint64 endPos,
                                QVector<LogViewerModel::Data> dataEntries,
                                ErrorString errorDescription);

public Q_SLOTS:
    void onReadLogFileLines(qint64 fromPos, quint32 maxLines);
    void onReadDataEntriesFromLogFile(qint64 fromPos, int maxDataEntries);

private:
    bool readDataEntriesFromLogFile(const qint64 fromPos, const int maxDataEntries,
                                    QVector<LogViewerModel::Data> & dataEntries, qint64 & endPos,
                                    ErrorString & errorDescription);
    void appendLogEntryLine(LogViewerModel::Data & data, const QString & line) const;

private:
    QFile       m_targetFile;
    QRegExp     m_logParsingRegex;
};

} // namespace quentier

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H
