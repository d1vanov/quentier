#include "TableSizeConstraintsActionWidget.h"
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QComboBox>

TableSizeConstraintsActionWidget::TableSizeConstraintsActionWidget(QWidget * parent) :
    QWidgetAction(parent),
    m_currentWidth(400.0),
    m_currentWidthTypeIsRelative(false)
{
    QWidget * layoutContainer = new QWidget(parent);

    QHBoxLayout * layout = new QHBoxLayout(layoutContainer);

    QDoubleSpinBox * widthSpinBox = new QDoubleSpinBox(layoutContainer);
    widthSpinBox->setMinimum(1.0);
    widthSpinBox->setMaximum(2000.0);
    widthSpinBox->setDecimals(2);
    widthSpinBox->setValue(m_currentWidth);

    QComboBox * widthTypeComboBox = new QComboBox(layoutContainer);
    widthTypeComboBox->addItem("pixels");
    widthTypeComboBox->addItem("%");
    widthTypeComboBox->setCurrentIndex(m_currentWidthTypeIsRelative ? 1 : 0);

    layout->addWidget(widthSpinBox);
    layout->addWidget(widthTypeComboBox);

    layoutContainer->setLayout(layout);
    setDefaultWidget(layoutContainer);

    QObject::connect(widthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onWidthChange(double)));
    QObject::connect(widthTypeComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onWidthTypeChange(QString)));
}

double TableSizeConstraintsActionWidget::width() const
{
    return m_currentWidth;
}

bool TableSizeConstraintsActionWidget::isRelative() const
{
    return m_currentWidthTypeIsRelative;
}

void TableSizeConstraintsActionWidget::onWidthChange(double width)
{
    m_currentWidth = width;
    emit chosenTableWidthConstraints(m_currentWidth, m_currentWidthTypeIsRelative);
}

void TableSizeConstraintsActionWidget::onWidthTypeChange(QString widthType)
{
    if (widthType == "%") {
        m_currentWidthTypeIsRelative = true;
    }
    else {
        m_currentWidthTypeIsRelative = false;
    }

    emit chosenTableWidthConstraints(m_currentWidth, m_currentWidthTypeIsRelative);
}
