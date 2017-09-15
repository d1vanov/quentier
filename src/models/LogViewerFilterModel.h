#ifndef QUENTIER_LOG_VIEWER_FILTER_MODEL_H
#define QUENTIER_LOG_VIEWER_FILTER_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>
#include <QSortFilterProxyModel>

namespace quentier {

class LogViewerFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    LogViewerFilterModel(QObject * parent = Q_NULLPTR);

    int filterOutBeforeRow() const { return m_filterOutBeforeRow; }
    void setFilterOutBeforeRow(const int filterOutBeforeRow);

    bool logLevelEnabled(const LogLevel::type logLevel) const;
    void setLogLevelEnabled(const LogLevel::type logLevel, const bool enabled);

private:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const Q_DECL_OVERRIDE;

private:
    int         m_filterOutBeforeRow;
    bool        m_enabledLogLevels[6];
};

} // namespace quentier

#endif // QUENTIER_LOG_VIEWER_FILTER_MODEL_H
