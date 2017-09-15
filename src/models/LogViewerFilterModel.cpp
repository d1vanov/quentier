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
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    QModelIndex sourceIndex = pLogViewerModel->index(sourceRow, LogViewerModel::Columns::LogLevel, sourceParent);
    if (!sourceIndex.isValid()) {
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    QVariant logLevelData = pLogViewerModel->data(sourceIndex);
    bool convertedToInt = false;
    int logLevelInt = logLevelData.toInt(&convertedToInt);
    if (Q_UNLIKELY(!convertedToInt)) {
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    if (Q_UNLIKELY((logLevelInt < 0) || (logLevelInt >= static_cast<int>(sizeof(m_enabledLogLevels))))) {
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    if (!m_enabledLogLevels[static_cast<size_t>(logLevelInt)]) {
        return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

} // namespace quentier
