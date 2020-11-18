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

#ifndef QUENTIER_LIB_WIDGET_TABLE_SETTINGS_DIALOG_H
#define QUENTIER_LIB_WIDGET_TABLE_SETTINGS_DIALOG_H

#include <QDialog>

namespace Ui {
class TableSettingsDialog;
}

namespace quentier {

class TableSettingsDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit TableSettingsDialog(QWidget * parent = nullptr);
    virtual ~TableSettingsDialog() override;

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
    Ui::TableSettingsDialog * ui;

    int m_numRows = 0;
    int m_numColumns = 0;
    double m_tableWidth = 0.0;
    bool m_relativeWidth = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_TABLE_SETTINGS_DIALOG_H
