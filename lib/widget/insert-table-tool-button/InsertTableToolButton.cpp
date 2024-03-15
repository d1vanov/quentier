/*
 * Copyright 2015-2024 Dmitry Ivanov
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
#include "TableSizeConstraintsActionWidget.h"
#include "TableSizeSelectorActionWidget.h"

#include <QMenu>

#include <memory>

namespace quentier {

InsertTableToolButton::InsertTableToolButton(QWidget * parent) :
    QToolButton{parent}, m_menu{new QMenu{this}}
{
    auto * sizeSelectorAction = new TableSizeSelectorActionWidget(this);
    m_menu->addAction(sizeSelectorAction);

    auto * constraintsSelectorAction =
        new TableSizeConstraintsActionWidget(this);

    m_menu->addAction(constraintsSelectorAction);

    setMenu(m_menu);

    auto * showTableSettingsDialogAction = new QAction(this);

    QObject::connect(
        showTableSettingsDialogAction, &QAction::triggered, this,
        &InsertTableToolButton::onTableSettingsDialogAction);

    setDefaultAction(showTableSettingsDialogAction);

    QObject::connect(
        sizeSelectorAction, &TableSizeSelectorActionWidget::tableSizeSelected,
        this, &InsertTableToolButton::onTableSizeChosen);

    QObject::connect(
        constraintsSelectorAction,
        &TableSizeConstraintsActionWidget::chosenTableWidthConstraints, this,
        &InsertTableToolButton::onTableSizeConstraintsChosen);

    m_currentWidth = constraintsSelectorAction->width();
    m_currentWidthIsRelative = constraintsSelectorAction->isRelative();
}

void InsertTableToolButton::onTableSettingsDialogAction()
{
    auto tableSettingsDialog = std::make_unique<TableSettingsDialog>(this);

    if (tableSettingsDialog->exec() == QDialog::Accepted) {
        int numRows = tableSettingsDialog->numRows();
        int numColumns = tableSettingsDialog->numColumns();
        double tableWidth = tableSettingsDialog->tableWidth();
        bool relativeWidth = tableSettingsDialog->relativeWidth();

        onTableSizeChosen(numRows, numColumns);
        onTableSizeConstraintsChosen(tableWidth, relativeWidth);
    }
}

void InsertTableToolButton::onTableSizeChosen(const int rows, const int columns)
{
    Q_EMIT createdTable(
        rows, columns, m_currentWidth, m_currentWidthIsRelative);
}

void InsertTableToolButton::onTableSizeConstraintsChosen(
    const double width, const bool relative)
{
    m_currentWidth = width;
    m_currentWidthIsRelative = relative;
}

} // namespace quentier
