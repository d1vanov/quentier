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

#include "LogViewerModel.h"
#include "LogViewerModelLogFileParser.h"

#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QStringList>

namespace quentier {

class LogViewerModel::FileReaderAsync final : public QObject
{
    Q_OBJECT
public:
    explicit FileReaderAsync(
        const QString & targetFilePath,
        const QList<LogLevel> & disabledLogLevels,
        const QString & logEntryContentFilter, QObject * parent = nullptr);

    ~FileReaderAsync() override;

    Q_DISABLE_COPY(FileReaderAsync)

Q_SIGNALS:
    void readLogFileDataEntries(
        qint64 fromPos, qint64 endPos,
        QList<LogViewerModel::Data> dataEntries,
        ErrorString errorDescription);

public Q_SLOTS:
    void onReadDataEntriesFromLogFile(qint64 fromPos, int maxDataEntries);

private:
    QFile m_targetFile;
    QList<LogLevel> m_disabledLogLevels;
    QRegularExpression m_filterRegExp;
    LogViewerModel::LogFileParser m_parser;
};

} // namespace quentier
