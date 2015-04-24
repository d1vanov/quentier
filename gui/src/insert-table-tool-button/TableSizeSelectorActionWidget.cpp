#include "TableSizeSelectorActionWidget.h"
#include "TableSizeSelector.h"
#include <QHBoxLayout>

TableSizeSelectorActionWidget::TableSizeSelectorActionWidget(QWidget * parent) :
    QWidgetAction(parent),
    m_selector(new TableSizeSelector(parent))
{
    QObject::connect(m_selector, SIGNAL(tableSizeSelected(int,int)), this, SIGNAL(tableSizeSelected(int,int)));

    QHBoxLayout * layout = new QHBoxLayout(parent);
    layout->addWidget(m_selector);
    layout->setAlignment(Qt::AlignCenter);

    QWidget * layoutContainer = new QWidget(parent);
    layoutContainer->setLayout(layout);

    setDefaultWidget(layoutContainer);
}
