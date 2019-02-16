/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "TableSizeSelector.h"
#include <quentier/utility/Macros.h>
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>

#define MAX_ROWS (10)
#define MAX_COLUMNS (20)

TableSizeSelector::TableSizeSelector(QWidget * parent) :
    QFrame(parent),
    m_currentRow(-1),
    m_currentColumn(-1),
    m_rowHeight(0),
    m_columnWidth(0),
    m_rect()
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    setFrameShape(QFrame::Box);

    QFontMetrics fontMetrics(font());
    m_rowHeight = fontMetrics.height() + 2;
    m_columnWidth = fontMetrics.width(QStringLiteral("  ")) + 3;

    m_rect.setHeight(m_rowHeight * MAX_ROWS);
    m_rect.setWidth(m_columnWidth * MAX_COLUMNS);
}

void TableSizeSelector::paintEvent(QPaintEvent * event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);

    // Fill background
    painter.fillRect(m_rect, palette().brush(QPalette::Base));

    // Fill current selection area (if any)
    if ((m_currentRow >= 0) && (m_currentColumn >= 0))
    {
        QRectF selectionRect;
        selectionRect.setHeight(m_currentRow * m_rowHeight);
        selectionRect.setWidth(m_currentColumn * m_columnWidth);
        painter.fillRect(selectionRect, palette().brush(QPalette::Highlight));
    }

    QPen pen = painter.pen();
    pen.setWidthF(0.7);
    painter.setPen(pen);

    // Drawing rows grid
    for(int i = 0; i <= MAX_ROWS; ++i) {
        double verticalPos = i * m_rowHeight;
        painter.drawLine(QPointF(0.0, verticalPos),
                         QPointF(m_rect.width(), verticalPos));
    }

    // Drawing columns grid
    for(int i = 0; i <= MAX_COLUMNS; ++i) {
        double horizontalPos = i * m_columnWidth;
        painter.drawLine(QPointF(horizontalPos, 0.0),
                         QPointF(horizontalPos, m_rect.height()));
    }
}

void TableSizeSelector::mouseMoveEvent(QMouseEvent * event)
{
    m_currentRow = static_cast<int>(event->y() / m_rowHeight) + 1;
    m_currentColumn = static_cast<int>(event->x() / m_columnWidth) + 1;

    if (m_currentRow > MAX_ROWS) {
        m_currentRow = MAX_ROWS;
    }

    if (m_currentColumn > MAX_COLUMNS) {
        m_currentColumn = MAX_COLUMNS;
    }

    QToolTip::showText(event->globalPos(), QString::number(m_currentRow) +
                       QStringLiteral("x") + QString::number(m_currentColumn));
    repaint();
}

void TableSizeSelector::mouseReleaseEvent(QMouseEvent * event)
{
    if (m_rect.contains(event->pos())) {
        Q_EMIT tableSizeSelected(m_currentRow, m_currentColumn);
    }

    QFrame::mouseReleaseEvent(event);
}

void TableSizeSelector::enterEvent(QEvent *event)
{
    QFrame::enterEvent(event);
    repaint();
}

void TableSizeSelector::leaveEvent(QEvent * event)
{
    QFrame::leaveEvent(event);

    m_currentRow = -1;
    m_currentColumn = -1;
    repaint();
}

QSize TableSizeSelector::sizeHint() const
{
    QSize size;
    size.setHeight(static_cast<int>(m_rect.height()) + 1);
    size.setWidth(static_cast<int>(m_rect.width()) + 1);
    return size;
}
