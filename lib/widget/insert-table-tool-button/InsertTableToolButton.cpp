/*
 * Copyright 2015-2019 Dmitry Ivanov
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

#include "InsertTableToolButton.h"
#include "TableSettingsDialog.h"
#include "TableSizeSelectorActionWidget.h"
#include "TableSizeConstraintsActionWidget.h"

#include <QMenu>

namespace quentier {

InsertTableToolButton::InsertTableToolButton(QWidget * parent) :
    QToolButton(parent),
    m_currentWidth(0.0),
    m_currentWidthIsRelative(false),
    m_menu(new QMenu(this))
{
    TableSizeSelectorActionWidget * sizeSelectorAction =
        new TableSizeSelectorActionWidget(this);
    m_menu->addAction(sizeSelectorAction);

    TableSizeConstraintsActionWidget * constraintsSelectorAction =
        new TableSizeConstraintsActionWidget(this);
    m_menu->addAction(constraintsSelectorAction);

    setMenu(m_menu);

    QAction * showTableSettingsDialogAction = new QAction(this);
    QObject::connect(showTableSettingsDialogAction, SIGNAL(triggered(bool)),
                     this, SLOT(onTableSettingsDialogAction()));

    setDefaultAction(showTableSettingsDialogAction);

    QObject::connect(sizeSelectorAction, SIGNAL(tableSizeSelected(int,int)),
                     this, SLOT(onTableSizeChosen(int,int)));
    QObject::connect(constraintsSelectorAction,
                     SIGNAL(chosenTableWidthConstraints(double,bool)),
                     this, SLOT(onTableSizeConstraintsChosen(double,bool)));

    m_currentWidth = constraintsSelectorAction->width();
    m_currentWidthIsRelative = constraintsSelectorAction->isRelative();
}

void InsertTableToolButton::onTableSettingsDialogAction()
{
    QScopedPointer<TableSettingsDialog> tableSettingsDialogHolder(
        new TableSettingsDialog(this));
    TableSettingsDialog * tableSettingsDialog = tableSettingsDialogHolder.data();
    if (tableSettingsDialog->exec() == QDialog::Accepted)
    {
        int numRows = tableSettingsDialog->numRows();
        int numColumns = tableSettingsDialog->numColumns();
        double tableWidth = tableSettingsDialog->tableWidth();
        bool relativeWidth = tableSettingsDialog->relativeWidth();

        onTableSizeChosen(numRows, numColumns);
        onTableSizeConstraintsChosen(tableWidth, relativeWidth);
    }
}

void InsertTableToolButton::onTableSizeChosen(int rows, int columns)
{
    Q_EMIT createdTable(rows, columns, m_currentWidth, m_currentWidthIsRelative);
}

void InsertTableToolButton::onTableSizeConstraintsChosen(double width, bool relative)
{
    m_currentWidth = width;
    m_currentWidthIsRelative = relative;
}

} // namespace quentier
