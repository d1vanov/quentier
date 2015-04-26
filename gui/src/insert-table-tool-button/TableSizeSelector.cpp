#include "TableSizeSelector.h"
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
    m_rect(),
    m_horizontalMargin(2),
    m_verticalMargin(2)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Sunken);
    setLineWidth(0);
    setMidLineWidth(2);

    QFontMetrics fontMetrics(font());
    m_rowHeight = fontMetrics.height() + 2;
    m_columnWidth = fontMetrics.width("  ") + 3;

    m_rect.setHeight(m_rowHeight * MAX_ROWS);
    m_rect.setWidth(m_columnWidth * MAX_COLUMNS);
    m_rect.translate(m_horizontalMargin, m_verticalMargin);
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
        selectionRect.translate(m_horizontalMargin, m_verticalMargin);
        painter.fillRect(selectionRect, palette().brush(QPalette::Highlight));
    }

    QPen pen = painter.pen();
    pen.setWidthF(0.7);
    painter.setPen(pen);

    // Drawing rows grid
    for(int i = 0; i <= MAX_ROWS; ++i) {
        double verticalPos = i * m_rowHeight + m_verticalMargin;
        painter.drawLine(QPointF(m_horizontalMargin, verticalPos), QPointF(m_rect.width() + m_horizontalMargin, verticalPos));
    }

    // Drawing columns grid
    for(int i = 0; i <= MAX_COLUMNS; ++i) {
        double horizontalPos = i * m_columnWidth + m_horizontalMargin;
        painter.drawLine(QPointF(horizontalPos, m_verticalMargin), QPointF(horizontalPos, m_rect.height() + m_verticalMargin));
    }
}

void TableSizeSelector::mouseMoveEvent(QMouseEvent * event)
{
    m_currentRow = static_cast<int>((event->y() - m_verticalMargin) / m_rowHeight) + 1;
    m_currentColumn = static_cast<int>((event->x() - m_horizontalMargin) / m_columnWidth) + 1;

    if (m_currentRow > MAX_ROWS) {
        m_currentRow = MAX_ROWS;
    }

    if (m_currentColumn > MAX_COLUMNS) {
        m_currentColumn = MAX_COLUMNS;
    }

    QToolTip::showText(event->globalPos(), QString::number(m_currentRow) + "x" + QString::number(m_currentColumn));
    repaint();
}

void TableSizeSelector::mouseReleaseEvent(QMouseEvent * event)
{
    if (m_rect.contains(event->pos())) {
        emit tableSizeSelected(m_currentRow, m_currentColumn);
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
    size.setHeight(static_cast<int>(m_rect.height()) + 2 * m_verticalMargin);
    size.setWidth(static_cast<int>(m_rect.width()) + 2 * m_horizontalMargin);
    return size;
}
