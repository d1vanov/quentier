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
#include <QList>
#include <QNetworkProxy>

namespace Ui {
class AddAccountDialog;
} // namespace Ui

namespace quentier {

class AddAccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAccountDialog(
        QList<quentier::Account> availableAccounts,
        QWidget * parent = nullptr);

    ~AddAccountDialog() override;

    [[nodiscard]] bool isLocal() const noexcept;
    [[nodiscard]] QString localAccountName() const;
    [[nodiscard]] QString evernoteServerUrl() const;
    [[nodiscard]] QString userFullName() const;

    [[nodiscard]] bool localAccountAlreadyExists(const QString & name) const;

Q_SIGNALS:
    void evernoteAccountAdditionRequested(
        QString evernoteServer, QNetworkProxy proxy);

    void localAccountAdditionRequested(QString name, QString fullName);

private Q_SLOTS:
    void onCurrentAccountTypeChanged(int index);
    void onLocalAccountNameChosen();
    void onLocalAccountUsernameEdited(const QString & username);

    void onUseNetworkProxyToggled(bool checked);

    void onNetworkProxyTypeChanged(int index);
    void onNetworkProxyHostChanged();
    void onNetworkProxyPortChanged(int port);

    void onNetworkProxyShowPasswordToggled(bool checked);

private: // QDialog
    void accept() override;

private:
    void setupNetworkProxySettingsFrame();
    void showLocalAccountAlreadyExistsMessage();
    void evaluateNetworkProxySettingsValidity();

    [[nodiscard]] QNetworkProxy networkProxy(
        ErrorString & errorDescription) const;

private:
    const QList<quentier::Account> m_availableAccounts;
    Ui::AddAccountDialog * m_ui;
    bool m_onceSuggestedFullName = false;
};

} // namespace quentier
