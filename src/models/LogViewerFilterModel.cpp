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

#include "LogViewerFilterModel.h"

namespace quentier {

LogViewerFilterModel::LogViewerFilterModel(QObject * parent) :
    QSortFilterProxyModel(parent),
    m_filterOutBeforeRow(-1),
    m_enabledLogLevels(),
    m_savingDisplayedLogEntriesToFile(false),
    m_targetFilePathToSaveDisplayedLogEntriesTo(),
    m_targetFileToSaveDisplayedLogEntriesTo(),
    m_saveDisplayedLogEntriesToFileLastProcessedSourceModelRow(-1)
{
    for(size_t i = 0; i < sizeof(m_enabledLogLevels); ++i) {
        m_enabledLogLevels[i] = true;
    }
}

void LogViewerFilterModel::setFilterOutBeforeRow(const int filterOutBeforeRow)
{
    QNDEBUG(QStringLiteral("LogViewerFilterModel::setFilterOutBeforeRow: ") << filterOutBeforeRow);

    if (m_filterOutBeforeRow == filterOutBeforeRow) {
        QNDEBUG(QStringLiteral("The same filter out before row is already set"));
        return;
    }

    m_filterOutBeforeRow = filterOutBeforeRow;
    invalidateFilter();
}

bool LogViewerFilterModel::logLevelEnabled(const LogLevel::type logLevel) const
{
    return m_enabledLogLevels[static_cast<size_t>(logLevel)];
}

void LogViewerFilterModel::setLogLevelEnabled(const LogLevel::type logLevel, const bool enabled)
{
    if (logLevelEnabled(logLevel) == enabled) {
        return;
    }

    m_enabledLogLevels[static_cast<size_t>(logLevel)] = enabled;
    invalidateFilter();
}

void LogViewerFilterModel::saveDisplayedLogEntriesToFile(const QString & filePath)
{
    if (m_savingDisplayedLogEntriesToFile)
    {
        if (filePath == m_targetFilePathToSaveDisplayedLogEntriesTo) {
            return;
        }

        // TODO: abort current saving operation
    }

    const LogViewerModel * pLogViewerModel = qobject_cast<const LogViewerModel*>(sourceModel());
    if (Q_UNLIKELY(!pLogViewerModel)) {
        ErrorString errorDescription(QT_TR_NOOP("wrong source model is set to log viewer filter model"));
        Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, false, errorDescription);
        return;
    }

    int numSourceModelRows = pLogViewerModel->rowCount();
    if (Q_UNLIKELY(numSourceModelRows <= 0)) {
        ErrorString errorDescription(QT_TR_NOOP("source model is empty"));
        Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, false, errorDescription);
        return;
    }

    m_targetFileToSaveDisplayedLogEntriesTo.setFileName(filePath);
    if (!m_targetFileToSaveDisplayedLogEntriesTo.open(QIODevice::WriteOnly)) {
        ErrorString errorDescription(QT_TR_NOOP("can't open the target log file for writing"));
        errorDescription.details() = QStringLiteral("error code = ") + QString::number(m_targetFileToSaveDisplayedLogEntriesTo.error());
        Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, false, errorDescription);
        return;
    }

    m_targetFilePathToSaveDisplayedLogEntriesTo = filePath;

    int row = 0;
    while(row < numSourceModelRows)
    {
        int startModelRow = 0;
        const QVector<LogViewerModel::Data> * pLogFileDataChunk = pLogViewerModel->dataChunkContainingModelRow(row, &startModelRow);
        if (!pLogFileDataChunk) {
            break;
        }

        for(int i = 0, size = pLogFileDataChunk->size(); i < size; ++i) {
            processLogFileDataEntryForSavingToFile(pLogFileDataChunk->at(i));
        }

        row = startModelRow + pLogFileDataChunk->size();
    }

    if (row < numSourceModelRows)
    {
        // We left the above loop because encountered a cache miss; need to force the source model to load the data for this row
        m_saveDisplayedLogEntriesToFileLastProcessedSourceModelRow = row;
        QObject::connect(pLogViewerModel, QNSIGNAL(LogViewerModel,notifyModelRowsCached,int,int),
                         this, QNSLOT(LogViewerFilterModel,onLogViewerModelRowsCached,int,int));
        Q_UNUSED(pLogViewerModel->data(pLogViewerModel->index(row, 0)))
        // The source model will send notifyModelRowsCached to our onLogViewerModelRowsCached slot so we'll continue the processing there
        return;
    }

    // If we got here, we are finished
    m_savingDisplayedLogEntriesToFile = false;
    m_targetFilePathToSaveDisplayedLogEntriesTo.clear();
    m_targetFileToSaveDisplayedLogEntriesTo.close();
    m_saveDisplayedLogEntriesToFileLastProcessedSourceModelRow = -1;
    Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, true, ErrorString());
}

bool LogViewerFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
    if (sourceRow < m_filterOutBeforeRow) {
        return false;
    }

    const LogViewerModel * pLogViewerModel = qobject_cast<const LogViewerModel*>(sourceModel());
    if (Q_UNLIKELY(!pLogViewerModel)) {
        return false;
    }

    QModelIndex sourceIndex = pLogViewerModel->index(sourceRow, LogViewerModel::Columns::LogLevel, sourceParent);
    if (!sourceIndex.isValid()) {
        return false;
    }

    const LogViewerModel::Data * pDataEntry = pLogViewerModel->dataEntry(sourceIndex.row());
    if (Q_UNLIKELY(!pDataEntry)) {
        return false;
    }

    return filterAcceptsEntry(*pDataEntry);
}

void LogViewerFilterModel::onLogViewerModelRowsCached(int from, int to)
{
    // TODO: implement
    Q_UNUSED(from)
    Q_UNUSED(to)
}

bool LogViewerFilterModel::filterAcceptsEntry(const LogViewerModel::Data & entry) const
{
    int logLevelInt = entry.m_logLevel;
    if (Q_UNLIKELY((logLevelInt < 0) || (logLevelInt >= static_cast<int>(sizeof(m_enabledLogLevels))))) {
        return false;
    }

    if (!m_enabledLogLevels[static_cast<size_t>(logLevelInt)]) {
        return false;
    }

    QRegExp regExp = filterRegExp();

    const QString & sourceFileName = entry.m_sourceFileName;
    if (regExp.indexIn(sourceFileName) >= 0) {
        return true;
    }

    const QString & logEntry = entry.m_logEntry;
    if (regExp.indexIn(logEntry) >= 0) {
        return true;
    }

    return false;
}

void LogViewerFilterModel::processLogFileDataEntryForSavingToFile(const LogViewerModel::Data & entry)
{
    if (!filterAcceptsEntry(entry)) {
        return;
    }

    const LogViewerModel * pLogViewerModel = qobject_cast<const LogViewerModel*>(sourceModel());
    if (Q_UNLIKELY(!pLogViewerModel)) {
        return;
    }

    QString entryString = pLogViewerModel->dataEntryToString(entry);
    entryString += QStringLiteral("\n");
    Q_UNUSED(m_targetFileToSaveDisplayedLogEntriesTo.write(entryString.toUtf8()))
}

} // namespace quentier
