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

#include "TableSizeSelectorActionWidget.h"

#include "TableSizeSelector.h"

#include <QHBoxLayout>

namespace quentier {

TableSizeSelectorActionWidget::TableSizeSelectorActionWidget(QWidget * parent) :
    QWidgetAction(parent),
    m_selector(new TableSizeSelector(parent))
{
    QObject::connect(
        m_selector,
        &TableSizeSelector::tableSizeSelected,
        this,
        &TableSizeSelectorActionWidget::tableSizeSelected);

    auto * layout = new QHBoxLayout(parent);
    layout->addWidget(m_selector);
    layout->setAlignment(Qt::AlignCenter);

    auto * layoutContainer = new QWidget(parent);
    layoutContainer->setLayout(layout);

    setDefaultWidget(layoutContainer);
}

} // namespace quentier
