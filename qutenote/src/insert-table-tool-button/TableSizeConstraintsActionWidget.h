#ifndef __TABLE_SIZE_CONSTRAINTS_ACTION_WIDGET_H
#define __TABLE_SIZE_CONSTRAINTS_ACTION_WIDGET_H

#include <QWidgetAction>

class TableSizeConstraintsActionWidget: public QWidgetAction
{
    Q_OBJECT
public:
    explicit TableSizeConstraintsActionWidget(QWidget * parent = 0);

    double width() const;
    bool isRelative() const;

Q_SIGNALS:
    void chosenTableWidthConstraints(double width, bool relative);

private Q_SLOTS:
    void onWidthChange(double width);
    void onWidthTypeChange(QString widthType);

private:
    double  m_currentWidth;
    bool    m_currentWidthTypeIsRelative;
};

#endif // __TABLE_SIZE_CONSTRAINTS_ACTION_WIDGET_H
