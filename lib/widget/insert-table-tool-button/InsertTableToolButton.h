/*
 * Copyright 2015-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_INSERT_TABLE_TOOL_BUTTON_H
#define QUENTIER_LIB_WIDGET_INSERT_TABLE_TOOL_BUTTON_H

#include <quentier/utility/Macros.h>

#include <QToolButton>

namespace quentier {

class InsertTableToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit InsertTableToolButton(QWidget * parent = nullptr);

Q_SIGNALS:
    void createdTable(int rows, int columns, double width, bool relative);

private Q_SLOTS:
    void onTableSettingsDialogAction();
    void onTableSizeChosen(int rows, int columns);
    void onTableSizeConstraintsChosen(double width, bool relative);

private:
    double m_currentWidth = 0.0;
    bool m_currentWidthIsRelative = false;
    QMenu * m_menu;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_INSERT_TABLE_TOOL_BUTTON_H
