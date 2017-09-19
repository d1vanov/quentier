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

namespace quentier {

class LogViewerModel::FileReaderAsync : public QObject
{
    Q_OBJECT
public:
    explicit FileReaderAsync(QString targetFilePath, qint64 startPos,
                             QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void finished(qint64 currentPos, QString readData, ErrorString errorDescription);

public Q_SLOTS:
    void onStartReading();

private:
    QFile       m_targetFile;
    qint64      m_startPos;
};

} // namespace quentier

#endif // QUENTIER_MODELS_LOG_VIEWER_MODEL_FILE_READER_ASYNC_H
