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
#include "SaveFilteredLogEntriesToFileProcessor.h"

namespace quentier {

LogViewerFilterModel::LogViewerFilterModel(QObject * parent) :
    QSortFilterProxyModel(parent),
    m_filterOutBeforeRow(-1),
    m_enabledLogLevels(),
    m_pSaveFilteredLogEntriesToFileProcessor(Q_NULLPTR)
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
    if (m_pSaveFilteredLogEntriesToFileProcessor)
    {
        if (filePath == m_pSaveFilteredLogEntriesToFileProcessor->targetFilePath()) {
            return;
        }

        // Aborting the current save operation
        m_pSaveFilteredLogEntriesToFileProcessor->stop();
        m_pSaveFilteredLogEntriesToFileProcessor->deleteLater();
        m_pSaveFilteredLogEntriesToFileProcessor = Q_NULLPTR;
    }

    const LogViewerModel * pLogViewerModel = qobject_cast<const LogViewerModel*>(sourceModel());
    if (Q_UNLIKELY(!pLogViewerModel)) {
        ErrorString errorDescription(QT_TR_NOOP("wrong source model is set to log viewer filter model"));
        Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, false, errorDescription);
        return;
    }

    m_pSaveFilteredLogEntriesToFileProcessor = new SaveFilteredLogEntriesToFileProcessor(filePath, *this, this);
    QObject::connect(m_pSaveFilteredLogEntriesToFileProcessor, QNSIGNAL(SaveFilteredLogEntriesToFileProcessor,finished,QString,bool,ErrorString),
                     this, QNSLOT(LogViewerFilterModel,onSavingFilteredLogEntriesToFileFinished,QString,bool,ErrorString));
    m_pSaveFilteredLogEntriesToFileProcessor->start();
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

void LogViewerFilterModel::onSavingFilteredLogEntriesToFileFinished(QString filePath, bool status, ErrorString errorDescription)
{
    if (m_pSaveFilteredLogEntriesToFileProcessor) {
        QObject::disconnect(m_pSaveFilteredLogEntriesToFileProcessor, QNSIGNAL(SaveFilteredLogEntriesToFileProcessor,finished,QString,bool,ErrorString),
                            this, QNSLOT(LogViewerFilterModel,onSavingFilteredLogEntriesToFileFinished,QString,bool,ErrorString));
        m_pSaveFilteredLogEntriesToFileProcessor->deleteLater();
        m_pSaveFilteredLogEntriesToFileProcessor = Q_NULLPTR;
    }

    Q_EMIT finishedSavingDisplayedLogEntriesToFile(filePath, status, errorDescription);
}

} // namespace quentier
