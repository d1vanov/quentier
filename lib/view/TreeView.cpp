/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "TreeView.h"

#include <quentier/utility/Macros.h>

namespace quentier {

TreeView::TreeView(QWidget * parent) : QTreeView(parent) {}

QModelIndex TreeView::singleRow(
    const QModelIndexList & indexes, const QAbstractItemModel & model,
    const int column) const
{
    int row = -1;
    QModelIndex sourceIndex;

    for (const auto & index: qAsConst(indexes)) {
        if (Q_UNLIKELY(!index.isValid())) {
            continue;
        }

        if (row < 0) {
            row = index.row();
            sourceIndex = index;
            continue;
        }

        if (row != index.row()) {
            sourceIndex = QModelIndex();
            break;
        }
    }

    if (!sourceIndex.isValid()) {
        return sourceIndex;
    }

    return model.index(sourceIndex.row(), column, sourceIndex.parent());
}

void TreeView::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QTreeView::dataChanged(topLeft, bottomRight, roles);

    /**
     * The default implementation of QTreeView doesn't resize the columns after
     * the data change has been processed, regardless of the resize mode used.
     * In the ideal world the affected columns should only be automatically
     * resized if they don't have enough space to display the changed data but
     * for now, as a shortcut, the affected columns are always resized to ensure
     * their width is enough to display the changed data
     */

    if (Q_UNLIKELY(!topLeft.isValid() || !bottomRight.isValid())) {
        return;
    }

    int minColumn = topLeft.column();
    int maxColumn = bottomRight.column();
    for (int i = minColumn; i <= maxColumn; ++i) {
        resizeColumnToContents(i);
    }
}

} // namespace quentier
