/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "AccountDelegate.h"
#include "AccountManager.h"
#include "AccountModel.h"
#include "AddAccountDialog.h"
#include "DeleteAccountDialog.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StringUtils.h>

#include <QAbstractTableModel>
#include <QRegExp>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <QItemSelection>

#include <memory>

#define NUM_ACCOUNTS_MODEL_COLUMNS (3)

namespace quentier {

ManageAccountsDialog::ManageAccountsDialog(
        AccountManager & accountManager, const int currentAccountRow,
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
    m_pUi->tableView->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    int numAvailableAccounts = accountModel.accounts().size();
    if ((currentAccountRow >= 0) &&
        (currentAccountRow < numAvailableAccounts))
    {
        QModelIndex index = accountModel.index(
            currentAccountRow,
            AccountModel::Columns::Username);
        m_pUi->tableView->setCurrentIndex(index);
    }

    QObject::connect(
        &accountModel,
        &AccountModel::badAccountDisplayName,
        this,
        &ManageAccountsDialog::onBadAccountDisplayNameEntered);

    QObject::connect(
        m_pUi->addNewAccountButton,
        &QPushButton::pressed,
        this,
        &ManageAccountsDialog::onAddAccountButtonPressed);

    QObject::connect(
        m_pUi->deleteAccountButton,
        &QPushButton::pressed,
        this,
        &ManageAccountsDialog::onDeleteAccountButtonPressed);

    QObject::connect(
        m_pUi->tableView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        &ManageAccountsDialog::onAccountSelectionChanged);

    // Revoke authentication button would only work if current account is
    // Evernote one, not a local one. So hiding the button if current account
    // is local
    Account currentAccount = m_accountManager.currentAccount();
    if (currentAccount.type() != Account::Type::Evernote) {
        m_pUi->revokeAuthenticationButton->hide();
    }
    else {
        QObject::connect(
            m_pUi->revokeAuthenticationButton,
            &QPushButton::pressed,
            this,
            &ManageAccountsDialog::onRevokeAuthenticationButtonPressed);
    }
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}

void ManageAccountsDialog::onAuthenticationRevoked(
    bool success, ErrorString errorDescription, qevercloud::UserID userId)
{
    QNDEBUG("ManageAccountsDialog::onAuthenticationRevoked: success = "
        << (success ? "true" : "false") << ", error description = "
        << errorDescription << ", user id = " << userId);

    if (!success)
    {
        QNWARNING("Failed to revoke authentication for account with id "
            << userId << ": " << errorDescription);

        Q_UNUSED(warningMessageBox(
            this,
            tr("Failed to revoke authentication"),
            tr("Failed to revoke authentication for account with id") +
            QStringLiteral(" ") + QString::number(userId) +
            QStringLiteral(": ") + errorDescription.localizedString()));

        return;
    }

    Account accountWithRevokedAuthentication;
    bool foundAccountWithRevokedAuthentication = false;
    const auto & availableAccounts = m_accountManager.availableAccounts();

    for(const auto & availableAccount: availableAccounts)
    {
        if (availableAccount.type() != Account::Type::Evernote) {
            continue;
        }

        if (availableAccount.id() != userId) {
            continue;
        }

        accountWithRevokedAuthentication = availableAccount;
        foundAccountWithRevokedAuthentication = true;
        break;
    }

    if (foundAccountWithRevokedAuthentication) {
        setStatusBarText(tr("Revoked authentication for account") +
            QStringLiteral(": ") + accountWithRevokedAuthentication.name() +
            QStringLiteral(" (id = ") + QString::number(userId) +
            QStringLiteral(")"));
    }
    else {
        QNWARNING("Failed to find the account for which the authentication "
            << "was revoked: " << userId);
    }
}

void ManageAccountsDialog::onAddAccountButtonPressed()
{
    QNDEBUG("ManageAccountsDialog::onAddAccountButtonPressed");

    setStatusBarText(QString());

    auto pAddAccountDialog = std::make_unique<AddAccountDialog>(
        m_accountManager.accountModel().accounts(),
        this);

    pAddAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pAddAccountDialog.get(),
                     QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,
                              QString,QNetworkProxy),
                     this,
                     QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,
                              QString,QNetworkProxy));
    QObject::connect(pAddAccountDialog.get(),
                     QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,
                              QString,QString),
                     this,
                     QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,
                              QString,QString));
    Q_UNUSED(pAddAccountDialog->exec())
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
        m_accountManager.availableAccounts();

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
        setStatusBarText(tr("Can't revoke authentication for local account"));
        return;
    }

    QString accountDetails;
    QTextStream strm(&accountDetails);
    strm << tr("name") << " = " << availableAccount.name();

    auto accountDisplayName = availableAccount.displayName();
    if (!accountDisplayName.isEmpty()) {
        strm << " (" << availableAccount.displayName() << ")";
    }

    strm << ", " << tr("evernote server") << " = " <<
        availableAccount.evernoteHost();

    strm.flush();

    int res = questionMessageBox(
        this,
        tr("Revoke authentication?"),
        tr("Are you sure you want to revoke authentication for this account?"),
        accountDetails);

    if (res == QMessageBox::Cancel) {
        QNDEBUG("Revoking authentication for account was cancelled: "
            << availableAccount);
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

    auto pDeleteAccountDialog = std::make_unique<DeleteAccountDialog>(
        accountToDelete,
        m_accountManager.accountModel(),
        this);

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
