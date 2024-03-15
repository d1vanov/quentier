/*
 * Copyright 2017-2024 Dmitry Ivanov
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

class WelcomeToQuentierDialog;

} // namespace Ui

namespace quentier {

class WelcomeToQuentierDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit WelcomeToQuentierDialog(QWidget * parent = nullptr);
    ~WelcomeToQuentierDialog() override;

    [[nodiscard]] QString evernoteServer() const;

private Q_SLOTS:
    void onContinueWithLocalAccountPushButtonPressed();
    void onLogInToEvernoteAccountPushButtonPressed();

private:
    Ui::WelcomeToQuentierDialog * m_ui;
};

} // namespace quentier
