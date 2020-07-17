/*
 * Copyright 2018-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_ACCOUNT_DELETE_ACOUNT_DIALOG_H
#define QUENTIER_LIB_ACCOUNT_DELETE_ACOUNT_DIALOG_H

#include <quentier/types/Account.h>
#include <quentier/utility/Macros.h>

#include <QDialog>
#include <QPointer>

namespace Ui {
class DeleteAccountDialog;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountModel)

class DeleteAccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DeleteAccountDialog(
        const Account & account, AccountModel & model,
        QWidget * parent = nullptr);

    virtual ~DeleteAccountDialog() override;

private Q_SLOTS:
    void onConfirmationLineEditTextEdited(const QString & text);

private:
    virtual void accept() override;

private:
    void setStatusBarText(const QString & text);

private:
    Ui::DeleteAccountDialog *   m_pUi;
    Account         m_account;
    AccountModel &  m_model;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_DELETE_ACOUNT_DIALOG_H
