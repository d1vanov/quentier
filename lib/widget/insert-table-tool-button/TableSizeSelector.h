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

#ifndef QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_H
#define QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_H

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
    virtual void paintEvent(QPaintEvent * event);

    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);

    virtual void enterEvent(QEvent * event);
    virtual void leaveEvent(QEvent * event);

    virtual QSize sizeHint() const;

private:
    int m_currentRow = -1;
    int m_currentColumn = -1;
    double m_rowHeight = 0.0;
    double m_columnWidth = 0.0;
    QRectF m_rect;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_H
