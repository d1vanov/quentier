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

#pragma once

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
    ~TableSettingsDialog() override;

    [[nodiscard]] int numRows() const noexcept;
    [[nodiscard]] int numColumns() const noexcept;
    [[nodiscard]] double tableWidth() const noexcept;
    [[nodiscard]] bool relativeWidth() const noexcept;

public Q_SLOTS:
    void onOkButtonPressed();
    void onCancelButtonPressed();

private:
    [[nodiscard]] bool verifySettings(QString & error) const;
    [[nodiscard]] bool checkRelativeWidth() const;

private:
    Ui::TableSettingsDialog * m_ui;

    int m_numRows = 0;
    int m_numColumns = 0;
    double m_tableWidth = 0.0;
    bool m_relativeWidth = false;
};

} // namespace quentier
