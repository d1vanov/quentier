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
#include "LogViewerModel.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

LogViewerFilterModel::LogViewerFilterModel(QObject * parent) :
    QSortFilterProxyModel(parent),
    m_filterOutBeforeRow(-1),
    m_enabledLogLevels()
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

    int logLevelInt = pDataEntry->m_logLevel;
    if (Q_UNLIKELY((logLevelInt < 0) || (logLevelInt >= static_cast<int>(sizeof(m_enabledLogLevels))))) {
        return false;
    }

    if (!m_enabledLogLevels[static_cast<size_t>(logLevelInt)]) {
        return false;
    }

    QRegExp regExp = filterRegExp();

    const QString & sourceFileName = pDataEntry->m_sourceFileName;
    if (regExp.indexIn(sourceFileName) >= 0) {
        return true;
    }

    const QString & logEntry = pDataEntry->m_logEntry;
    if (regExp.indexIn(logEntry) >= 0) {
        return true;
    }

    return false;
}

} // namespace quentier
