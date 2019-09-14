/*
 * Copyright 2018-2019 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DIALOG_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H
#define QUENTIER_LIB_DIALOG_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>

#include <QDialog>

namespace Ui {
class LocalStorageVersionTooHighDialog;
}

QT_FORWARD_DECLARE_CLASS(QItemSelection)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ErrorString)
QT_FORWARD_DECLARE_CLASS(AccountModel)
QT_FORWARD_DECLARE_CLASS(AccountFilterModel)
QT_FORWARD_DECLARE_CLASS(LocalStorageManager)

class LocalStorageVersionTooHighDialog: public QDialog
{
    Q_OBJECT
public:
    explicit LocalStorageVersionTooHighDialog(
        const Account & currentAccount,
        AccountModel & accountModel,
        LocalStorageManager & localStorageManager,
        QWidget * parent = Q_NULLPTR);

    ~LocalStorageVersionTooHighDialog();

Q_SIGNALS:
    void shouldSwitchToAccount(Account account);
    void shouldCreateNewAccount();
    void shouldQuitApp();

private Q_SLOTS:
    void onSwitchToAccountPushButtonPressed();
    void onCreateNewAccountButtonPressed();
    void onQuitAppButtonPressed();

    void onAccountViewSelectionChanged(
        const QItemSelection & selected,
        const QItemSelection & deselected);

private:
    virtual void reject() Q_DECL_OVERRIDE;

private:
    void createConnections();
    void initializeDetails(const Account & currentAccount,
                           LocalStorageManager & localStorageManager);
    void setErrorToStatusBar(const ErrorString & error);

private:
    Ui::LocalStorageVersionTooHighDialog *  m_pUi;
    AccountFilterModel *                    m_pAccountFilterModel;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H
