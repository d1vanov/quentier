#ifndef __TABLE_SIZE_SELECTOR_ACTION_WIDGET_H
#define __TABLE_SIZE_SELECTOR_ACTION_WIDGET_H

#include <QWidgetAction>

QT_FORWARD_DECLARE_CLASS(TableSizeSelector)

class TableSizeSelectorActionWidget : public QWidgetAction
{
    Q_OBJECT
public:
    explicit TableSizeSelectorActionWidget(QWidget * parent = 0);

Q_SIGNALS:
    void tableSizeSelected(int rows, int columns);

private:
    TableSizeSelector *     m_selector;
};

#endif // __TABLE_SIZE_SELECTOR_ACTION_WIDGET_H
