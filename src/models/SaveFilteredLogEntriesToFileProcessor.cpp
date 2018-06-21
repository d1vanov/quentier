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

#include "SaveFilteredLogEntriesToFileProcessor.h"
#include "LogViewerFilterModel.h"

namespace quentier {

SaveFilteredLogEntriesToFileProcessor::SaveFilteredLogEntriesToFileProcessor(const QString & targetFilePath,
                                                                             LogViewerFilterModel & model,
                                                                             QObject * parent) :
    QObject(parent),
    m_pModel(&model),
    m_targetFilePath(targetFilePath),
    m_targetFile(),
    m_isActive(false),
    m_lastProcessedModelRow(-1)
{}

SaveFilteredLogEntriesToFileProcessor::~SaveFilteredLogEntriesToFileProcessor()
{}

bool SaveFilteredLogEntriesToFileProcessor::isActive() const
{
    return m_isActive;
}

const QString & SaveFilteredLogEntriesToFileProcessor::targetFilePath() const
{
    return m_targetFilePath;
}

void SaveFilteredLogEntriesToFileProcessor::start()
{
    if (m_isActive) {
        return;
    }

    m_targetFile.setFileName(m_targetFilePath);
    if (!m_targetFile.open(QIODevice::WriteOnly)) {
        ErrorString errorDescription(QT_TR_NOOP("failed to open the file for writing"));
        Q_EMIT finished(m_targetFilePath, false, errorDescription);
        return;
    }

    processLogModelRows(0);
}

void SaveFilteredLogEntriesToFileProcessor::stop()
{
    if (!m_isActive) {
        return;
    }

    m_targetFile.close();
    m_isActive = false;
    m_lastProcessedModelRow = -1;
}

void SaveFilteredLogEntriesToFileProcessor::onLogViewerModelRowsCached(int from, int to)
{
    Q_UNUSED(to)

    if (!m_isActive) {
        return;
    }

    if (from != (m_lastProcessedModelRow + 1)) {
        return;
    }

    processLogModelRows(from);
}

void SaveFilteredLogEntriesToFileProcessor::processLogModelRows(const int from)
{
    if (Q_UNLIKELY(m_pModel.isNull())) {
        ErrorString errorDescription(QT_TR_NOOP("internal error: null pointer to log viewer filter model"));
        Q_EMIT finished(m_targetFilePath, false, errorDescription);
        return;
    }

    LogViewerModel * pLogViewerModel = qobject_cast<LogViewerModel*>(m_pModel.data()->sourceModel());
    if (Q_UNLIKELY(!pLogViewerModel)) {
        ErrorString errorDescription(QT_TR_NOOP("internal error: no source log viewer model in log viewer filter model"));
        Q_EMIT finished(m_targetFilePath, false, errorDescription);
        return;
    }

    int numSourceModelRows = pLogViewerModel->rowCount();
    if (Q_UNLIKELY(numSourceModelRows <= 0)) {
        ErrorString errorDescription(QT_TR_NOOP("source model is empty"));
        Q_EMIT finished(m_targetFilePath, false, errorDescription);
        return;
    }

    int row = from;
    while(row < numSourceModelRows)
    {
        int startModelRow = 0;
        const QVector<LogViewerModel::Data> * pLogFileDataChunk = pLogViewerModel->dataChunkContainingModelRow(row, &startModelRow);
        if (!pLogFileDataChunk) {
            break;
        }

        for(int i = 0, size = pLogFileDataChunk->size(); i < size; ++i) {
            processLogDataEntry(*m_pModel.data(), *pLogViewerModel, pLogFileDataChunk->at(i));
        }

        row = startModelRow + pLogFileDataChunk->size();
    }

    if (row < numSourceModelRows)
    {
        // We left the above loop due to a cache miss; need to force the source model to load the data for the next row
        m_lastProcessedModelRow = row;
        QObject::connect(pLogViewerModel, QNSIGNAL(LogViewerModel,notifyModelRowsCached,int,int),
                         this, QNSLOT(SaveFilteredLogEntriesToFileProcessor,onLogViewerModelRowsCached,int,int),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        Q_UNUSED(pLogViewerModel->data(pLogViewerModel->index(row, 0)))
        // The source model will send notifyModelRowsCached to our onLogViewerModelRowsCached slot so we'll continue the processing there
        return;
    }

    // If we got here, we have processed all the entries within the model
    // but it might be incomplete since LogViewerModel is lazy and doesn't load
    // all the logs into itself unless asked to do so
    if (pLogViewerModel->canFetchMore(QModelIndex())) {
        m_lastProcessedModelRow = row;
        QObject::connect(pLogViewerModel, QNSIGNAL(LogViewerModel,notifyModelRowsCached,int,int),
                         this, QNSLOT(SaveFilteredLogEntriesToFileProcessor,onLogViewerModelRowsCached,int,int),
                         Qt::ConnectionType(Qt::UniqueConnection | Qt::QueuedConnection));
        pLogViewerModel->fetchMore(QModelIndex());
        // The source model will send notifyModelRowsCached to our onLogViewerModelRowsCached slot so we'll continue the processing there
        return;
    }

    // If we got here, we are finished
    QObject::disconnect(pLogViewerModel, QNSIGNAL(LogViewerModel,notifyModelRowsCached,int,int),
                        this, QNSLOT(SaveFilteredLogEntriesToFileProcessor,onLogViewerModelRowsCached,int,int));

    m_isActive = false;
    m_targetFile.close();
    Q_EMIT finished(m_targetFilePath, true, ErrorString());
}

void SaveFilteredLogEntriesToFileProcessor::processLogDataEntry(const LogViewerFilterModel & filterModel,
                                                                const LogViewerModel & sourceModel,
                                                                const LogViewerModel::Data & entry)
{
    if (!filterModel.filterAcceptsEntry(entry)) {
        return;
    }

    QString entryString = sourceModel.dataEntryToString(entry);
    entryString += QStringLiteral("\n");
    Q_UNUSED(m_targetFile.write(entryString.toUtf8()))
}

} // namespace quentier
