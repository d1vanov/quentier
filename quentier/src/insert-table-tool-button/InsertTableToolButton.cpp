/*
* The MIT License (MIT)
*
* Copyright (c) 2015 Dmitry Ivanov
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include "InsertTableToolButton.h"
#include "TableSettingsDialog.h"
#include "TableSizeSelectorActionWidget.h"
#include "TableSizeConstraintsActionWidget.h"
#include <QMenu>

InsertTableToolButton::InsertTableToolButton(QWidget * parent) :
    QToolButton(parent),
    m_currentWidth(0.0),
    m_currentWidthIsRelative(false),
    m_menu(new QMenu(this))
{
    TableSizeSelectorActionWidget * sizeSelectorAction = new TableSizeSelectorActionWidget(this);
    m_menu->addAction(sizeSelectorAction);

    TableSizeConstraintsActionWidget * constraintsSelectorAction = new TableSizeConstraintsActionWidget(this);
    m_menu->addAction(constraintsSelectorAction);

    setMenu(m_menu);

    QAction * showTableSettingsDialogAction = new QAction(this);
    QObject::connect(showTableSettingsDialogAction, SIGNAL(triggered(bool)), this, SLOT(onTableSettingsDialogAction()));

    setDefaultAction(showTableSettingsDialogAction);

    QObject::connect(sizeSelectorAction, SIGNAL(tableSizeSelected(int,int)), this, SLOT(onTableSizeChosen(int,int)));
    QObject::connect(constraintsSelectorAction, SIGNAL(chosenTableWidthConstraints(double,bool)),
                     this, SLOT(onTableSizeConstraintsChosen(double,bool)));

    m_currentWidth = constraintsSelectorAction->width();
    m_currentWidthIsRelative = constraintsSelectorAction->isRelative();
}

void InsertTableToolButton::onTableSettingsDialogAction()
{
    QScopedPointer<TableSettingsDialog> tableSettingsDialogHolder(new TableSettingsDialog(this));
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
    emit createdTable(rows, columns, m_currentWidth, m_currentWidthIsRelative);
}

void InsertTableToolButton::onTableSizeConstraintsChosen(double width, bool relative)
{
    m_currentWidth = width;
    m_currentWidthIsRelative = relative;
}
