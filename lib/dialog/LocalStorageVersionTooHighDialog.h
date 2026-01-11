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

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>

#include <QDialog>

namespace Ui {

class LocalStorageVersionTooHighDialog;

} // namespace Ui

class QItemSelection;

namespace quentier {

class ErrorString;
class AccountModel;
class AccountFilterModel;

class LocalStorageVersionTooHighDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit LocalStorageVersionTooHighDialog(
        const Account & currentAccount, AccountModel & accountModel,
        local_storage::ILocalStoragePtr localStorage,
        QWidget * parent = nullptr);

    ~LocalStorageVersionTooHighDialog() override;

Q_SIGNALS:
    void shouldSwitchToAccount(Account account);
    void shouldCreateNewAccount();
    void shouldQuitApp();

private Q_SLOTS:
    void onSwitchToAccountPushButtonPressed();
    void onCreateNewAccountButtonPressed();
    void onQuitAppButtonPressed();

    void onAccountViewSelectionChanged(
        const QItemSelection & selected, const QItemSelection & deselected);

private:
    void reject() override;

private:
    void createConnections();
    void initializeDetails(const Account & currentAccount);
    void setErrorToStatusBar(const ErrorString & error);

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    Ui::LocalStorageVersionTooHighDialog * m_ui;
    AccountFilterModel * m_accountFilterModel;
};

} // namespace quentier
