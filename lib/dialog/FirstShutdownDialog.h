/*
 * Copyright 2018-2024 Dmitry Ivanov
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

class FirstShutdownDialog;

} // namespace Ui

namespace quentier {

class FirstShutdownDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit FirstShutdownDialog(QWidget * parent = nullptr);
    ~FirstShutdownDialog() override;

private Q_SLOTS:
    void onCloseToTrayPushButtonPressed();
    void onClosePushButtonPressed();

private:
    Ui::FirstShutdownDialog * m_ui;
};

} // namespace quentier
