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

public:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const Q_DECL_OVERRIDE;

private:
    int         m_filterOutBeforeRow;
    bool        m_enabledLogLevels[6];
};

} // namespace quentier

#endif // QUENTIER_LOG_VIEWER_FILTER_MODEL_H
