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

#include <quentier/types/Account.h>

#include <QDialog>
#include <QPointer>

namespace Ui {
class DeleteAccountDialog;
} // namespace Ui

namespace quentier {

class AccountModel;

class DeleteAccountDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit DeleteAccountDialog(
        Account account, AccountModel & model,
        QWidget * parent = nullptr);

    ~DeleteAccountDialog() override;

private Q_SLOTS:
    void onConfirmationLineEditTextEdited(const QString & text);

private: // QDialog
    void accept() override;

private:
    void setStatusBarText(const QString & text);

private:
    const Account m_account;
    Ui::DeleteAccountDialog * m_ui;
    AccountModel & m_model;
};

} // namespace quentier
