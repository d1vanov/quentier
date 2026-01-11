/*
 * Copyright 2016-2025 Dmitry Ivanov
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

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

namespace quentier {

namespace {

constexpr int gMaxRows = 10;
constexpr int gMaxColumns = 20;

} // namespace

TableSizeSelector::TableSizeSelector(QWidget * parent) : QFrame{parent}
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    setFrameShape(QFrame::Box);

    const QFontMetrics fontMetrics{font()};
    m_rowHeight = fontMetrics.height() + 2;
    m_columnWidth = fontMetrics.horizontalAdvance(QStringLiteral("  ")) + 3;

    m_rect.setHeight(m_rowHeight * gMaxRows);
    m_rect.setWidth(m_columnWidth * gMaxColumns);
}

void TableSizeSelector::paintEvent(QPaintEvent * event)
{
    QFrame::paintEvent(event);

    QPainter painter{this};

    // Fill background
    painter.fillRect(m_rect, palette().brush(QPalette::Base));

    // Fill current selection area (if any)
    if ((m_currentRow >= 0) && (m_currentColumn >= 0)) {
        QRectF selectionRect;
        selectionRect.setHeight(m_currentRow * m_rowHeight);
        selectionRect.setWidth(m_currentColumn * m_columnWidth);
        painter.fillRect(selectionRect, palette().brush(QPalette::Highlight));
    }

    QPen pen = painter.pen();
    pen.setWidthF(0.7);
    painter.setPen(pen);

    // Drawing rows grid
    for (int i = 0; i <= gMaxRows; ++i) {
        const double verticalPos = i * m_rowHeight;
        painter.drawLine(
            QPointF{0.0, verticalPos}, QPointF{m_rect.width(), verticalPos});
    }

    // Drawing columns grid
    for (int i = 0; i <= gMaxColumns; ++i) {
        const double horizontalPos = i * m_columnWidth;
        painter.drawLine(
            QPointF{horizontalPos, 0.0},
            QPointF{horizontalPos, m_rect.height()});
    }
}

void TableSizeSelector::mouseMoveEvent(QMouseEvent * event)
{
    const int x =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        qRound(event->position().x());
#else
        event->x();
#endif

    const int y =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        qRound(event->position().y());
#else
        event->y();
#endif

    m_currentRow = static_cast<int>(y / m_rowHeight) + 1;
    m_currentColumn = static_cast<int>(x / m_columnWidth) + 1;

    if (m_currentRow > gMaxRows) {
        m_currentRow = gMaxRows;
    }

    if (m_currentColumn > gMaxColumns) {
        m_currentColumn = gMaxColumns;
    }

    const auto globalPos =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        event->globalPosition().toPoint();
#else
        event->globalPos();
#endif

    QToolTip::showText(
        globalPos,
        QString::number(m_currentRow) + QStringLiteral("x") +
            QString::number(m_currentColumn));

    repaint();
}

void TableSizeSelector::mouseReleaseEvent(QMouseEvent * event)
{
    if (m_rect.contains(event->pos())) {
        Q_EMIT tableSizeSelected(m_currentRow, m_currentColumn);
    }

    QFrame::mouseReleaseEvent(event);
}

void TableSizeSelector::enterEvent(
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QEvent * event)
#else
    QEnterEvent * event)
#endif
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

} // namespace quentier
