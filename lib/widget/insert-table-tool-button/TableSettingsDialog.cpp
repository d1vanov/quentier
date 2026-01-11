/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "TableSettingsDialog.h"
#include "ui_TableSettingsDialog.h"

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

TableSettingsDialog::TableSettingsDialog(QWidget * parent) :
    QDialog{parent}, m_ui{new Ui::TableSettingsDialog}
{
    m_ui->setupUi(this);

    m_ui->warningLine->clear();
    m_ui->warningLine->setHidden(true);

    auto * tableWidthModeComboBox = m_ui->tableWidthModeComboBox;
    tableWidthModeComboBox->addItem(tr("pixels"));
    tableWidthModeComboBox->addItem(tr("% of page width"));
    tableWidthModeComboBox->setCurrentIndex(1);

    QObject::connect(
        m_ui->buttonBox, &QDialogButtonBox::accepted, this,
        &TableSettingsDialog::onOkButtonPressed);

    QObject::connect(
        m_ui->buttonBox, &QDialogButtonBox::rejected, this,
        &TableSettingsDialog::onCancelButtonPressed);
}

TableSettingsDialog::~TableSettingsDialog()
{
    delete m_ui;
}

int TableSettingsDialog::numRows() const noexcept
{
    return m_numRows;
}

int TableSettingsDialog::numColumns() const noexcept
{
    return m_numColumns;
}

double TableSettingsDialog::tableWidth() const noexcept
{
    return m_tableWidth;
}

bool TableSettingsDialog::relativeWidth() const noexcept
{
    return m_relativeWidth;
}

void TableSettingsDialog::onOkButtonPressed()
{
    QNDEBUG(
        "widget::InsertTableToolButton::TableSettingsDialog",
        "TableSettingsDialog::onOkButtonPressed");

    QString error;
    if (!verifySettings(error)) {
        QNTRACE(
            "widget::InsertTableToolButton::TableSettingsDialog",
            "Error: " << error);

        m_ui->warningLine->setText(
            QStringLiteral("<font color=red>") + error +
            QStringLiteral("</font>"));

        m_ui->warningLine->setHidden(false);
        return;
    }

    if (!m_ui->warningLine->isHidden()) {
        m_ui->warningLine->clear();
        m_ui->warningLine->setHidden(true);
    }

    m_numRows = static_cast<int>(m_ui->numRowsSpinBox->value());
    m_numColumns = static_cast<int>(m_ui->numColumnsSpinBox->value());
    m_tableWidth = m_ui->tableWidthDoubleSpinBox->value();
    m_relativeWidth = checkRelativeWidth();

    QNTRACE(
        "widget::InsertTableToolButton::TableSettingsDialog",
        "Accepted: num rows = "
            << m_numRows << ", num columns = " << m_numColumns
            << ", table width = " << m_tableWidth << ", "
            << (m_relativeWidth ? "relative" : "absolute") << " width");

    accept();
}

void TableSettingsDialog::onCancelButtonPressed()
{
    QNDEBUG(
        "widget::InsertTableToolButton::TableSettingsDialog",
        "TableSettingsDialog::onCancelButtonPressed");

    reject();
}

bool TableSettingsDialog::verifySettings(QString & error) const
{
    const int numRows = m_ui->numRowsSpinBox->value();
    if (numRows < 1 || numRows > 30) {
        error = tr("Number of rows should be between 1 and 30");
        return false;
    }

    const int numColumns = m_ui->numColumnsSpinBox->value();
    if (numColumns < 1 || numColumns > 30) {
        error = tr("Number of columns should be between 1 and 30");
        return false;
    }

    const double tableWidth = m_ui->tableWidthDoubleSpinBox->value();
    const int intTableWidth = static_cast<int>(tableWidth);

    const bool tableRelativeWidth = checkRelativeWidth();
    if (tableRelativeWidth) {
        if (intTableWidth < 1 || intTableWidth > 100) {
            error = tr("Relative table width should be between 1 and 100");
            return false;
        }
    }
    else {
        if (intTableWidth < 1 || intTableWidth > 999999999) {
            error = tr("Bad table width in pixels number");
            return false;
        }
    }

    return true;
}

bool TableSettingsDialog::checkRelativeWidth() const
{
    const int comboBoxValueIndex = m_ui->tableWidthModeComboBox->currentIndex();
    return (comboBoxValueIndex == 1);
}

} // namespace quentier
