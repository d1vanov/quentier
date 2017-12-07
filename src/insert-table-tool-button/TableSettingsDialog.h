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

#ifndef QUENTIER_TABLE_SETTINGS_DIALOG_H
#define QUENTIER_TABLE_SETTINGS_DIALOG_H

#include <QDialog>
#include <quentier/utility/Macros.h>

namespace Ui {
class TableSettingsDialog;
}

class TableSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableSettingsDialog(QWidget * parent = 0);
    ~TableSettingsDialog();

    int numRows() const;
    int numColumns() const;
    double tableWidth() const;
    bool relativeWidth() const;

public Q_SLOTS:
    void onOkButtonPressed();
    void onCancelButtonPressed();

private:
    bool verifySettings(QString & error) const;
    bool checkRelativeWidth() const;

private:
    Ui::TableSettingsDialog *ui;

    int     m_numRows;
    int     m_numColumns;
    double  m_tableWidth;
    bool    m_relativeWidth;
};

#endif // QUENTIER_TABLE_SETTINGS_DIALOG_H
