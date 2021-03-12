/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_COLUMN_CHANGE_REROUTER_H
#define QUENTIER_LIB_MODEL_COLUMN_CHANGE_REROUTER_H

#include <QAbstractItemModel>

/**
 * @brief The ColumnChangeRerouter catches the dataChanged signal from the model
 * and emits its own dataChanged signal with the same row and parent item but
 * with different column
 */
class ColumnChangeRerouter final : public QObject
{
    Q_OBJECT
public:
    explicit ColumnChangeRerouter(
        int columnFrom, int columnTo, QObject * parent = nullptr);

    ~ColumnChangeRerouter() override;

    void setModel(QAbstractItemModel * model);

Q_SIGNALS:
    void dataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

private Q_SLOTS:
    void onModelDataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

private:
    int m_columnFrom;
    int m_columnTo;
};

#endif // QUENTIER_LIB_MODEL_COLUMN_CHANGE_REROUTER_H
