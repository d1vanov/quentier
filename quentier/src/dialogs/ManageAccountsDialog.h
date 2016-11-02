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

#ifndef QUENTIER_MANAGE_ACCOUNTS_DIALOG_H
#define QUENTIER_MANAGE_ACCOUNTS_DIALOG_H

#include "AvailableAccount.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QVector>
#include <QStringListModel>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

namespace Ui {
class ManageAccountsDialog;
}

class ManageAccountsDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ManageAccountsDialog(const QVector<AvailableAccount> & availableAccounts,
                                  QWidget * parent = Q_NULLPTR);
    virtual ~ManageAccountsDialog();

Q_SIGNALS:
    void evernoteAccountAdditionRequested(QString evernoteServer);
    void localAccountAdditionRequested(QString name);
    void revokeAuthentication(qevercloud::UserID id);

public Q_SLOTS:
    void onAvailableAccountsChanged(const QVector<AvailableAccount> & availableAccounts);

private Q_SLOTS:
    void onAddAccountButtonPressed();
    void onRevokeAuthenticationButtonPressed();

private:
    void updateAvailableAccountsInView();

private:
    Ui::ManageAccountsDialog *  m_pUi;
    QVector<AvailableAccount>   m_availableAccounts;
    QStringListModel            m_accountListModel;
};

#endif // QUENTIER_MANAGE_ACCOUNTS_DIALOG_H
