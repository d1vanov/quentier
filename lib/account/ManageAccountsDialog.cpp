/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include "ManageAccountsDialog.h"
#include "ui_ManageAccountsDialog.h"
#include "DeleteAccountDialog.h"
#include "AccountManager.h"
#include "AccountModel.h"
#include "AccountDelegate.h"
#include "AddAccountDialog.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/types/ErrorString.h>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QScopedPointer>
#include <QRegExp>
#include <QToolTip>
#include <QItemSelection>

#define NUM_ACCOUNTS_MODEL_COLUMNS (3)

namespace quentier {

ManageAccountsDialog::ManageAccountsDialog(AccountManager & accountManager,
                                           const int currentAccountRow,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::ManageAccountsDialog),
    m_accountManager(accountManager)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Manage accounts"));

    m_pUi->statusBarLabel->hide();

    AccountModel & accountModel = m_accountManager.accountModel();

    m_pUi->tableView->setModel(&accountModel);
    AccountDelegate * pDelegate = new AccountDelegate(this);
    m_pUi->tableView->setItemDelegate(pDelegate);
    m_pUi->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    int numAvailableAccounts = accountModel.accounts().size();
    if ((currentAccountRow >= 0) && (currentAccountRow < numAvailableAccounts)) {
        QModelIndex index = accountModel.index(currentAccountRow,
                                               AccountModel::Columns::Username);
        m_pUi->tableView->setCurrentIndex(index);
    }

    QObject::connect(&accountModel,
                     QNSIGNAL(AccountModel,badAccountDisplayName,
                              ErrorString,int),
                     this,
                     QNSLOT(ManageAccountsDialog,onBadAccountDisplayNameEntered,
                            ErrorString,int));
    QObject::connect(m_pUi->addNewAccountButton,
                     QNSIGNAL(QPushButton,pressed),
                     this,
                     QNSLOT(ManageAccountsDialog,onAddAccountButtonPressed));
    QObject::connect(m_pUi->revokeAuthenticationButton,
                     QNSIGNAL(QPushButton,pressed),
                     this,
                     QNSLOT(ManageAccountsDialog,
                            onRevokeAuthenticationButtonPressed));
    QObject::connect(m_pUi->deleteAccountButton,
                     QNSIGNAL(QPushButton,pressed),
                     this,
                     QNSLOT(ManageAccountsDialog,onDeleteAccountButtonPressed));
    QObject::connect(m_pUi->tableView->selectionModel(),
                     QNSIGNAL(QItemSelectionModel,selectionChanged,
                              QItemSelection,QItemSelection),
                     this,
                     QNSLOT(ManageAccountsDialog,onAccountSelectionChanged,
                            QItemSelection,QItemSelection));
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}

void ManageAccountsDialog::onAddAccountButtonPressed()
{
    QNDEBUG("ManageAccountsDialog::onAddAccountButtonPressed");

    setStatusBarText(QString());

    QScopedPointer<AddAccountDialog> addAccountDialog(
        new AddAccountDialog(m_accountManager.accountModel().accounts(), this));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(addAccountDialog.data(),
                     QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,
                              QString,QNetworkProxy),
                     this,
                     QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,
                              QString,QNetworkProxy));
    QObject::connect(addAccountDialog.data(),
                     QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,
                              QString,QString),
                     this,
                     QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,
                              QString,QString));
    Q_UNUSED(addAccountDialog->exec())
}

void ManageAccountsDialog::onRevokeAuthenticationButtonPressed()
{
    QNDEBUG("ManageAccountsDialog::onRevokeAuthenticationButtonPressed");

    setStatusBarText(QString());

    QModelIndex currentIndex = m_pUi->tableView->currentIndex();
    if (Q_UNLIKELY(!currentIndex.isValid())) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current index is invalid)");
        setStatusBarText(error.localizedString());
        return;
    }

    int currentRow = currentIndex.row();
    if (Q_UNLIKELY(currentRow < 0)) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current row is negative)");
        setStatusBarText(error.localizedString());
        return;
    }

    const QVector<Account> & availableAccounts =
        m_accountManager.accountModel().accounts();

    if (Q_UNLIKELY(currentRow >= availableAccounts.size())) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current row is larger than the number "
                << "of available accounts)");
        setStatusBarText(error.localizedString());
        return;
    }

    const Account & availableAccount = availableAccounts.at(currentRow);
    if (availableAccount.type() == Account::Type::Local) {
        QNDEBUG("The current account is local, nothing to do");
        return;
    }

    Q_EMIT revokeAuthentication(availableAccount.id());
}

