/*
 * Copyright 2016 Dmitry Ivanov
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

#include "TableSizeConstraintsActionWidget.h"
#include <quentier/utility/Qt4Helper.h>
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
    if (widthType == QStringLiteral("%")) {
        m_currentWidthTypeIsRelative = true;
    }
    else {
        m_currentWidthTypeIsRelative = false;
    }

    emit chosenTableWidthConstraints(m_currentWidth, m_currentWidthTypeIsRelative);
}
