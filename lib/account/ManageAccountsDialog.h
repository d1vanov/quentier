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

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QDialog>
#include <QNetworkProxy>

namespace Ui {

class ManageAccountsDialog;

} // namespace Ui

class QItemSelection;

namespace quentier {

class AccountManager;

class ManageAccountsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManageAccountsDialog(
        AccountManager & accountManager, int currentAccountRow = -1,
        QWidget * parent = nullptr);

    ~ManageAccountsDialog() override;

Q_SIGNALS:
    void evernoteAccountAdditionRequested(
        QString evernoteServer, QNetworkProxy proxy);

    void localAccountAdditionRequested(QString name, QString fullName);
    void revokeAuthentication(qevercloud::UserID id);

public Q_SLOTS:
    void onAuthenticationRevoked(
        bool success, ErrorString errorDescription, qevercloud::UserID userId);

private Q_SLOTS:
    void onAddAccountButtonPressed();
    void onRevokeAuthenticationButtonPressed();
    void onDeleteAccountButtonPressed();
    void onBadAccountDisplayNameEntered(ErrorString errorDescription, int row);

    void onAccountSelectionChanged(
        const QItemSelection & selected, const QItemSelection & deselected);

private:
    void setStatusBarText(const QString & text);

private:
    Ui::ManageAccountsDialog * m_ui;
    AccountManager & m_accountManager;
};

} // namespace quentier
