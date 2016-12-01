#include "TabWidget.h"

namespace quentier {

TabWidget::TabWidget(QWidget * parent) :
    QTabWidget(parent)
{}

QTabBar * TabWidget::tabBar() const
{
    return QTabWidget::tabBar();
}

} // namespace quentier
