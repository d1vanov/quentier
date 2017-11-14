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

#ifndef QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
#define QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <QDialog>
#include <QNetworkProxy>

namespace Ui {
class AddAccountDialog;
}

namespace quentier {

class AddAccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAccountDialog(const QVector<quentier::Account> & availableAccounts,
                              QWidget * parent = Q_NULLPTR);
    ~AddAccountDialog();

    bool isLocal() const;
    QString localAccountName() const;
    QString evernoteServerUrl() const;
    QString userFullName() const;

    bool localAccountAlreadyExists(const QString & name) const;

Q_SIGNALS:
    void evernoteAccountAdditionRequested(QString evernoteServer, QNetworkProxy proxy);
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

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setupNetworkProxySettingsFrame();
    void showLocalAccountAlreadyExistsMessage();

    void evaluateNetworkProxySettingsValidity();
    QNetworkProxy networkProxy(ErrorString & errorDescription) const;

private:
    Ui::AddAccountDialog *      m_pUi;
    QVector<quentier::Account>  m_availableAccounts;
    bool                        m_onceSuggestedFullName;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
