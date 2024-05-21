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

#include <QFrame>

namespace quentier {

class TableSizeSelector : public QFrame
{
    Q_OBJECT
public:
    explicit TableSizeSelector(QWidget * parent = nullptr);

Q_SIGNALS:
    void tableSizeSelected(int rows, int columns);

private:
    void paintEvent(QPaintEvent * event) override;

    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent * event) override;
#else
    void enterEvent(QEnterEvent * event) override;
#endif

    void leaveEvent(QEvent * event) override;

    [[nodiscard]] QSize sizeHint() const override;

private:
    int m_currentRow = -1;
    int m_currentColumn = -1;
    double m_rowHeight = 0.0;
    double m_columnWidth = 0.0;
    QRectF m_rect;
};

} // namespace quentier
