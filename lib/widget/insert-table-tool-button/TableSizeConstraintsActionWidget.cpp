/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include <quentier/utility/Macros.h>

#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QComboBox>

namespace quentier {

TableSizeConstraintsActionWidget::TableSizeConstraintsActionWidget(
        QWidget * parent) :
    QWidgetAction(parent)
{
    auto * layoutContainer = new QWidget(parent);
    auto * layout = new QHBoxLayout(layoutContainer);

    auto * widthSpinBox = new QDoubleSpinBox(layoutContainer);
    widthSpinBox->setMinimum(1.0);
    widthSpinBox->setMaximum(2000.0);
    widthSpinBox->setDecimals(2);
    widthSpinBox->setValue(m_currentWidth);

    auto * widthTypeComboBox = new QComboBox(layoutContainer);
    widthTypeComboBox->addItem(tr("pixels"));
    widthTypeComboBox->addItem(QStringLiteral("%"));
    widthTypeComboBox->setCurrentIndex(m_currentWidthTypeIsRelative ? 1 : 0);

    layout->addWidget(widthSpinBox);
    layout->addWidget(widthTypeComboBox);

    layoutContainer->setLayout(layout);
    setDefaultWidget(layoutContainer);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        widthSpinBox,
        qOverload<double>(&QDoubleSpinBox::valueChanged),
        this,
        &TableSizeConstraintsActionWidget::onWidthChange);

    QObject::connect(
        widthTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TableSizeConstraintsActionWidget::onWidthTypeChange);
#else
    QObject::connect(
        widthSpinBox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(onWidthChange(double)));

    QObject::connect(
        widthTypeComboBox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(onWidthTypeChange(int)));
#endif

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

    Q_EMIT chosenTableWidthConstraints(
        m_currentWidth,
        m_currentWidthTypeIsRelative);
}

void TableSizeConstraintsActionWidget::onWidthTypeChange(int widthTypeIndex)
{
    auto * pComboBox = qobject_cast<QComboBox*>(sender());
    Q_ASSERT(pComboBox);

    QString widthType = pComboBox->itemText(widthTypeIndex);

    if (widthType == QStringLiteral("%")) {
        m_currentWidthTypeIsRelative = true;
    }
    else {
        m_currentWidthTypeIsRelative = false;
    }

    Q_EMIT chosenTableWidthConstraints(
        m_currentWidth,
        m_currentWidthTypeIsRelative);
}

} // namespace quentier
