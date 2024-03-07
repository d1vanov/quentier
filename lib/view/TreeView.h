/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <QTreeView>

namespace quentier {

/**
 * @brief The TreeView class provides somewhat extended version of
 * QTreeView (can be used with tree or table models) which works around
 * different quirks of the default implementations like column sizing
 */
class TreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit TreeView(QWidget * parent = nullptr);

    /**
     * @return              Valid index if all indexes in the list point to
     *                      the same row or invalid model index otherwise
     */
    [[nodiscard]] QModelIndex singleRow(
        const QModelIndexList & indexes, const QAbstractItemModel & model,
        const int column) const;

public Q_SLOTS:
    /**
     * @brief dataChanged slot redefines QTreeView's implementation: it
     * forces the columns affected by the data change to be automatically
     * resized after the change was processed; QTreeView's default 
     * implementation does not do that.
     *
     * @param topLeft       Top left model index of the changed data
     * @param bottomRight   Bottom right model index of the changed data
     * @param roles         Roles under which the data has been changed
     */
    void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = {}) override;
};

} // namespace quentier
