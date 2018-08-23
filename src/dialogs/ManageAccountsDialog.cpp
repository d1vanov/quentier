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

#include "ManageAccountsDialog.h"
#include "ui_ManageAccountsDialog.h"
#include "../models/AccountModel.h"
#include "../delegates/AccountDelegate.h"
#include "AddAccountDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/StringUtils.h>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QRegExp>
#include <QToolTip>

#define NUM_ACCOUNTS_MODEL_COLUMNS (3)

namespace quentier {

ManageAccountsDialog::ManageAccountsDialog(AccountModel & accountModel,
                                           const int currentAccountRow, QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::ManageAccountsDialog),
    m_accountModel(accountModel)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Manage accounts"));

    m_pUi->tableView->setModel(&m_accountModel);
    AccountDelegate * pDelegate = new AccountDelegate(this);
    m_pUi->tableView->setItemDelegate(pDelegate);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_pUi->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    m_pUi->tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif

    int numAvailableAccounts = m_accountModel.accounts().size();
    if ((currentAccountRow >= 0) && (currentAccountRow < numAvailableAccounts)) {
        QModelIndex index = m_accountModel.index(currentAccountRow, AccountModel::Columns::Username);
        m_pUi->tableView->setCurrentIndex(index);
    }

    QObject::connect(&m_accountModel, QNSIGNAL(AccountModel,badAccountDisplayName,ErrorString,int),
                     this, QNSLOT(ManageAccountsDialog,onBadAccountDisplayNameEntered,ErrorString,int));
    QObject::connect(m_pUi->addNewAccountButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onAddAccountButtonPressed));
    QObject::connect(m_pUi->revokeAuthenticationButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(ManageAccountsDialog,onRevokeAuthenticationButtonPressed));
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}

void ManageAccountsDialog::onAddAccountButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onAddAccountButtonPressed"));

    QScopedPointer<AddAccountDialog> addAccountDialog(new AddAccountDialog(m_accountModel.accounts(), this));
    addAccountDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,evernoteAccountAdditionRequested,QString,QNetworkProxy),
                     this, QNSIGNAL(ManageAccountsDialog,evernoteAccountAdditionRequested,QString,QNetworkProxy));
    QObject::connect(addAccountDialog.data(), QNSIGNAL(AddAccountDialog,localAccountAdditionRequested,QString,QString),
                     this, QNSIGNAL(ManageAccountsDialog,localAccountAdditionRequested,QString,QString));
    Q_UNUSED(addAccountDialog->exec())
}

void ManageAccountsDialog::onRevokeAuthenticationButtonPressed()
{
    QNDEBUG(QStringLiteral("ManageAccountsDialog::onRevokeAuthenticationButtonPressed"));

    QModelIndex currentIndex = m_pUi->tableView->currentIndex();
    if (Q_UNLIKELY(!currentIndex.isValid())) {
        QNTRACE(QStringLiteral("Current index is invalid"));
        return;
    }

    int currentRow = currentIndex.row();
    if (Q_UNLIKELY(currentRow < 0)) {
        QNTRACE(QStringLiteral("Current row is negative"));
        return;
    }

    const QVector<Account> & availableAccounts = m_accountModel.accounts();

    if (Q_UNLIKELY(currentRow >= availableAccounts.size())) {
        QNTRACE(QStringLiteral("Current row is larger than the number of available accounts"));
        return;
    }

    const Account & availableAccount = availableAccounts.at(currentRow);
    if (availableAccount.type() == Account::Type::Local) {
        QNTRACE(QStringLiteral("The current account is local, nothing to do"));
        return;
    }

    Q_EMIT revokeAuthentication(availableAccount.id());
}

void ManageAccountsDialog::onBadAccountDisplayNameEntered(ErrorString errorDescription, int row)
{
    QNINFO(QStringLiteral("Bad account display name entered: ")
           << errorDescription << QStringLiteral("; row = ") << row);

    QModelIndex index = m_accountModel.index(row, AccountModel::Columns::DisplayName);
    QRect rect = m_pUi->tableView->visualRect(index);
    QPoint pos = m_pUi->tableView->mapToGlobal(rect.bottomLeft());
    QToolTip::showText(pos, errorDescription.localizedString(), m_pUi->tableView);
}

} // namespace quentier
