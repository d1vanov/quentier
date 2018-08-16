/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_DIALOGS_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H
#define QUENTIER_DIALOGS_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>

namespace Ui {
class LocalStorageVersionTooHighDialog;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(LocalStorageManager)

class LocalStorageVersionTooHighDialog: public QDialog
{
    Q_OBJECT
public:
    explicit LocalStorageVersionTooHighDialog(const Account & currentAccount,
                                              LocalStorageManager & localStorageManager,
                                              QWidget * parent = Q_NULLPTR);
    ~LocalStorageVersionTooHighDialog();

Q_SIGNALS:
    void shouldSwitchToAccount(Account account);
    void shouldCreateNewAccount();
    void shouldQuitApp();

private Q_SLOTS:
    void onSwitchToAccountPushButtonPressed();

private:
    virtual void reject() Q_DECL_OVERRIDE;

private:
    void createConnections();
    void initializeDetails(const Account & currentAccount,
                           LocalStorageManager & localStorageManager);

private:
    Ui::LocalStorageVersionTooHighDialog * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_LOCAL_STORAGE_VERSION_TOO_HIGH_DIALOG_H
