/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_SAVE_FILTERED_LOG_ENTRIES_TO_FILE_PROCESSOR_H
#define QUENTIER_MODELS_SAVE_FILTERED_LOG_ENTRIES_TO_FILE_PROCESSOR_H

#include "LogViewerFilterModel.h"
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QObject>
#include <QPointer>
#include <QFile>

namespace quentier {

class SaveFilteredLogEntriesToFileProcessor: public QObject
{
    Q_OBJECT
public:
    explicit SaveFilteredLogEntriesToFileProcessor(const QString & targetFilePath,
                                                   LogViewerFilterModel & model,
                                                   QObject * parent = Q_NULLPTR);
    ~SaveFilteredLogEntriesToFileProcessor();

    bool isActive() const;
    const QString & targetFilePath() const;

Q_SIGNALS:
    void finished(QString filePath, bool status, ErrorString errorDescription);

public Q_SLOTS:
    void start();
    void stop();

private Q_SLOTS:
    void onLogViewerModelRowsCached(int from, int to);

private:
    void processLogModelRows(const int from);
    void processLogDataEntry(const LogViewerFilterModel & filterModel,
                             const LogViewerModel & sourceModel,
                             const LogViewerModel::Data & entry);

private:
    Q_DISABLE_COPY(SaveFilteredLogEntriesToFileProcessor)

private:
    QPointer<LogViewerFilterModel>  m_pModel;
    QString     m_targetFilePath;
    QFile       m_targetFile;
    bool        m_isActive;
    int         m_lastProcessedModelRow;
};

} // namespace quentier

#endif // QUENTIER_MODELS_SAVE_FILTERED_LOG_ENTRIES_TO_FILE_PROCESSOR_H
