#ifndef QUENTIER_WIDGETS_TAB_WIDGET_H
#define QUENTIER_WIDGETS_TAB_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <QTabWidget>

namespace quentier {

class TabWidget: public QTabWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget * parent = Q_NULLPTR);

    QTabBar * tabBar() const;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_TAB_WIDGET_H
