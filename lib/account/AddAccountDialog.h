/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_ACCOUNT_ADD_ACCOUNT_DIALOG_H
#define QUENTIER_LIB_ACCOUNT_ADD_ACCOUNT_DIALOG_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>

#include <QDialog>
#include <QNetworkProxy>

namespace Ui {
class AddAccountDialog;
}

namespace quentier {

class AddAccountDialog final: public QDialog
{
    Q_OBJECT
public:
    explicit AddAccountDialog(
        QVector<quentier::Account> availableAccounts,
        QWidget * parent = nullptr);

    ~AddAccountDialog() override;

    [[nodiscard]] bool isLocal() const;
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

    void accept() override;

private:
    void setupNetworkProxySettingsFrame();
    void showLocalAccountAlreadyExistsMessage();

    void evaluateNetworkProxySettingsValidity();

    [[nodiscard]] QNetworkProxy networkProxy(
        ErrorString & errorDescription) const;

private:
    Ui::AddAccountDialog * m_pUi;
    QVector<quentier::Account> m_availableAccounts;
    bool m_onceSuggestedFullName = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_ADD_ACCOUNT_DIALOG_H
