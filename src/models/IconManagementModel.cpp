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

#include "IconManagementModel.h"
#include "../IconThemeManager.h"

#define ICON_MANAGEMENT_MODEL_COLUMN_COUNT (3)

namespace quentier {

IconManagementModel::IconManagementModel(IProvider & provider, QObject * parent) :
    QAbstractTableModel(parent),
    m_provider(provider),
    m_usedIconNames(m_provider.usedIconNames())
{}

int IconManagementModel::rowCount(const QModelIndex & parent) const
{
    if (Q_UNLIKELY(parent.isValid())) {
        return 0;
    }

    return m_usedIconNames.size();
}

int IconManagementModel::columnCount(const QModelIndex & parent) const
{
    if (Q_UNLIKELY(parent.isValid())) {
        return 0;
    }

    return ICON_MANAGEMENT_MODEL_COLUMN_COUNT;
}

QVariant IconManagementModel::data(const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (!index.isValid()) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= ICON_MANAGEMENT_MODEL_COLUMN_COUNT)) {
        return QVariant();
    }

    int rowIndex = index.row();
    if ((rowIndex < 0) || (rowIndex >= m_usedIconNames.size())) {
        return QVariant();
    }

    const QString & name = m_usedIconNames[rowIndex];
    IconThemeManager & iconThemeManager = m_provider.iconThemeManager();

    switch(columnIndex)
    {
    case Columns::OriginalThemeIcon:
        return QIcon::fromTheme(name);
    case Columns::OverrideThemeIcon:
        return iconThemeManager.overrideThemeIcon(name);
    case Columns::FallbackThemeIcon:
        return iconThemeManager.iconFromFallbackTheme(name);
    }

    return QVariant();
}

QVariant IconManagementModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::OriginalThemeIcon:
        return QVariant(tr("Original"));
    case Columns::OverrideThemeIcon:
        return QVariant(tr("Override"));
    case Columns::FallbackThemeIcon:
        return QVariant(tr("Fallback"));
    }

    return QVariant();
}

} // namespace quentier
