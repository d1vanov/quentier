/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_ACTION_WIDGET_H
#define QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_ACTION_WIDGET_H

#include <quentier/utility/Macros.h>

#include <QWidgetAction>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TableSizeSelector)

class TableSizeSelectorActionWidget : public QWidgetAction
{
    Q_OBJECT
public:
    explicit TableSizeSelectorActionWidget(QWidget * parent = Q_NULLPTR);

Q_SIGNALS:
    void tableSizeSelected(int rows, int columns);

private:
    TableSizeSelector *     m_selector;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_TABLE_SIZE_SELECTOR_ACTION_WIDGET_H
