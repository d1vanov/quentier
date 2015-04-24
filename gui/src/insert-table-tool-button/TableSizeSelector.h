#ifndef __TABLE_SIZE_SELECTOR_H
#define __TABLE_SIZE_SELECTOR_H

#include <QFrame>

class TableSizeSelector : public QFrame
{
    Q_OBJECT
public:
    explicit TableSizeSelector(QWidget * parent = 0);

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
    int         m_currentRow;
    int         m_currentColumn;
    double      m_rowHeight;
    double      m_columnWidth;
    QRectF      m_rect;
};

#endif // __TABLE_SIZE_SELECTOR_H
