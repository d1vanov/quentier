/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include "ColumnChangeRerouter.h"

#include <quentier/logging/QuentierLogger.h>

#include <algorithm>

ColumnChangeRerouter::ColumnChangeRerouter(
    const int columnFrom, const int columnTo, QObject * parent) :
    QObject(parent),
    m_columnFrom(columnFrom), m_columnTo(columnTo)
{}

void ColumnChangeRerouter::setModel(QAbstractItemModel * model)
{
    QObject::connect(
        model, &QAbstractItemModel::dataChanged, this,
        &ColumnChangeRerouter::onModelDataChanged);
}

void ColumnChangeRerouter::onModelDataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNTRACE(
        "model",
        "ColumnChangeRerouter::onModelDataChanged: top left: "
            << "is valid = " << (topLeft.isValid() ? "true" : "false")
            << ", row = " << topLeft.row() << ", column = " << topLeft.column()
            << "; bottom right: is valid = "
            << (bottomRight.isValid() ? "true" : "false") << ", row = "
            << bottomRight.row() << ", column = " << bottomRight.column()
            << ", column from = " << m_columnFrom
            << ", column to = " << m_columnTo);

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        return;
    }

    int columnLeft = topLeft.column();
    int columnRight = bottomRight.column();

    if ((columnLeft <= m_columnTo) && (columnRight >= m_columnTo)) {
        QNTRACE("model", "Already includes column to");
        return;
    }

    if ((columnLeft > m_columnFrom) || (columnRight < m_columnFrom)) {
        QNTRACE("model", "Doesn't include column from");
        return;
    }

    const auto * model = topLeft.model();
    if (Q_UNLIKELY(!model)) {
        model = bottomRight.model();
    }

    if (Q_UNLIKELY(!model)) {
        QNDEBUG("model", "No model");
        return;
    }

    auto newTopLeft = model->index(topLeft.row(), m_columnTo, topLeft.parent());

    auto newBottomRight =
        model->index(bottomRight.row(), m_columnTo, bottomRight.parent());

    QNDEBUG(
        "model", "Emitting the dataChanged signal for column " << m_columnTo);

    Q_EMIT dataChanged(newTopLeft, newBottomRight, roles);
}