void ManageAccountsDialog::onDeleteAccountButtonPressed()
{
    QNDEBUG("ManageAccountsDialog::onDeleteAccountButtonPressed");

    setStatusBarText(QString());

    QModelIndex currentIndex = m_pUi->tableView->currentIndex();
    if (Q_UNLIKELY(!currentIndex.isValid())) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current index is invalid)");
        setStatusBarText(error.localizedString());
        return;
    }

    int currentRow = currentIndex.row();
    if (Q_UNLIKELY(currentRow < 0)) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current row is negative)");
        setStatusBarText(error.localizedString());
        return;
    }

    const QVector<Account> & availableAccounts =
        m_accountManager.accountModel().accounts();

    if (Q_UNLIKELY(currentRow >= availableAccounts.size())) {
        ErrorString error(QT_TR_NOOP("No account is selected"));
        QNDEBUG(error << " (current row is larger than the number "
                << "of available accounts)");
        setStatusBarText(error.localizedString());
        return;
    }

    const Account & accountToDelete = availableAccounts.at(currentRow);
    QNTRACE("Account to be deleted: " << accountToDelete);

    Account currentAccount = m_accountManager.currentAccount();

    if ( (accountToDelete.type() == currentAccount.type()) &&
         (accountToDelete.name() == currentAccount.name()) &&
         (accountToDelete.id() == currentAccount.id()) )
    {
        ErrorString error;
        if (availableAccounts.size() != 1) {
            error.setBase(QT_TR_NOOP("Can't delete the currently used account, "
                                     "please switch to another account first"));
        }
        else {
            error.setBase(QT_TR_NOOP("Can't delete the last remaining account"));
        }

        QNDEBUG(error << ", current account: " << currentAccount
                << "\nAccount to be deleted: " << accountToDelete);
        setStatusBarText(error.localizedString());
        return;
    }

    if (availableAccounts.size() == 1)
    {
        ErrorString error(QT_TR_NOOP("Can't delete the last remaining account"));
        QNDEBUG(error << ", current account: " << currentAccount
                << "\nAccount to be deleted: " << accountToDelete);
        setStatusBarText(error.localizedString());
        return;
    }

    QScopedPointer<DeleteAccountDialog> pDeleteAccountDialog(
        new DeleteAccountDialog(accountToDelete, m_accountManager.accountModel(),
                                this));
    pDeleteAccountDialog->setWindowModality(Qt::WindowModal);
    Q_UNUSED(pDeleteAccountDialog->exec())
}

void ManageAccountsDialog::onBadAccountDisplayNameEntered(
    ErrorString errorDescription, int row)
{
    QNINFO("Bad account display name entered: "
           << errorDescription << "; row = " << row);

    setStatusBarText(QString());

    QModelIndex index =
        m_accountManager.accountModel().index(row,
                                              AccountModel::Columns::DisplayName);
    QRect rect = m_pUi->tableView->visualRect(index);
    QPoint pos = m_pUi->tableView->mapToGlobal(rect.bottomLeft());
    QToolTip::showText(pos, errorDescription.localizedString(), m_pUi->tableView);
}

void ManageAccountsDialog::onAccountSelectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("ManageAccountsDialog::onAccountSelectionChanged");

    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    setStatusBarText(QString());
}

void ManageAccountsDialog::setStatusBarText(const QString & text)
{
    QNDEBUG("ManageAccountsDialog::setStatusBarText: " << text);

    m_pUi->statusBarLabel->setText(text);

    if (!text.isEmpty()) {
        m_pUi->statusBarLabel->show();
    }
    else {
        m_pUi->statusBarLabel->hide();
    }
}

} // namespace quentier
